#include "RTSPFramedSource.hpp"

namespace rtsp {
uint32_t RtspFramedSource::mRefCount = 0;
EventTriggerId RtspFramedSource::eventTriggerId = 0;

RtspFramedSource *RtspFramedSource::createNew(UsageEnvironment &env)
{
    return new RtspFramedSource(env);
}

RtspFramedSource::RtspFramedSource(UsageEnvironment &env) : FramedSource(env)
{
    if (mRefCount == 0) {
        // 设备的任何全局初始化都将在这里完成:
        // %%% TO BE WRITTEN %%%
    }
    ++mRefCount;

    mTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame);

#ifndef RTSP_SERVER_APPLY_QUEUE
    mRingBuff = new RTSPRingBuffer(700000, 2, "RTSP");
#endif
}

RtspFramedSource::~RtspFramedSource()
{
    --mRefCount;

    envir().taskScheduler().deleteEventTrigger(mTriggerId);
    eventTriggerId = 0;

#ifndef RTSP_SERVER_APPLY_QUEUE
    delete(mRingBuff);
#endif
}

void RtspFramedSource::addFrameBuff(uint8_t channel, const unsigned char *buff, uint32_t size, uint64_t timestamp, bool bIFrame)
{
#ifdef RTSP_SERVER_APPLY_QUEUE
    frame_info_t frame;
    frame.buff = (uint8_t *)malloc(size);
    memcpy(frame.buff, buff, size);
    frame.size = size;
    mFrame.push(frame);
#else
    RTSPRingElement ele((uint8_t *)buff, size, channel, timestamp, bIFrame);
    mRingBuff->Put(ele);
#endif

    envir().taskScheduler().triggerEvent(mTriggerId, this);
}

void RtspFramedSource::doGetNextFrame()
{
    deliverFrame();
}

void RtspFramedSource::deliverFrame(void *clientData)
{
    ((RtspFramedSource *)clientData)->deliverFrame();
}

void RtspFramedSource::deliverFrame()
{
    if (!isCurrentlyAwaitingData()) {
        return;
    }

#ifdef RTSP_SERVER_APPLY_QUEUE
    if (mFrame.size() == 0) {
        return;
    }

    frame_info_t frame = mFrame.front();
    mFrame.pop();

    uint8_t *newFrameDataStart = (uint8_t *)frame.buff;
    uint32_t newFrameSize = static_cast<uint32_t>(frame.size);
#else
    RTSPRingElement *element = mRingBuff->Get();
    if (!element) {
        return;
    }

    uint8_t *newFrameDataStart = element->pBuf;
    uint32_t newFrameSize = element->nSize;

    if ((0 == newFrameSize) || (nullptr == newFrameDataStart)) {
        return;
    }
#endif

    // 在这里分发数据
    if (newFrameSize > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = newFrameSize - fMaxSize;
    } else {
        fFrameSize = newFrameSize;
    }

    gettimeofday(&fPresentationTime, NULL);
    memmove(fTo, newFrameDataStart, fFrameSize);

#ifdef RTSP_SERVER_APPLY_QUEUE
    free(frame.buff);
#endif

    mRingBuff->Pop();

    // 在传递数据后，通知读者数据现在可用
    FramedSource::afterGetting(this);
}

uint32_t RtspFramedSource::maxFrameSize() const
{
    return 700000;
}
}
