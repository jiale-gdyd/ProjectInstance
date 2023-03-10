#ifndef XOP_G711A_SOURCE_H
#define XOP_G711A_SOURCE_H

#include "rtp.h"
#include "MediaSource.h"

namespace xop {
class G711ASource : public MediaSource {
public:
    static G711ASource *CreateNew();
    virtual ~G711ASource();

    uint32_t GetSampleRate() const {
        return samplerate_;
    }

    uint32_t GetChannels() const {
        return channels_;
    }

    virtual std::string GetMediaDescription(uint16_t port = 0);
    virtual std::string GetAttribute();

    virtual bool HandleFrame(MediaChannelId channel_id, AVFrame frame);
    static uint32_t GetTimestamp(struct timeval *tv, uint32_t *ts);

private:
    G711ASource();

    uint32_t samplerate_ = 8000;
    uint32_t channels_ = 1;
};
}

#endif
