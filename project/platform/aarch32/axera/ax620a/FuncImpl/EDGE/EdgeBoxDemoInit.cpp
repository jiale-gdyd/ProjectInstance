#include "EdgeBoxDemo.hpp"

namespace edge {
int EdgeBoxDemo::pipelineCreate()
{
    for (size_t i = 0; i < mRtspURL.size(); ++i) {
        auto &pipelines = mPipelines[i];
        pipelines.resize(MAX_PIPELINE);
        memset(pipelines.data(), 0, sizeof(axpi::axpipe_t) * MAX_PIPELINE);

        {
            axpi::axpipe_t &pipe1 = pipelines[1];
            pipe1.ivps.group = MAX_PIPELINE * i + 1;
            pipe1.ivps.framerate = mIvpsFPS;
            pipe1.ivps.width = mAlgoWidth;
            pipe1.ivps.height = mAlgoHeight;
            if (axpi::axpi_get_model_type(mHandler) != axpi::MT_SEG_PPHUMSEG) {
                pipe1.ivps.letterBbox = true;
            }
            pipe1.ivps.fifoCount = 1;
            pipe1.enable = mAlgoInit;
            pipe1.pipeId = MAX_PIPELINE * i + 1;
            pipe1.inType = axpi::AXPIPE_INPUT_VDEC_H264;
            if (mHandler && mAlgoInit) {
                switch (axpi::axpi_get_color_space(mHandler)) {
                    case AX_FORMAT_RGB888:
                        pipe1.outType = axpi::AXPIPE_OUTPUT_BUFF_RGB;
                        break;

                    case AX_FORMAT_BGR888:
                        pipe1.outType = axpi::AXPIPE_OUTPUT_BUFF_BGR;
                        break;

                    case AX_YUV420_SEMIPLANAR:
                        pipe1.outType = axpi::AXPIPE_OUTPUT_BUFF_NV12;
                        break;
                }
            } else {
                pipe1.enable = false;
            }
            pipe1.bThreadQuit = false;
            pipe1.vdec.channel = i;
            pipe1.frameCallback = inferenceProcess;
            pipe1.userData = this;
            pipe1.userData2 = INT_TO_POINTER(i);

            axpi::axpipe_t &pipe2 = pipelines[0];
            pipe2.ivps.group = MAX_PIPELINE * i + 2;
            pipe2.ivps.rotate = 0;
            pipe2.ivps.framerate = mCamFPS;
            pipe2.ivps.width = mAlgoWidth;
            pipe2.ivps.height = mAlgoHeight;
            pipe2.ivps.regions = pipe1.enable ? 1 : 0;
            pipe2.enable = true;
            pipe2.pipeId = MAX_PIPELINE * i + 2;
            pipe2.inType = axpi::AXPIPE_INPUT_VDEC_H264;
            pipe2.outType = axpi::AXPIPE_OUTPUT_RTSP_H264;
            pipe2.bThreadQuit = false;
            pipe2.venc.endPoint = "live/stream" + std::to_string(i);
            pipe2.venc.channel = i;
            pipe2.vdec.channel = i;
        }
    }

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        auto &pipelines = mPipelines[i];
        for (size_t j = 0; j < pipelines.size(); ++j) {
            axpi::axpipe_create(&pipelines[j]);
            if (pipelines[j].ivps.regions > 0) {
                mPipesNeedOSD[i].push_back(&pipelines[j]);
            }
        }

        if (mPipesNeedOSD[i].size() && mAlgoInit) {
            mOsdThreadId[i] = std::thread(std::bind(&EdgeBoxDemo::osdProcessThread, this, i));
        }
    }

    return 0;
}

int EdgeBoxDemo::pipelineRelease()
{
    axpi::axpipe_buffer_t buff = {0};
    for (size_t i = 0; i < mRtspURL.size(); ++i) {
        auto &pipelines = mPipelines[i];
        axpi::axpipe_user_input(pipelines.data(), 1, &buff);
    }

    for (size_t i = 0; i < mRtspURL.size(); ++i) {
        if (mOsdThreadId[i].joinable()) {
            mOsdThreadId[i].join();
        }
    }

    for (size_t i = 0; i < mPipelines.size(); ++i) {
        auto &pipelines = mPipelines[i];
        for (size_t j = 0; j < pipelines.size(); ++j) {
            axpi::axpipe_release(&pipelines[i]);
        }
    }

    return 0;
}
}
