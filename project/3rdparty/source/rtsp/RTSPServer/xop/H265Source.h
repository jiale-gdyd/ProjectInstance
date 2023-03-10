#ifndef XOP_H265_SOURCE_H
#define XOP_H265_SOURCE_H

#include "rtp.h"
#include "MediaSource.h"

namespace xop {
class H265Source : public MediaSource {
public:
    static H265Source *CreateNew(uint32_t framerate = 25);
    ~H265Source();

    void Setframerate(uint32_t framerate) {
        framerate_ = framerate;
    }

    uint32_t GetFramerate() const {
        return framerate_;
    }

    virtual std::string GetMediaDescription(uint16_t port = 0);
    virtual std::string GetAttribute();

    virtual bool HandleFrame(MediaChannelId channelId, AVFrame frame);
    static uint32_t GetTimestamp(struct timeval *tv, uint32_t *ts);

private:
    H265Source(uint32_t framerate);

    uint32_t framerate_ = 25;
};
}

#endif
