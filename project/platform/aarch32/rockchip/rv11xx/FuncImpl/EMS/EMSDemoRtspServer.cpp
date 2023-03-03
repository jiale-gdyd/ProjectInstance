#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::rtspServerInit()
{
#if defined(CONFIG_SIMPLE_RTSP_SERVER)
    pSimpleServer = std::make_shared<rtsp::SimpleRTSPServer>();
    if (pSimpleServer == nullptr) {
        return -1;
    }

    if (pSimpleServer->init(mEMSConfig.videoRtspServerParam.streamPath,
        mEMSConfig.videoRtspServerParam.serverPort, mVideoVencType == DRM_CODEC_TYPE_H264 ? 0 : 1))
    {
        return -2;
    }
#endif

    return 0;
}

void EMSDemoImpl::rtspEncodeProcessCallback(media_buffer_t mediaFrame, void *user_data)
{
    static EMSDemoImpl *me = reinterpret_cast<EMSDemoImpl *>(user_data);

    if (mediaFrame) {
        if (me->pSimpleServer) {
#if defined(CONFIG_SIMPLE_RTSP_SERVER)
            uint32_t dataSize = me->getApi()->getSys().getFrameSize(mediaFrame);
            uint64_t timestamp = me->getApi()->getSys().getFrameTimestamp(mediaFrame);
            const uint8_t *data = (const uint8_t *)me->getApi()->getSys().getFrameData(mediaFrame);

            me->pSimpleServer->sendFrame(data, dataSize, timestamp);
#endif
        }

        me->getApi()->getSys().releaseFrame(mediaFrame);
    }
}

API_END_NAMESPACE(EMS)
