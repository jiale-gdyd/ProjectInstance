#include <cstdio>
#include <chrono>
#include <cstdlib>
#include <sys/time.h>

#include "AACSource.h"

namespace xop {
AACSource::AACSource(uint32_t samplerate, uint32_t channels, bool has_adts)
    : samplerate_(samplerate), channels_(channels), has_adts_(has_adts)
{
    payload_ = 97;
    media_type_ = AAC;
    clock_rate_ = samplerate;
}

AACSource *AACSource::CreateNew(uint32_t samplerate, uint32_t channels, bool has_adts)
{
    return new AACSource(samplerate, channels, has_adts);
}

AACSource::~AACSource()
{

}

std::string AACSource::GetMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=audio %hu RTP/AVP 97", port);
    return std::string(buf);
}

static uint32_t AACSampleRate[16] = {
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};

std::string AACSource::GetAttribute()
{
    char buf[500] = { 0 };
    sprintf(buf, "a=rtpmap:97 MPEG4-GENERIC/%u/%u\r\n", samplerate_, channels_);

    uint8_t index = 0;
    for (index = 0; index < 16; index++) {
        if (AACSampleRate[index] == samplerate_) {
            break;
        }
    }

    if (index == 16) {
        return "";
    }

    uint8_t profile = 1;
    char config[10] = {0};

    sprintf(config, "%02x%02x", (uint8_t)((profile + 1) << 3)|(index >> 1), (uint8_t)((index << 7)|(channels_<< 3)));
    sprintf(buf+strlen(buf),
            "a=fmtp:97 profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizlength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
            atoi(config));

    return std::string(buf);
}

bool AACSource::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
    uint32_t frame_size = frame.size;
    uint8_t *frame_buf = frame.buffer.get();

    if (frame.timestamp == 0) {
        GetTimestamp(&frame.timeNow, &frame.timestamp);
    }

    int adts_size = 0;
    if (has_adts_) {
        adts_size = ADTS_SIZE;
    }

    frame_buf += adts_size;
    frame_size -= adts_size;

    char AU[AU_SIZE] = { 0 };
    AU[0] = 0x00;
    AU[1] = 0x10;
    AU[2] = (frame_size & 0x1fe0) >> 5;
    AU[3] = (frame_size & 0x1f) << 3;

    while ((frame_size + AU_SIZE) > MAX_RTP_PAYLOAD_SIZE){
        RtpPacket rtp_pkt;

        rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
        rtp_pkt.timestamp = frame.timestamp;
        rtp_pkt.timeNow = frame.timeNow;
        rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
        rtp_pkt.last = 0;

        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = AU[0];
        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = AU[1];
        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2] = AU[2];
        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 3] = AU[3];

        memcpy(rtp_pkt.data.get() + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + AU_SIZE, frame_buf, MAX_RTP_PAYLOAD_SIZE - AU_SIZE);
        if (m_pThis && m_pCallMember) {
            (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
        }

        rtp_pkt.data.reset();

        frame_buf += (MAX_RTP_PAYLOAD_SIZE - AU_SIZE);
        frame_size -= (MAX_RTP_PAYLOAD_SIZE - AU_SIZE);
    }

    {
        RtpPacket rtp_pkt;

        rtp_pkt.data = std::shared_ptr<uint8_t>(new uint8_t[RTP_PACKET_SIZE]);
        rtp_pkt.timestamp = frame.timestamp;
        rtp_pkt.timeNow = frame.timeNow;
        rtp_pkt.size = RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + AU_SIZE + frame_size;
        rtp_pkt.last = 1;

        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = AU[0];
        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = AU[1];
        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2] = AU[2];
        rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 3] = AU[3];

        memcpy(rtp_pkt.data.get()+RTP_TCP_HEAD_SIZE+RTP_HEADER_SIZE+AU_SIZE, frame_buf, frame_size);
        if (m_pThis && m_pCallMember) {
            (m_pThis->*m_pCallMember)(channel_id, rtp_pkt);
        }

        rtp_pkt.data.reset();
    }

    return true;
}

uint32_t AACSource::GetTimestamp(struct timeval *tv, uint32_t *ts)
{
    uint32_t timestamp;

    gettimeofday(tv, NULL);
    timestamp = (tv->tv_sec + tv->tv_usec / 1000000.0) * 44100 + 0.5;
    *ts = htonl(timestamp);

    return *ts;
}
}
