#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::mediaInit()
{
    media::axpipe_t pipeline;

    /* vin --> ivps --> venc --> rtsp */
    pipeline.ivpsConfig.ivpsGroup = 0;
    pipeline.ivpsConfig.ivpsFramerate = 25;
    pipeline.ivpsConfig.ivpsWidth = 1920;
    pipeline.ivpsConfig.ivpsHeight = 1080;
    pipeline.ivpsConfig.osdRegions = 0;
    pipeline.bEnable = true;
    pipeline.pipeId = 0x90015;
    pipeline.inType = media::AXPIPE_INPUT_VIN;
    pipeline.outType = media::AXPIPE_OUTPUT_RTSP_H264;
    pipeline.bThreadQuit = false;
    pipeline.vinChn = 0;
    pipeline.vinPipe = 0;
    pipeline.vencConfig.endPoint = "/live/main_stream";
    pipeline.vencConfig.vencChannel = 0;
    mPipelines.push_back(pipeline);

    /* vin --> ivps --> vo */
    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.ivpsConfig.ivpsGroup = 1;
    pipeline.ivpsConfig.ivpsRotate = 90;
    pipeline.ivpsConfig.ivpsFramerate = 60;
    pipeline.ivpsConfig.ivpsWidth = 854;
    pipeline.ivpsConfig.ivpsHeight = 480;
    pipeline.ivpsConfig.osdRegions = 0;
    pipeline.bEnable = true;
    pipeline.pipeId = 0x90016;
    pipeline.inType = media::AXPIPE_INPUT_VIN;
    pipeline.outType = media::AXPIPE_OUTPUT_VO_SIPEED_SCREEN;
    pipeline.bThreadQuit = false;
    pipeline.vinPipe = 0;
    pipeline.vinChn = 0;
    mPipelines.push_back(pipeline);

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        int ret = media::axpipe_create(&mPipelines[i]);
        if (ret != 0) {
            return -1;
        }
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
