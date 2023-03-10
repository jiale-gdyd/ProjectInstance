#include <thread>
#include <memory.h>
#include <rtsp/rtspServerWrapper.hpp>

#include "net/Timer.h"
#include "xop/RtspServer.h"

namespace rtsp {
class RtspServerWarpper {
public:
    RtspServerWarpper(uint16_t port) {
        mLoopExit = 0;
        mPort = port;
        std::shared_ptr<std::thread> _thread(new std::thread(&RtspServerWarpper::Start, port, std::ref(mServer), &mLoopExit));

        usleep(500 * 1000);
        mThread = _thread;
        mRtspUrl = "rtsp://127.0.0.1:" + std::to_string(port);
    }

    ~RtspServerWarpper() {
        // Quit();
    }

    void Quit() {
        mLoopExit = 1;
        if (mThread.get()) {
            mThread->join();
            mThread = nullptr;
            mServer = nullptr;
        }
    }

    xop::MediaSessionId AddSession(std::string url_suffix, bool h265) {
        xop::MediaSession *session = xop::MediaSession::CreateNew(url_suffix);
        if (h265) {
            session->AddSource(xop::channel_0, xop::H265Source::CreateNew());
        } else {
            session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
        }

        session->AddNotifyConnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client connect, session Id:[%d], ip:[%s], port:[%hu]\n", sessionId, peer_ip.c_str(), peer_port);
        });

        session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client disconnect, session id:[%d], ip:[%s], port:[%hu]\n", sessionId, peer_ip.c_str(), peer_port);
        });

        xop::MediaSessionId session_id = mServer->AddSession(session);

        std::cout << "Play URL: " << mRtspUrl << "/" << url_suffix << "   sessionID:" << session_id << std::endl;
        return session_id;
    }

    void RemoveSession(xop::MediaSessionId sessionID) {
        mServer->RemoveSession(sessionID);
    }

    bool PushFrame(xop::MediaSessionId session_id, xop::MediaChannelId channel_id, xop::AVFrame &videoFrame) {
        return mServer->PushFrame(session_id, channel_id, videoFrame);
    }

private:
    static void Start(int port, std::shared_ptr<xop::RtspServer> &server, volatile int *loopExit) {
        std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
        server = xop::RtspServer::Create(event_loop.get());

        if (!server->Start("0.0.0.0", port)) {
            printf("RTSP Server listen on port:[%d] failed\n", port);
            return;
        }

        while (!*loopExit) {
            xop::Timer::Sleep(100);
        }

        server->Stop();
        event_loop->Quit();
    }

private:
    std::shared_ptr<xop::RtspServer> mServer;
    std::string                      mRtspUrl;
    std::shared_ptr<std::thread>     mThread;

    int                              mPort = 8554;
    volatile int                     mLoopExit = 0;
};

rtsp_server_t rtsp_new_server(uint16_t port)
{
    RtspServerWarpper *rtsp_wapper = new RtspServerWarpper(port);
    return rtsp_wapper;
}

void rtsp_release_server(rtsp_server_t *rtsp_server)
{
    if (rtsp_server && *rtsp_server) {
        RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)*rtsp_server;
        rtsp_wapper->Quit();

        delete rtsp_wapper;
        rtsp_wapper = nullptr;
        *rtsp_server = nullptr;
    }
}

rtsp_session_t rtsp_new_session(rtsp_server_t rtsp_server, const char *url_suffix, bool bH265)
{
    RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
    if (!rtsp_wapper) {
        return -1;
    }

    xop::MediaSessionId session_id = rtsp_wapper->AddSession(url_suffix, bH265);
    return session_id;
}

void rtsp_release_session(rtsp_server_t rtsp_server, rtsp_session_t rtsp_session)
{
    RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
    if (!rtsp_wapper) {
        return;
    }

    rtsp_wapper->RemoveSession(rtsp_session);
}

int rtsp_push(rtsp_server_t rtsp_server, rtsp_session_t rtsp_session, rtsp_buff_t *buff)
{
    if (buff->vsize > 0) {
        RtspServerWarpper *rtsp_wapper = (RtspServerWarpper *)rtsp_server;
        if (!rtsp_wapper) {
            return -1;
        }

        struct timeval tv;
        uint32_t timestamp = 0;
        xop::AVFrame videoFrame = {0};
        timestamp = xop::H264Source::GetTimestamp(&tv, &timestamp);

        videoFrame.size = buff->vsize;
        videoFrame.timestamp = buff->vtimstamp == 0 ? timestamp : buff->vtimstamp;
        videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
        memcpy(videoFrame.buffer.get(), buff->vbuff, videoFrame.size);

        bool ret = rtsp_wapper->PushFrame(rtsp_session, xop::channel_0, videoFrame);
        return ret ? 0 : -1;
    }

    return -1;
}
}
