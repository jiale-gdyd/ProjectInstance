#include <cstdio>
#include <chrono>
#include <sys/time.h>

#include "G711ASource.h"

namespace xop {
G711ASource::G711ASource()
{
    payload_ = 8;
    media_type_ = PCMA;
    clock_rate_ = 8000;
}

G711ASource *G711ASource::CreateNew()
{
    return new G711ASource();
}

G711ASource::~G711ASource()
{

}

std::string G711ASource::GetMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=audio %hu RTP/AVP 8", port);
    return std::string(buf);
}

std::string G711ASource::GetAttribute()
{
    return std::string("a=rtpmap:8 PCMA/8000/1");
}

bool G711ASource::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
    uint32_t frame_size = frame.size;
    uint8_t *frame_buf = frame.buffer.get();

    if (frame.size > MAX_RTP_PAYLOAD_SIZE) {
        return false;
    }

    if (frame.timestamp == 0) {
        GetTimestamp(&frame.timeNow, &frame.timestamp);
    }

    RtpPacket rtp_pkt;

    rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
    rtp_pkt.timestamp = frame.timestamp;
    rtp_pkt.timeNow = frame.timeNow;
    rtp_pkt.size = frame_size + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE;
    rtp_pkt.last = 1;

    memcpy(rtp_pkt.data.get() + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE, frame_buf, frame_size);
    if (m_pThis && m_pCallMember) {
        (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
    }

    rtp_pkt.data.reset();
    return true;
}

uint32_t G711ASource::GetTimestamp(struct timeval *tv, uint32_t *ts)
{
    uint32_t timestamp;

    gettimeofday(tv, NULL);
    timestamp = (tv->tv_sec + tv->tv_usec / 1000000.0) * 8000 + 0.5;
    *ts = htonl(timestamp);

    return *ts;
}
}
