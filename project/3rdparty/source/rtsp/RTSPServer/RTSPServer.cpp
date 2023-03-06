#include "RTSPServer.hpp"
#include "RTSPFramedSource.hpp"
#include <live555/groupsock/GroupsockHelper.hh>

namespace rtsp {
#define BASE_PORT       18888

static char gStopEventLoop = 0;
static char const *inputFilename = "live_stream";

// #define MULTICAST

template<typename ... Args>
static std::string string_format(const std::string &format, Args ... args)
{
    size_t size = 1 + snprintf(nullptr, 0, format.c_str(), args ...);
    char bytes[size];
    snprintf(bytes, size, format.c_str(), args ...);
    return std::string(bytes);
}

#ifdef MULTICAST
RTPSink *videoSink;
UsageEnvironment *uEnv;
H264VideoStreamFramer *videoSource;

void afterPlaying(void */* clientData */)
{
    videoSink->stopPlaying();
    Medium::close(videoSource);
    // play();
}

void play()
{
    RtspFramedSource *source = RtspFramedSource::createNew(*uEnv);
    FramedSource *videoES = source;

    videoSource = H264VideoStreamFramer::createNew(*uEnv, videoES);
    videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
}
#endif

void *RtspServerThreadFunc(void *args)
{
    RtspServer *pThis = (RtspServer *)args;

#ifdef MULTICAST
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    uEnv = BasicUsageEnvironment::createNew(*scheduler);

    struct sockaddr_storage destinationAddress;
    struct sockaddr_in *addr = (struct sockaddr_in *)&destinationAddress;
    addr->sin_addr.s_addr = inet_addr("10.126.11.244");
    const unsigned short rtpPortNum = pThis->mBasePort;
    const unsigned short rtcpPortNum = pThis->mBasePort + 1;
    const unsigned char ttl = 255;

    const Port rtpPort(rtpPortNum);
    const Port rtcpPort(rtcpPortNum);

    Groupsock rtpGroupsock(*uEnv, destinationAddress, rtpPort, ttl);
    rtpGroupsock.multicastSendOnly();
    Groupsock rtcpGroupsock(*uEnv, destinationAddress, rtcpPort, ttl);
    rtcpGroupsock.multicastSendOnly();

    OutPacketBuffer::maxSize = 200000;
    videoSink = H264VideoRTPSink::createNew(*uEnv, &rtpGroupsock, 96);

    const unsigned estimatedSessionBandwidth = 500;
    const unsigned maxCNAMElen = 100;
    unsigned char CNAME[maxCNAMElen + 1];
    gethostname((char *)CNAME, maxCNAMElen);
    CNAME[maxCNAMElen] = '\0';

    RTCPInstance *rtcp = RTCPInstance::createNew(*uEnv, &rtcpGroupsock, estimatedSessionBandwidth, CNAME, videoSink, NULL);

    RTSPServer *rtspServer = RTSPServer::createNew(*uEnv, 8554);
    ServerMediaSession *sms = ServerMediaSession::createNew(*uEnv, inputFilename, "test.h264", "Session streamed by \"jiale-gdyd\"");
    sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink, rtcp));
    rtspServer->addServerMediaSession(sms);

    char *url = rtspServer->rtspURL(sms);
    delete[] url;

    play();

    uEnv->taskScheduler().doEventLoop();
#else
    OutPacketBuffer::maxSize = 700000;
    TaskScheduler *taskSchedular = BasicTaskScheduler::createNew();

    pThis->mUEnv = BasicUsageEnvironment::createNew(*taskSchedular);
    pThis->mRtspServer = RTSPServer::createNew(*pThis->mUEnv, 8554, NULL);
    if(pThis->mRtspServer == NULL) {
        *pThis->mUEnv << "Failed to create rtsp server ::" << pThis->mUEnv->getResultMsg() <<"\n";
        return nullptr;
    }

    uint32_t nRTSPIndex = 0;
    for (size_t i = 0; i < 3; i++) {
        std::string strStream = string_format("live_stream%d", nRTSPIndex++);
        ServerMediaSession *sms = ServerMediaSession::createNew(*pThis->mUEnv, strStream.c_str(), strStream.c_str(), "Live Stream");
        pThis->mLiveServerMediaSession[i] = RtspLiveServerMediaSession::createNew(*pThis->mUEnv, true, pThis->mH264);
        sms->addSubsession(pThis->mLiveServerMediaSession[i]);
        pThis->mSessions[strStream] = sms;
        pThis->mRtspServer->addServerMediaSession(sms);

        char *url = pThis->mRtspServer->rtspURL(sms);
        delete[] url;
    }

    taskSchedular->doEventLoop(&gStopEventLoop);
    delete(taskSchedular);
#endif

    return nullptr;
}

RtspServer::RtspServer(void)
{
    mH264 = true;
    mBasePort = BASE_PORT;
    mServerTid = 0;
    for (int i = 0; i < 3; i++) {
        mLiveServerMediaSession[i] = NULL;
    }

    mUEnv = NULL;
    mRtspServer = nullptr;
}

RtspServer::~RtspServer(void)
{

}

bool RtspServer::init(bool isH264, uint16_t uBasePort)
{
    mH264 = isH264;
    if (uBasePort == 0) {
        uBasePort = BASE_PORT;
    }

    mBasePort = uBasePort;
    gStopEventLoop = 0;
    return true;
}

void RtspServer::SendNalu(uint8_t channel, const unsigned char *buff, uint32_t size, uint64_t timestamp, bool bIFrame)
{
#ifdef MULTICAST
    if (videoSource) {
        ((RtspFramedSource *)videoSource->inputSource())->addFrameBuff(channel, buff, size, timestamp, bIFrame);
    }
#else
    if (mLiveServerMediaSession[channel]) {
        mLiveServerMediaSession[channel]->SendNalu(channel, buff, size, timestamp, bIFrame);
    }
#endif
}

bool RtspServer::start(void)
{
    if (0 != pthread_create(&mServerTid, NULL, RtspServerThreadFunc, this)) {
        mServerTid = 0;
        return false;
    }

    return true;
}

void RtspServer::stop(void)
{
    uint32_t nRTSPIndex = 0;

    for (size_t i = 0; i < 3; i++) {
        std::string strStream = string_format("live_stream%d", nRTSPIndex++);
        ServerMediaSession *sms = mSessions[strStream];
        if (sms) {
            mRtspServer->removeServerMediaSession(sms);
            mRtspServer->closeAllClientSessionsForServerMediaSession(sms);
            sms->deleteAllSubsessions();
        }

        mLiveServerMediaSession[i] = NULL;
    }

    mUEnv = NULL;
    mSessions.clear();
    gStopEventLoop = 1;
}
}
