#include "EMSDemo.hpp"

namespace EMS {
int EMSDemo::pipelineCreate()
{
    mPipelines.clear();

    /* vin[0:0]->ivps[0]->venc[0]->rtsp */
    axpi::axpipe_t pipe;
    memset(&pipe, 0, sizeof(axpi::axpipe_t));
    pipe.ivpsConfig.ivpsGroup = 0;
    pipe.ivpsConfig.ivpsFramerate = mCamFPS;
    pipe.ivpsConfig.ivpsWidth = 1920;
    pipe.ivpsConfig.ivpsHeight = 1080;
    pipe.ivpsConfig.osdRegions = 1;
    pipe.bEnable = true;
    pipe.pipeId = mStartID++;
    pipe.inType = axpi::AXPIPE_INPUT_VIN;
    pipe.outType = axpi::AXPIPE_OUTPUT_RTSP_H264;
    pipe.bThreadQuit = false;
    pipe.vinChn = 0;
    pipe.vinPipe = 0;
    pipe.vencConfig.vencChannel = 0;
    pipe.vencConfig.rtspPort = 8554;
    pipe.vencConfig.endPoint = "/live/main_stream";
    mPipelines.push_back(pipe);

    /* vin[0:0]->ivps[1]->joint */
    memset(&pipe, 0, sizeof(axpi::axpipe_t));
    pipe.ivpsConfig.ivpsGroup = 1;
    pipe.ivpsConfig.ivpsFramerate = mIvpsFPS;
    pipe.ivpsConfig.ivpsWidth = mAlgoWidth;
    pipe.ivpsConfig.ivpsHeight = mAlgoHeight;
    if (axpi::axpi_get_model_type(mHandler) != axpi::MT_SEG_PPHUMSEG) {
        pipe.ivpsConfig.bLetterBbox = true;
    }
    pipe.ivpsConfig.fifoCount = 1;
    pipe.bEnable = true;
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
        pipe.bEnable = false;
    }
    pipe.bThreadQuit = false;
    pipe.vinPipe = 0;
    pipe.vinChn = 0;
    pipe.frameCallback = inferenceProcess;
    pipe.userData = this;
    mPipelines.push_back(pipe);

    /* vin[0:0]->ivps[2]->vo */
    memset(&pipe, 0, sizeof(axpi::axpipe_t));
    pipe.ivpsConfig.ivpsGroup = 2;
    pipe.ivpsConfig.ivpsRotate = mVoRotateAngle;
    pipe.ivpsConfig.ivpsFramerate = mIvpsFPS;
    pipe.ivpsConfig.ivpsWidth = ((mVoRotateAngle == 90) || (mVoRotateAngle == 270)) ? mScreenHeight : mScreenWidth;
    pipe.ivpsConfig.ivpsHeight = ((mVoRotateAngle == 90) || (mVoRotateAngle == 270)) ? mScreenWidth : mScreenHeight;
    pipe.ivpsConfig.osdRegions = 1;
    pipe.bEnable = true;
    pipe.pipeId = mStartID++;
    pipe.inType = axpi::AXPIPE_INPUT_VIN;
    pipe.outType = axpi::AXPIPE_OUTPUT_VO_SIPEED_SCREEN;
    pipe.voConfig.dispChannel = 0;
    pipe.voConfig.dispDevStr = "dsi0@480x854@60";
    pipe.vinChn = 0;
    pipe.vinPipe = 0;
    mPipelines.push_back(pipe);

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        axpi::axpipe_create(&mPipelines[i]);
        if (mPipelines[i].ivpsConfig.osdRegions > 0) {
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
