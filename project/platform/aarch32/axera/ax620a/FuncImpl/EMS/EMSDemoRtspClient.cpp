#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::RtspFrameHandler(void *arg, const char *tag, int frameType, int64_t timestamp, uint8_t *frame, uint32_t frameSize)
{
    media::axpipe_buffer_t h264;
    media::axpipe_t *pipe = (media::axpipe_t *)arg;

    switch (frameType) {
        case rtsp::FRAME_TYPE_VIDEO:
            h264.virAddr = frame;
            h264.size = frameSize;
            media::axpipe_user_input(pipe, 1, &h264);
            break;

        default:
            break;
    }
}

API_END_NAMESPACE(EMS)
