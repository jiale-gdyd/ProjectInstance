#ifndef RTSP_RTSPSERVER_RTSPFRAMEDSOURCE_HPP
#define RTSP_RTSPSERVER_RTSPFRAMEDSOURCE_HPP

#include <queue>
#include <live555/liveMedia/FramedSource.hh>

#include "RtspRingBuffer.hpp"

namespace rtsp {
typedef struct {
    uint8_t  *buff;
    uint32_t *size;
} frame_info_t;

class RtspFramedSource : public FramedSource {
public:
    static RtspFramedSource *createNew(UsageEnvironment &env);

public:
    static EventTriggerId eventTriggerId;

    void addFrameBuff(uint8_t channel, const unsigned char *buff, uint32_t size, uint64_t timestamp = 0, bool bIFrame = false);
    virtual uint32_t maxFrameSize() const;

protected:
    RtspFramedSource(UsageEnvironment &env);
    virtual ~RtspFramedSource();

private:
    virtual void doGetNextFrame();

private:
    static void deliverFrame(void *clientData);
    void deliverFrame();

private:
    static uint32_t          mRefCount;
    std::queue<frame_info_t> mFrame;
    RTSPRingBuffer           *mRingBuff;
    uint32_t                 mTriggerId;
};
}

#endif
