#include <cstdio>
#include <chrono>

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include "VP8Source.h"

namespace xop {
VP8Source::VP8Source(uint32_t framerate) : framerate_(framerate)
{
    payload_ = 96;
    clock_rate_ = 90000;
}

VP8Source *VP8Source::CreateNew(uint32_t framerate)
{
    return new VP8Source(framerate);
}

VP8Source::~VP8Source()
{

}

std::string VP8Source::GetMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=video %hu RTP/AVP 96", port);
    return std::string(buf);
}

std::string VP8Source::GetAttribute()
{
    return std::string("a=rtpmap:96 VP8/90000");
}

bool VP8Source::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
    uint32_t frame_size = frame.size;
    uint8_t *frame_buf = frame.buffer.get();

    if (frame.timestamp == 0) {
        frame.timestamp = GetTimestamp(&frame.timeNow, &frame.timestamp);
    }

    uint8_t vp8_payload_descriptor = 0x10;

    while (frame_size > 0) {
        uint32_t payload_size = MAX_RTP_PAYLOAD_SIZE;

        RtpPacket rtp_pkt;

        rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
        rtp_pkt.timestamp = frame.timestamp;
        rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + RTP_VPX_HEAD_SIZE + MAX_RTP_PAYLOAD_SIZE;
        rtp_pkt.last = 0;

        if (frame_size < MAX_RTP_PAYLOAD_SIZE) {
            payload_size = frame_size;
            rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + RTP_VPX_HEAD_SIZE + frame_size;
            rtp_pkt.last = 1;
        }

        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = vp8_payload_descriptor;
        memcpy(rtp_pkt.data.get()+RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + RTP_VPX_HEAD_SIZE, frame_buf, payload_size);
        if (m_pThis && m_pCallMember) {
            (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
        }

        rtp_pkt.data.reset();

        frame_buf += payload_size;
        frame_size -= payload_size;
        vp8_payload_descriptor = 0x00;
    }

    return true;
}

uint32_t VP8Source::GetTimestamp(struct timeval *tv, uint32_t *ts)
{
    uint32_t timestamp;

    gettimeofday(tv, NULL);
    timestamp = (tv->tv_sec + tv->tv_usec / 1000000.0) * 90000 + 0.5;
    *ts = htonl(timestamp);

    return *ts;
}
}
