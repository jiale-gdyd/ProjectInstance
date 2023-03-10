#include <cstdio>
#include <chrono>
#include <sys/time.h>

#include "H265Source.h"

namespace xop {
H265Source::H265Source(uint32_t framerate) : framerate_(framerate)
{
    payload_ = 96;
    media_type_ = H265;
    clock_rate_ = 90000;
}

H265Source *H265Source::CreateNew(uint32_t framerate)
{
    return new H265Source(framerate);
}

H265Source::~H265Source()
{

}

std::string H265Source::GetMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port);
    return std::string(buf);
}

std::string H265Source::GetAttribute()
{
    return std::string("a=rtpmap:96 H265/90000");
}

bool H265Source::HandleFrame(MediaChannelId channelId, AVFrame frame)
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
            (m_pThis->*m_pCallMember)(channelId, rtp_pkt);
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

        uint8_t PL_FU[3] = {0};
        uint8_t nalUnitType = (frame_buf[0] & 0x7e) >> 1;
        PL_FU[0] = (frame_buf[0] & 0x81) | (49<<1);
        PL_FU[1] = frame_buf[1];
        PL_FU[2] = 0x80 | nalUnitType;

        frame_buf += 2;
        frame_size -= 2;

        while ((frame_size + 3) > MAX_RTP_PAYLOAD_SIZE) {
            RtpPacket rtp_pkt;

            rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
            rtp_pkt.timestamp = frame.timestamp;
            rtp_pkt.timeNow = frame.timeNow;
            rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
            rtp_pkt.last = 0;

            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = PL_FU[0];
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = PL_FU[1];
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2] = PL_FU[2];

            memcpy(rtp_pkt.data.get() + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 3, frame_buf, MAX_RTP_PAYLOAD_SIZE - 3);
            if (m_pThis && m_pCallMember) {
                (m_pThis->*m_pCallMember)(channelId, rtp_pkt);
            }

            rtp_pkt.data.reset();
            frame_buf  += (MAX_RTP_PAYLOAD_SIZE - 3);
            frame_size -= (MAX_RTP_PAYLOAD_SIZE - 3);

            PL_FU[2] &= ~0x80;
        }

        {
            RtpPacket rtp_pkt;

            rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
            rtp_pkt.timestamp = frame.timestamp;
            rtp_pkt.timeNow = frame.timeNow;
            rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 3 + frame_size;
            rtp_pkt.last = 1;

            PL_FU[2] |= 0x40;
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = PL_FU[0];
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = PL_FU[1];
            rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2] = PL_FU[2];

            memcpy(rtp_pkt.data.get() + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 3, frame_buf, frame_size);
            if (m_pThis && m_pCallMember) {
                (m_pThis->*m_pCallMember)(channelId, rtp_pkt);
            }

            rtp_pkt.data.reset();
        }
    }

    return true;
}

uint32_t H265Source::GetTimestamp(struct timeval *tv, uint32_t *ts)
{
    uint32_t timestamp;

    gettimeofday(tv, NULL);
    timestamp = (tv->tv_sec + tv->tv_usec / 1000000.0) * 90000 + 0.5;
    *ts = htonl(timestamp);

    return *ts;
}
}
