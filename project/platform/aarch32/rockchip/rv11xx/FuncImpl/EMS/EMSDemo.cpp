#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

EMSDemoImpl::EMSDemoImpl()
    : mThreadQuit(false),
    mAlgoInitOk(false),
    mUseVdecNotVi(false),
    pDetector(nullptr),
    pSimpleServer(nullptr),
    mOriginWidth(1280),
    mOriginHeight(720),
    mOriginChns(3),
    mCropRgaChn(DRM_RGA_CHANNEL_14),
    mZoomRgaChn(DRM_RGA_CHANNEL_15),
    mInferRgaChn(DRM_RGA_CHANNEL_00),
    mDispPrevRgaChn(DRM_RGA_CHANNEL_01),
    mVideoFirstInChn(DRM_VI_CHANNEL_00),
    mVideoInputWidth(1280),
    mVideoInputHeight(720),
    mVideoInputDevNode("/dev/video0"),
    mImageFlipMode(RGA_FLIP_NULL),
    mVdecImagePixType(DRM_IMAGE_TYPE_NV12),
    mInputImagePixType(DRM_IMAGE_TYPE_NV12),
    mInputImagePixTypeEx(DRM_IMAGE_TYPE_NV16),
    mOutputImagePixType(DRM_IMAGE_TYPE_RGB888),
    mInferImagePixType(DRM_IMAGE_TYPE_BGR888),
    mPrimaryDispSwap(false),
    mOverlayDispSwap(false),
    mDisplayDeviceCard("/dev/dri/card0"),
    mPrimaryXoffset(0),
    mPrimaryYoffset(0),
    mPrimaryDispWidth(1440/* 1280 */),
    mPrimaryDispHeight(810/* 720 */),
    mOverlayXoffset(0),
    mOverlayYoffset(0),
    mOverlayDispWidth(1280),
    mOverlayDispHeight(720),
    mPrimaryVoDisp(true),
    mOverlayVoDisp(false),
    mPrimaryZpos(0),
    mOverlayZpos(1),
    mPrimaryVoChn(DRM_VO_CHANNEL_00),
    mOverlayVoChn(DRM_VO_CHANNEL_01),
    mPrimaryDispLayers(VO_PLANE_PRIMARY),
    mOverlayDispLayers(VO_PLANE_OVERLAY),
    mVideoVencEnOK(false),
    mRtspVencEnOK(false),
    mRtspVencChn(DRM_VENC_CHANNEL_00),
    mVideoVencChn(DRM_VENC_CHANNEL_01),
    mRtspVencRgaChn(DRM_RGA_CHANNEL_06),
    mVideoVencType(DRM_CODEC_TYPE_H264),
    mVideoVencRcMode(VENC_RC_MODE_H264VBR)
{
    parseApplicationConfigParam(APPLF_CONFIG_JSONFILE);
    parseVideoDecodeParam(VIDEO_DECODE_JSONFILE);

    getApi()->getSys().setupEnableRGAFlushCache(mEMSConfig.enableRGAFlushCache);

    std::unordered_map<int, std::string> fonts;
    fonts.insert(std::make_pair(0, "/opt/aure/fonts/simhei.ttf"));
    getApi()->getRgn().registerFontLibraries(fonts);

    if (access("/opt/aure/ARM.png", F_OK) == 0) {
        mBlendImage = cv::imread("/opt/aure/ARM.png");
    }
}

EMSDemoImpl::~EMSDemoImpl()
{
    mThreadQuit = true;
#if defined(CONFIG_XLIB)
    x_main_loop_quit(mMainLoop);
#endif

    if (mVideoVencEnOK) {
        videoEncodeExit();
    }

    if (mDispFrameCapThreadId.joinable()) {
        mDispFrameCapThreadId.join();
    }

    if (mInferFrameCapThreadId.joinable()) {
        mInferFrameCapThreadId.join();
    }

    if (mDispPostThreadId.joinable()) {
        mDispPostThreadId.join();
    }

    if (mDrawPostThreadId.joinable()) {
        mDrawPostThreadId.join();
    }

    if (mInferPostThreadId.joinable()) {
        mInferPostThreadId.join();
    }

    mediaDeinit();
}

