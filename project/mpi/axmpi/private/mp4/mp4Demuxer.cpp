#include <cstdio>
#include <vector>
#include <thread>
#include <string>
#include <cstdint>
#include <cstdlib>

#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"
#include "mp4Demuxer.hpp"

namespace axpi {

typedef struct {
    uint8_t *buffer;
    ssize_t size;
} input_buffer_t;

typedef struct {
    std::shared_ptr<uint8_t> buff_h264;
    ssize_t                  h264_size;

    std::vector<uint8_t>     spspps_buffer;

    std::string              path;
    axpipe_mp4_frame_t       callback;
    void                     *user_data;
    void                     *reserved;
    std::thread              threadId;

    bool                     loop_play;
    bool                     loop_exit = false;
} mp4_demuxer_handler_t;

static uint8_t *preload(const char *path, ssize_t *data_size)
{
    uint8_t *data;
    FILE *file = fopen(path, "rb");

    *data_size = 0;
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END)) {
        return NULL;
    }

    *data_size = (ssize_t)ftell(file);
    if (*data_size < 0) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET)) {
        return NULL;
    }

    data = (unsigned char *)malloc(*data_size);
    if (!data) {
        return NULL;
    }

    if ((ssize_t)fread(data, 1, *data_size, file) != *data_size) {
        return NULL;
    }

    fclose(file);
    return data;
}

static int read_callback(int64_t offset, void *buffer, size_t size, void *token)
{
    input_buffer_t *buf = (input_buffer_t *)token;
    size_t to_copy = MINIMP4_MIN(size, buf->size - offset - size);
    memcpy(buffer, buf->buffer + offset, to_copy);

    return (to_copy != size);
}

void mp4_demuxer_thread(mp4_demuxer_handler_t *handle)
{
    do {
        handle->buff_h264.reset(preload(handle->path.c_str(), &handle->h264_size), std::default_delete<uint8_t[]>());

        int ntrack = 0;
        const void *spspps;
        int i, spspps_bytes;
        MP4D_demux_t mp4 = {0};
        input_buffer_t buf = {handle->buff_h264.get(), handle->h264_size};

        MP4D_open(&mp4, read_callback, &buf, handle->h264_size);

        unsigned sum_duration = 0;
        MP4D_track_t *tr = mp4.track + ntrack;

        i = 0;
        if (tr->handler_type == MP4D_HANDLER_TYPE_VIDE) {
#define USE_SHORT_SYNC 0
            char sync[4] = {0, 0, 0, 1};
            while ((spspps = MP4D_read_sps(&mp4, ntrack, i, &spspps_bytes)) && !handle->loop_exit) {
                if (handle->callback) {
                    if ((spspps_bytes + 4 - USE_SHORT_SYNC) > handle->spspps_buffer.size()) {
                        handle->spspps_buffer.resize(spspps_bytes + 4 - USE_SHORT_SYNC);
                    }

                    memcpy(handle->spspps_buffer.data(), sync + USE_SHORT_SYNC, 4 - USE_SHORT_SYNC);
                    memcpy(handle->spspps_buffer.data() + 4 - USE_SHORT_SYNC, spspps, spspps_bytes);
                    handle->callback(handle->spspps_buffer.data(), spspps_bytes + 4 - USE_SHORT_SYNC, FRAME_VIDEO, handle->user_data, handle->reserved);
                }

                i++;
            }

            i = 0;
            while ((spspps = MP4D_read_pps(&mp4, ntrack, i, &spspps_bytes)) && !handle->loop_exit) {
                if (handle->callback) {
                    if ((spspps_bytes + 4 - USE_SHORT_SYNC) > handle->spspps_buffer.size()) {
                        handle->spspps_buffer.resize(spspps_bytes + 4 - USE_SHORT_SYNC);
                    }

                    memcpy(handle->spspps_buffer.data(), sync + USE_SHORT_SYNC, 4 - USE_SHORT_SYNC);
                    memcpy(handle->spspps_buffer.data() + 4 - USE_SHORT_SYNC, spspps, spspps_bytes);
                    handle->callback(handle->spspps_buffer.data(), spspps_bytes + 4 - USE_SHORT_SYNC, FRAME_VIDEO, handle->user_data, handle->reserved);
                }

                i++;
            }

            for (i = 0; (i < mp4.track[ntrack].sample_count) && !handle->loop_exit; i++) {
                unsigned frame_bytes, timestamp, duration;

                MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
                uint8_t *mem = handle->buff_h264.get() + ofs;
                sum_duration += duration;

                while (frame_bytes) {
                    uint32_t size = ((uint32_t)mem[0] << 24) | ((uint32_t)mem[1] << 16) | ((uint32_t)mem[2] << 8) | mem[3];
                    size += 4;
                    mem[0] = 0;
                    mem[1] = 0;
                    mem[2] = 0;
                    mem[3] = 1;

                    if (handle->callback) {
                        handle->callback(mem + USE_SHORT_SYNC, size - USE_SHORT_SYNC, FRAME_VIDEO, handle->user_data, handle->reserved);
                    }

                    if (frame_bytes < size) {
                        printf("demux sample failed");
                        return;
                    }

                    frame_bytes -= size;
                    mem += size;
                }
            }
        }

        MP4D_close(&mp4);
    } while (handle->loop_play && !handle->loop_exit);

    if (handle->callback) {
        handle->callback(NULL, 0, FRAME_VIDEO, handle->user_data, handle->reserved);
    }
}

axpipe_mp4_handler_t axpipe_mp4_open(const char *filename, bool bLoopPlay, axpipe_mp4_frame_t callback, void *user_data, void *reserved)
{
    mp4_demuxer_handler_t *handle = new mp4_demuxer_handler_t;
    handle->path = filename;
    handle->loop_play = bLoopPlay;
    handle->callback = callback;
    handle->threadId = std::thread(mp4_demuxer_thread, handle);
    handle->user_data = user_data;
    handle->reserved = reserved;
    return handle;
}

void axpipe_mp4_close(axpipe_mp4_handler_t *handler)
{
    mp4_demuxer_handler_t *handle = (mp4_demuxer_handler_t *)handler;
    if (handle) {
        handle->loop_exit = true;
        if (handle->threadId.joinable()) {
            handle->threadId.join();
        }

        delete handle;
        handle = nullptr;
        *handler = nullptr;
    }
}
}
