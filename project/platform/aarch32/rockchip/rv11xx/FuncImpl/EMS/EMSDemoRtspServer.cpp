#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::rtspServerInit()
{
#if defined(CONFIG_RTSP_SERVER)
    pRtspServer = rtsp::rtsp_new_server(mEMSConfig.videoRtspServerParam.serverPort);
    if (pRtspServer == NULL) {
        return -1;
    }

    mRtspSession = rtsp::rtsp_new_session(pRtspServer, mEMSConfig.videoRtspServerParam.streamPath.c_str(), mVideoVencType == DRM_CODEC_TYPE_H264 ? 0 : 1);
    if (mRtspSession < 0) {
        return -2;
    }
#endif

    return 0;
}

void EMSDemoImpl::rtspServerDeinit()
{
    if (mRtspSession > 0) {
        rtsp::rtsp_release_session(pRtspServer, mRtspSession);
    }

    if (pRtspServer) {
        rtsp::rtsp_release_server(&pRtspServer);
    }
}

void EMSDemoImpl::rtspEncodeProcessCallback(media_buffer_t mediaFrame, void *user_data)
{
    static EMSDemoImpl *me = reinterpret_cast<EMSDemoImpl *>(user_data);

    if (mediaFrame) {
        if (me->pRtspServer) {
#if defined(CONFIG_RTSP_SERVER)
            rtsp::rtsp_buff_t buff = {0};
            buff.vsize = me->getApi()->getSys().getFrameSize(mediaFrame);
            buff.vbuff = me->getApi()->getSys().getFrameData(mediaFrame);
            buff.vtimstamp = me->getApi()->getSys().getFrameTimestamp(mediaFrame);

            rtsp::rtsp_push(me->pRtspServer, me->mRtspSession, &buff);
#endif
        }

        me->getApi()->getSys().releaseFrame(mediaFrame);
    }
}

API_END_NAMESPACE(EMS)
