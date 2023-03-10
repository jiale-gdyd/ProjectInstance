#include <cstdio>
#include <chrono>
#include <sys/time.h>

#include "H264Source.h"

namespace xop {
H264Source::H264Source(uint32_t framerate) : framerate_(framerate)
{
    payload_ = 96;
    media_type_ = H264;
    clock_rate_ = 90000;
}

H264Source *H264Source::CreateNew(uint32_t framerate)
{
    return new H264Source(framerate);
}

H264Source::~H264Source()
{

}

std::string H264Source::GetMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port);
    return std::string(buf);
}

std::string H264Source::GetAttribute()
{
    return std::string("a=rtpmap:96 H264/90000");
}

bool H264Source::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
    uint32_t frame_size = frame.size;
    uint8_t *frame_buf = frame.buffer.get();

    if (frame.timestamp == 0) {
        GetTimestamp(&frame.timeNow, &frame.timestamp);
    }

    if (frame_size <= MAX_RTP_PAYLOAD_SIZE) {
        uint32_t start_code = 0;
        if ((frame_buf[0] == 0) && (frame_buf[1] == 0) && (frame_buf[2] == 1)) {
            start_code = 3;
        } else if ((frame_buf[0] == 0) && (frame_buf[1] == 0) && (frame_buf[2] == 0) && (frame_buf[3] == 1)) {
            start_code = 4;
        }

        frame_buf += start_code;
        frame_size -= start_code;

        RtpPacket rtp_pkt;

        rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
        rtp_pkt.timestamp = frame.timestamp;
        rtp_pkt.timeNow = frame.timeNow;
        rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + frame_size;
        rtp_pkt.last = 1;

        memcpy(rtp_pkt.data.get() + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE, frame_buf, frame_size);
        if (m_pThis && m_pCallMember) {
            (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
        }

        rtp_pkt.data.reset();
    } else {
        uint32_t start_code = 0;
        if ((frame_buf[0] == 0) && (frame_buf[1] == 0) && (frame_buf[2] == 1)) {
            start_code = 3;
        } else if ((frame_buf[0] == 0) && (frame_buf[1] == 0) && (frame_buf[2] == 0) && (frame_buf[3] == 1)) {
            start_code = 4;
        }

        frame_buf += start_code;
        frame_size -= start_code;

        char FU_A[2] = {0};
        FU_A[0] = (frame_buf[0] & 0xe0) | 28;
        FU_A[1] = 0x80 | (frame_buf[0] & 0x1f);

        frame_buf  += 1;
        frame_size -= 1;

        while ((frame_size + 2) > MAX_RTP_PAYLOAD_SIZE) {
            RtpPacket rtp_pkt;

            rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
            rtp_pkt.timestamp = frame.timestamp;
            rtp_pkt.timeNow = frame.timeNow;
            rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
            rtp_pkt.last = 0;

            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = FU_A[0];
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = FU_A[1];
            memcpy(rtp_pkt.data.get()+RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2, frame_buf, MAX_RTP_PAYLOAD_SIZE - 2);
            if (m_pThis && m_pCallMember) {
                (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
            }

            rtp_pkt.data.reset();
            frame_buf += (MAX_RTP_PAYLOAD_SIZE - 2);
            frame_size -= (MAX_RTP_PAYLOAD_SIZE - 2);

            FU_A[1] &= ~0x80;
        }

        {
            RtpPacket rtp_pkt;

            rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
            rtp_pkt.timestamp = frame.timestamp;
            rtp_pkt.timeNow = frame.timeNow;
            rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2 + frame_size;
            rtp_pkt.last = 1;

            FU_A[1] |= 0x40;
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = FU_A[0];
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = FU_A[1];
            memcpy(rtp_pkt.data.get() + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2, frame_buf, frame_size);
            if (m_pThis && m_pCallMember) {
                (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
            }

            rtp_pkt.data.reset();
        }
    }

    return true;
}

uint32_t H264Source::GetTimestamp(struct timeval *tv, uint32_t *ts)
{
    uint32_t timestamp;

    gettimeofday(tv, NULL);
    timestamp = (tv->tv_sec + tv->tv_usec / 1000000.0) * 90000 + 0.5;
    *ts = htonl(timestamp);

    return *ts;
}
}
