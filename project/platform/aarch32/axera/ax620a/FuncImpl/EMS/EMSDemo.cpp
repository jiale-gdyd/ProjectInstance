#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

EMSDemoImpl::EMSDemoImpl() : mThreadFin(false)
{
    mPipelines.clear();
}

EMSDemoImpl::~EMSDemoImpl()
{
    if (mOsdThreadId.joinable()) {
        mOsdThreadId.join();
    }

    getApi()->stop();
    mThreadFin = true;
    mediaDeinit();
}

int EMSDemoImpl::init()
{
    media::axsys_init_params_t sysParams;
    sysParams.enableNPU = true;
    sysParams.npuInitInfo.npuHdrMode = AX_NPU_VIRTUAL_1_1;
#if 0
    sysParams.enableCamera = true;
    sysParams.cameraInfo.sysCameraCase = media::SYS_CASE_SINGLE_GC4653;
    sysParams.cameraInfo.sysCameraHdrMode = media::SNS_MODE_NONE;
    sysParams.cameraInfo.sysCameraSnsType = media::GALAXYCORE_GC4653;
    sysParams.cameraInfo.cameraIvpsFrameRate = 25;
#else
    sysParams.enableCamera = false;
    media::axsys_pool_cfg_t poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 10},
    };
    sysParams.sysCommonArgs.poolCfgCount = 1;
    sysParams.sysCommonArgs.poolCfg = poolcfg;
#endif

    if (getApi()->init(&sysParams) != 0) {
        return -1;
    }

    /* 算法初始化 */
    pDetector = std::make_shared<Ai::AiDetector>();
    if (!pDetector) {
        return -2;
    }

    mModelAlgoType = Ai::MT_DET_YOLOV5;
    if (pDetector->init("/opt/aure/yolov5.joint", 1920, 1080, 80, 0.4f, 0.45f, mModelAlgoType, Ai::AUTHOR_AXERA)) {
        return -3;
    }
    pDetector->getModelInputWidthHeight(mModelWidth, mModelHeight, mModelFormat);

    if (mediaInit()) {
        return -4;
    }

    mRtspClient = new rtsp::RTSPClient();
    if (mRtspClient != nullptr) {
        return -5;
    }

    if (mRtspClient->openURL("http://192.168.1.7", 1, 2) != 0) {
        return -6;
    }

    if (mRtspClient->playURL(RtspFrameHandler, &mPipelines[0], "me", NULL, NULL) != 0) {
        return -7;
    }

    getApi()->run();
    return 0;
}

API_END_NAMESPACE(EMS)
