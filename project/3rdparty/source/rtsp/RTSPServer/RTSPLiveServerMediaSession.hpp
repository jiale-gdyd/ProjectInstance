#ifndef RTSP_RTSPSERVER_RTSPLIVESERVERMEDIASESSION_HPP
#define RTSP_RTSPSERVER_RTSPLIVESERVERMEDIASESSION_HPP

#include <queue>
#include <pthread.h>
#include <live555/liveMedia/liveMedia.hh>
#include <live555/liveMedia/OnDemandServerMediaSubsession.hh>

#include "RTSPFramedSource.hpp"

namespace rtsp {
class RtspLiveServerMediaSession: public OnDemandServerMediaSubsession {
public:
    static RtspLiveServerMediaSession *createNew(UsageEnvironment &env, bool reuseFirstSource, bool isH264 = true);

    void checkForAuxSDPLine1();
    void afterPlayingDummy1();

    void SendNalu(uint8_t channel, const unsigned char *buff, uint32_t size, uint64_t timestamp = 0, bool bIFrame = false);

protected:
    RtspLiveServerMediaSession(UsageEnvironment &env, bool reuseFirstSource, bool isH264);
    virtual ~RtspLiveServerMediaSession(void);

    void setDoneFlag() {
        mDoneFlag = ~0;
    }

protected:
    virtual char const *getAuxSDPLine(RTPSink *rtpSink, FramedSource *inputSource);

    virtual FramedSource *createNewStreamSource(uint32_t clientSessionId, uint32_t &estBitrate);
    virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, uint8_t rtpPayloadTypeIfDynamic, FramedSource *inputSource);

    virtual void closeStreamSource(FramedSource *inputSource);
    virtual char const *sdpLines(int addressFamily = AF_INET);

private:
    bool               mH264;
    char               *mAuxSDPLine;
    char               mDoneFlag;
    RTPSink            *mDummySink;
    RtspFramedSource   *mSource;
    pthread_spinlock_t mLock;
};
}

#endif
