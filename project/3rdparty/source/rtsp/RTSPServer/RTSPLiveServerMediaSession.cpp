#include <live555/groupsock/GroupsockHelper.hh>
#include "RTSPLiveServerMediaSession.hpp"

namespace rtsp {
RtspLiveServerMediaSession *RtspLiveServerMediaSession::createNew(UsageEnvironment &env, bool reuseFirstSource, bool isH264)
{
    return new RtspLiveServerMediaSession(env, reuseFirstSource, isH264);
}

RtspLiveServerMediaSession::RtspLiveServerMediaSession(UsageEnvironment &env, bool reuseFirstSource, bool isH264)
    :OnDemandServerMediaSubsession(env, reuseFirstSource),
    mH264(isH264), mAuxSDPLine(NULL), mDoneFlag(0), mDummySink(NULL), mSource(NULL)
{
    pthread_spin_init(&mLock, 0);
}

RtspLiveServerMediaSession::~RtspLiveServerMediaSession(void)
{
    delete[] mAuxSDPLine;
    pthread_spin_destroy(&mLock);
}

static void afterPlayingDummy(void *clientData)
{
    RtspLiveServerMediaSession *session = (RtspLiveServerMediaSession *)clientData;
    session->afterPlayingDummy1();
}

void RtspLiveServerMediaSession::afterPlayingDummy1()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    RtspLiveServerMediaSession *session = (RtspLiveServerMediaSession *)clientData;
    session->checkForAuxSDPLine1();
}

void RtspLiveServerMediaSession::checkForAuxSDPLine1()
{
    char const *dasl;

    if (mAuxSDPLine != NULL) {
        setDoneFlag();
    } else if((mDummySink != NULL) && ((dasl = mDummySink->auxSDPLine()) != NULL)) {
        mAuxSDPLine = strDup(dasl);
        mDummySink = NULL;
        setDoneFlag();
    } else {
        int uSecsDelay = 100000;
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsDelay, (TaskFunc *)checkForAuxSDPLine, this);
    }
}

char const *RtspLiveServerMediaSession::getAuxSDPLine(RTPSink *rtpSink, FramedSource *inputSource)
{
    if (mAuxSDPLine != NULL) {
        return mAuxSDPLine;
    }

    if (mDummySink == NULL) {
        mDummySink = rtpSink;
        mDummySink->startPlaying(*inputSource, afterPlayingDummy, this);
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&mDoneFlag);
    return mAuxSDPLine;
}

FramedSource *RtspLiveServerMediaSession::createNewStreamSource(uint32_t clientSessionID, uint32_t &estBitRate)
{
    RtspFramedSource *source = RtspFramedSource::createNew(envir());

    pthread_spin_lock(&mLock);
    mSource = source;
    pthread_spin_unlock(&mLock);

    if (mH264) {
        return H264VideoStreamFramer::createNew(envir(),source);
    } else {
        return H265VideoStreamFramer::createNew(envir(),source);
    }
}

void RtspLiveServerMediaSession::closeStreamSource(FramedSource *inputSource)
{
    pthread_spin_lock(&mLock);
    mSource = NULL;
    Medium::close(inputSource);
    pthread_spin_unlock(&mLock);
}

char const *RtspLiveServerMediaSession::sdpLines(int addressFamily)
{
    return OnDemandServerMediaSubsession::sdpLines(addressFamily);
}

RTPSink *RtspLiveServerMediaSession::createNewRTPSink(Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource)
{
    increaseSendBufferTo(envir(), rtpGroupsock->socketNum(), 500 * 1024);
    if (mH264) {
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    } else {
        return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    }
}

void RtspLiveServerMediaSession::SendNalu(uint8_t channel, const unsigned char *buff, uint32_t size, uint64_t timestamp, bool bIFrame)
{
    pthread_spin_lock(&mLock);
    if (mSource) {
        mSource->addFrameBuff(channel, buff, size, timestamp, bIFrame);
    }
    pthread_spin_unlock(&mLock);
}
}