int EMSDemoImpl::init()
{
    int ret = -1;

    rtspClientInit();

    if (mEMSConfig.enableThisChannel) {
        ret = mediaInit();
        if (ret) {
            printf("mediaInit failed, return:[%d]\n", ret);
            return -1;
        }
    } else {
        printf("EMS Application startup faliled, because no channel enabled\n");
        return -1;
    }

    if (mVideoVencEnOK) {
        videoEncodeInit();
    }

    std::thread(std::bind(&EMSDemoImpl::algoInitThread, this)).detach();

    mDispFrameCapThreadId = std::thread(std::bind(&EMSDemoImpl::dispStreamCaptureThread, this));
    mInferFrameCapThreadId =  std::thread(std::bind(&EMSDemoImpl::inferStreamCaptureThread, this));

    mDispPostThreadId = std::thread(std::bind(&EMSDemoImpl::dispPostThread, this));
    mDrawPostThreadId = std::thread(std::bind(&EMSDemoImpl::drawPostThread, this));
    mInferPostThreadId = std::thread(std::bind(&EMSDemoImpl::inferPostThread, this));

    if (mRtspVencEnOK) {
        rtspServerInit();
    }

#if defined(CONFIG_XLIB)
    mMainContex = x_main_context_new();
    mMainLoop = x_main_loop_new(mMainContex, FALSE);
    x_main_loop_run(mMainLoop);
    x_main_loop_unref(mMainLoop);
#else
    while (!mThreadQuit) {
        sleep(10);
    }
#endif

    return 0;
}

void EMSDemoImpl::dispStreamCaptureThread()
{
    int cpus = 0;
    cpu_set_t cpuset;

    pthread_setname_np(pthread_self(), "dispCapThread");

    cpus = get_nprocs();
    CPU_ZERO(&cpuset);
    CPU_SET(cpus <= 1 ? 0 : cpus - 3, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (!mThreadQuit) {
        media_buffer_t mediaFrame = getApi()->getRga().getFrame(mDispPrevRgaChn, 100);
        if (mediaFrame) {
            if (!mDispRawFrameRing.insert(mediaFrame)) {
                getApi()->getRga().releaseFrame(mediaFrame);
            }
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(100 * 1000));
    }
}

void EMSDemoImpl::inferStreamCaptureThread()
{
    int cpus = 0;
    cpu_set_t cpuset;
    int abortCount = 0;
    int abortQuitCount = 2;
    bool bLockSetFishFecStatus = false;

    pthread_setname_np(pthread_self(), "inferCapThread");

    cpus = get_nprocs();
    CPU_ZERO(&cpuset);
    CPU_SET(cpus <= 1 ? 0 : cpus - 3, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    if (mUseVdecNotVi) {
        abortQuitCount = 5;
    } else {
        abortQuitCount = 2;
    }

    while (!mThreadQuit) {
        media_buffer_t mediaFrame = getApi()->getRga().getFrame(mInferRgaChn, 500);
        if (mediaFrame) {
            abortCount = 0;

#if defined(CONFIG_RKNPU)
            if (pDetector && mAlgoInitOk) {
                int ret = pDetector->forward(mediaFrame);
                if (ret == 0) {
                    mAlgForwardFishRing.insert(1);
                }
            }
#endif
            if (mediaFrame) {
                getApi()->getRga().releaseFrame(mediaFrame);
            }
        } else {
            abortCount++;
        }

        if (abortCount > abortQuitCount) {
            printf("\033[1;33mlong time can't capture frame\033[0m\n");
            abort();
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(200 * 1000));
    }
}

API_END_NAMESPACE(EMS)
