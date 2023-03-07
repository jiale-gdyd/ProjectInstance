#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::mediaInit()
{
    media::axpipe_t pipeline;

    /* rtsp --> ivps --> npu */
    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.ivpsConfig.ivpsGroup = 1;
    pipeline.ivpsConfig.ivpsFramerate = 60;
    pipeline.ivpsConfig.ivpsWidth = mModelWidth;
    pipeline.ivpsConfig.ivpsHeight = mModelHeight;
    if (mModelAlgoType != Ai::MT_SEG_PPHUMSEG) {
        pipeline.ivpsConfig.bLetterBbox = true;
    }
    pipeline.ivpsConfig.fifoCount = 1;
    pipeline.bEnable = true;
    pipeline.pipeId = 0x90015;
    pipeline.inType = media::AXPIPE_INPUT_VIN;
    switch (mModelFormat) {
        case Ai::AXDL_COLOR_SPACE_RGB:
            pipeline.outType = media::AXPIPE_OUTPUT_BUFF_RGB;
            break;

        case Ai::AXDL_COLOR_SPACE_BGR:
            pipeline.outType = media::AXPIPE_OUTPUT_BUFF_BGR;
            break;

        case Ai::AXDL_COLOR_SPACE_NV12:
        default:
            pipeline.outType = media::AXPIPE_OUTPUT_BUFF_NV12;
            break;
    }
    pipeline.bThreadQuit = false;
    pipeline.vdecConfig.vdecGroup = 0;
    pipeline.frameCallback = inferenceProcess;
    pipeline.userData = this;

    /* rtsp --> ivps --> venc --> rtsp */
    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.ivpsConfig.ivpsGroup = 0;
    pipeline.ivpsConfig.ivpsRotate = 0;
    pipeline.ivpsConfig.ivpsFramerate = 25;
    pipeline.ivpsConfig.ivpsWidth = 960;
    pipeline.ivpsConfig.ivpsHeight = 540;
    pipeline.ivpsConfig.osdRegions = 1;
    pipeline.bEnable = true;
    pipeline.pipeId = 0x90016;
    pipeline.inType = media::AXPIPE_INPUT_VDEC_H264;
    pipeline.outType = media::AXPIPE_OUTPUT_RTSP_H264;
    pipeline.bThreadQuit = false;
    pipeline.vencConfig.endPoint = "/live/main_stream";
    pipeline.vencConfig.vencChannel = 0;
    pipeline.vdecConfig.vdecGroup = 0;
    mPipelines.push_back(pipeline);

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        int ret = media::axpipe_create(&mPipelines[i]);
        if (ret != 0) {
            return -1;
        }

        if (mPipelines[i].ivpsConfig.osdRegions > 0) {
            mPipeNeedOsd.push_back(&mPipelines[i]);
        }
    }

    if (mPipeNeedOsd.size() > 0) {
        mOsdThreadId = std::thread(std::bind(&EMSDemoImpl::osdProcessThread, this));
    }

    return 0;
}

void EMSDemoImpl::mediaDeinit()
{
    for (size_t i = 0; i < mPipelines.size(); ++i) {
        media::axpipe_release(&mPipelines[i]);
    }
}

API_END_NAMESPACE(EMS)
