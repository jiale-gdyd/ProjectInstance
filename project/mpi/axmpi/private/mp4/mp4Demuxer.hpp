#pragma once

#include <functional>

namespace axpi {
enum {
    FRAME_VIDEO,
    FRAME_AUDIO,
};

typedef void* axpipe_mp4_handler_t;
using axpipe_mp4_frame_t = std::function<int (const void *buff, int frame_size, int frame_type, void *user_data, void *reserved)>;

void axpipe_mp4_close(axpipe_mp4_handler_t *handler);
axpipe_mp4_handler_t axpipe_mp4_open(const char *filename, bool bLoopPlay, axpipe_mp4_frame_t callback, void *user_data, void *reserved);
}
