#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::rtspClientInit()
{
#if defined(CONFIG_RTSP_SERVER_CLIENT)
    pRTSPClient = std::make_shared<rtsp::RTSPClient>();
    if (!pRTSPClient) {
        return -1;
    }

    std::string rtspURL = "rtsp://admin:a123456@192.168.1.100";
    if (pRTSPClient->openURL(rtspURL.c_str(), 1, 2, false) != 0) {
        return -2;
    }

    if (pRTSPClient->playURL(rtspClientFrameHandler, this, "tag", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != 0) {
        return -3;
    }
#endif

    return 0;
}

void EMSDemoImpl::rtspClientFrameHandler(void *arg, const char *tag, int frameType, int64_t timestamp, uint8_t *frame, uint32_t frameSize)
{
    EMSDemoImpl *me = reinterpret_cast<EMSDemoImpl *>(arg);

    switch (frameType) {
        case rtsp::FRAME_TYPE_VIDEO: {
            /* 这里可以将数据发送到解码器进行解码 */
            if (frameSize > 0) {
                printf("[video]: tag:[%s], recv from rtsp client frame size:[%d]\n", tag, frameSize);
            }

            break;
        }

        default:
            break;
    }
}

API_END_NAMESPACE(EMS)
