#include "EMSDemo.hpp"

namespace ems {
int EMSDemo::pipelineCreate()
{
    mPipelines.clear();

    /* vin[0:0]->ivps[0]->venc[0]->rtsp */
    axpi::axpipe_t pipe;
    memset(&pipe, 0, sizeof(axpi::axpipe_t));
    pipe.ivps.group = 0;
    pipe.ivps.framerate = mCamFPS;
    pipe.ivps.width = 1920;
    pipe.ivps.height = 1080;
    pipe.ivps.regions = 1;
    pipe.enable = true;
    pipe.pipeId = mStartID++;
    pipe.inType = axpi::AXPIPE_INPUT_VIN;
    pipe.outType = axpi::AXPIPE_OUTPUT_RTSP_H264;
    pipe.bThreadQuit = false;
    pipe.vinChn = 0;
    pipe.vinPipe = 0;
    pipe.venc.channel = 0;
    pipe.venc.rtspPort = 8554;
    pipe.venc.endPoint = "live/main_stream";
    mPipelines.push_back(pipe);

    /* vin[0:0]->ivps[1]->joint */
    memset(&pipe, 0, sizeof(axpi::axpipe_t));
    pipe.ivps.group = 1;
    pipe.ivps.framerate = mIvpsFPS;
    pipe.ivps.width = mAlgoWidth;
    pipe.ivps.height = mAlgoHeight;
    if (axpi::axpi_get_model_type(mHandler) != axpi::MT_SEG_PPHUMSEG) {
        pipe.ivps.letterBbox = true;
    }
    pipe.ivps.fifoCount = 1;
    pipe.enable = true;
    pipe.pipeId = mStartID++;
    pipe.inType = axpi::AXPIPE_INPUT_VIN;
    if (mAlgoInit && mHandler) {
        switch (axpi::axpi_get_color_space(mHandler)) {
            case axpi::AXPI_COLOR_SPACE_RGB:
                pipe.outType = axpi::AXPIPE_OUTPUT_BUFF_RGB;
                break;

            case axpi::AXPI_COLOR_SPACE_BGR:
                pipe.outType = axpi::AXPIPE_OUTPUT_BUFF_BGR;
                break;

            case axpi::AXPI_COLOR_SPACE_NV12:
            default:
                pipe.outType = axpi::AXPIPE_OUTPUT_BUFF_NV12;
                break;
        }
    } else {
        pipe.enable = false;
    }
    pipe.bThreadQuit = false;
    pipe.vinPipe = 0;
    pipe.vinChn = 0;
    pipe.frameCallback = inferenceProcess;
    pipe.userData = this;
    pipe.userData2 = NULL;
    mPipelines.push_back(pipe);

    /* vin[0:0]->ivps[2]->vo */
    memset(&pipe, 0, sizeof(axpi::axpipe_t));
    pipe.ivps.group = 2;
    pipe.ivps.rotate = mVoRotateAngle;
    pipe.ivps.framerate = mIvpsFPS;
    pipe.ivps.width = ((mVoRotateAngle == 90) || (mVoRotateAngle == 270)) ? mScreenHeight : mScreenWidth;
    pipe.ivps.height = ((mVoRotateAngle == 90) || (mVoRotateAngle == 270)) ? mScreenWidth : mScreenHeight;
    pipe.ivps.regions = 1;
    pipe.enable = true;
    pipe.pipeId = mStartID++;
    pipe.inType = axpi::AXPIPE_INPUT_VIN;
    pipe.outType = axpi::AXPIPE_OUTPUT_VO_SIPEED_SCREEN;
    pipe.vo.channel = 0;
    pipe.vo.dispDevStr = "dsi0@480x854@60";
    pipe.vinChn = 0;
    pipe.vinPipe = 0;
    mPipelines.push_back(pipe);

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        axpi::axpipe_create(&mPipelines[i]);
        if (mPipelines[i].ivps.regions > 0) {
            mPipesNeedOSD.push_back(&mPipelines[i]);
        }
    }

    if ((mPipesNeedOSD.size() > 0) && mAlgoInit) {
        mOsdThreadId = std::thread(std::bind(&EMSDemo::osdProcessThread, this));
    }

    return 0;
}

int EMSDemo::pipelineRelease()
{
    mQuitThread = true;
    if ((mPipesNeedOSD.size() > 0) && mAlgoInit) {
        if (mOsdThreadId.joinable()) {
            mOsdThreadId.join();
        }
    }

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        axpi::axpipe_release(&mPipelines[i]);
    }

    mPipelines.clear();
    return 0;
}
}
