#include "EdgeBoxDemo.hpp"

namespace edge {
int EdgeBoxDemo::rtspClientCreate()
{
    int okCount = 0;

    for (size_t i = 0; i < mRtspURL.size(); ++i) {
        auto &pipeline = mPipelines[i];
        rtsp::RTSPClient *rtspClient = new rtsp::RTSPClient();
        if (rtspClient) {
            if (rtspClient->openURL(mRtspURL[i].c_str(), 1, 2) == 0) {
                if (rtspClient->playURL(rtspClientFrameHandler, pipeline.data(), mRtspURL[i].c_str(), NULL, NULL) == 0) {
                    mRtspClient.push_back(rtspClient);
                    okCount++;
                }
            }
        }
    }

    return okCount > 0 ? 0 : -1;
}

int EdgeBoxDemo::rtspClientRelease()
{
    for (size_t i = 0; i < mRtspURL.size(); ++i) {
        rtsp::RTSPClient *rtspClient = mRtspClient[i];
        if (rtspClient) {
            rtspClient->closeURL();
            delete rtspClient;
        }
    }

    return 0;
}

void EdgeBoxDemo::rtspClientFrameHandler(void *arg, const char *tag, int frameType, int64_t timestamp, uint8_t *frame, uint32_t frameSize)
{
    axpi::axpipe_buffer_t buff;
    axpi::axpipe_t *pipe = (axpi::axpipe_t *)arg;

    switch (frameType) {
        case rtsp::FRAME_TYPE_VIDEO:
            buff.virAddr = frame;
            buff.size = frameSize;
            axpi::axpipe_user_input(pipe, 1, &buff);
            printf("tag:[%s]\n", tag);
            break;

        default:
            break;
    }
}
}
