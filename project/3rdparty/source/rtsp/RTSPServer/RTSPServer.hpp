#ifndef RTSP_RTSPSERVER_RTSPSERVER_HPP
#define RTSP_RTSPSERVER_RTSPSERVER_HPP

#include <map>
#include <live555/liveMedia/liveMedia.hh>
#include <live555/BasicUsageEnvironment/BasicUsageEnvironment.hh>

#include "RTSPLiveServerMediaSession.hpp"

namespace rtsp {
class RtspServer {
public:
    RtspServer(void);
    virtual ~RtspServer();

public:
    bool init(bool isH264 = true, uint16_t uBasePort = 0);

    bool start(void);
    void stop(void);

    void SendNalu(uint8_t channel, const unsigned char *buff, uint32_t size, uint64_t timestamp = 0, bool bIFrame = false);

public:
    RTSPServer                                  *mRtspServer;
    uint16_t                                    mBasePort;
    RtspLiveServerMediaSession                  *mLiveServerMediaSession[3];
    UsageEnvironment                            *mUEnv;
    bool                                        mH264;
    std::map<std::string, ServerMediaSession *> mSessions;

private:
    pthread_t                                   mServerTid;
};
}

#endif
