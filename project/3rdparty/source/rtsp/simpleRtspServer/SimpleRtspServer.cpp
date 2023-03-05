#include <rtsp/simpleRtspServer.h>
#include <rtsp/SimpleRtspServer.hpp>

namespace rtsp {
SimpleRTSPServer::SimpleRTSPServer() : mInitialized(false), mRtspServerHandle(NULL), mRtspServerSession(NULL)
{

}

SimpleRTSPServer::~SimpleRTSPServer()
{
    if (mRtspServerHandle) {
        simple_rtsp_delete_handle(mRtspServerHandle);
        mRtspServerHandle = NULL;
    }

    if (mRtspServerSession) {
        simple_rtsp_delete_session(mRtspServerSession);
        mRtspServerSession = NULL;
    }

    mInitialized = false;
}

int SimpleRTSPServer::init(std::string path, uint16_t port, int codecFlag)
{
    mRtspServerHandle = simple_create_rtsp_server(port);
    if (mRtspServerHandle == NULL) {
        return -1;
    }

    mRtspServerSession = simple_create_rtsp_session(mRtspServerHandle, path.c_str(), codecFlag);
    if (mRtspServerSession == NULL) {
        return -2;
    }

    mInitialized = true;
    return 0;
}

int SimpleRTSPServer::sendFrame(const uint8_t *frame, uint32_t frameSize, uint64_t timestamp)
{
    if (!mInitialized) {
        return -1;
    }

    return simple_rtsp_server_send_video(mRtspServerHandle, mRtspServerSession, frame, frameSize, timestamp);
}
}
