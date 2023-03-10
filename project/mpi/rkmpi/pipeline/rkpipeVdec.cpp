#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "rkpipe.hpp"
#include "../rkmpi.h"

API_BEGIN_NAMESPACE(mpi)

static void *vdecProcessThread(void *arg)
{
    FILE *fp = NULL;
    size_t data_size = 0;
    size_t read_size = 0;
    drm_video_mode_e enMode = VIDEO_MODE_STREAM;
    drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264;
    rkpipe_t *pipe = reinterpret_cast<rkpipe_t *>(arg);

    if (pipe->inputType == RKPIPE_INPUT_VDEC_H265) {
        enCodecType = DRM_CODEC_TYPE_H265;
    } else {
        enCodecType = DRM_CODEC_TYPE_JPEG;
    }

    if ((enCodecType == DRM_CODEC_TYPE_JPEG) || (enCodecType == DRM_CODEC_TYPE_MJPEG)) {
        enMode = VIDEO_MODE_FRAME;
    }

    fp = fopen(pipe->vdec.vdecFile.c_str(), "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (enMode == VIDEO_MODE_STREAM) {
        data_size = 4096;
    } else if (enMode == VIDEO_MODE_FRAME) {
        fseek(fp, 0, SEEK_END);
        data_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    while (!pipe->quitThread) {
        media_buffer_t mediaFrame = drm_mpi_mb_create_buffer(data_size, false, MB_FLAG_NOCACHED);
VDEC_LOOP:
        if (mediaFrame) {
            read_size = fread(drm_mpi_mb_get_ptr(mediaFrame), 1, data_size, fp);
            if ((read_size == 0) || feof(fp)) {
                if (pipe->vdec.loopVdec) {
                    fseek(fp, 0, SEEK_SET);
                    goto VDEC_LOOP;
                } else {
                    drm_mpi_mb_release_buffer(mediaFrame);
                    break;
                }
            }

            drm_mpi_mb_set_size(mediaFrame, read_size);
            drm_mpi_system_send_media_buffer(MOD_ID_VDEC, pipe->vdec.vdecChannel, mediaFrame);
            drm_mpi_mb_release_buffer(mediaFrame);
        }

        usleep(5000);
    }

    if (fp) {
        fclose(fp);
    }

    return NULL;
}

int rkpipe_create_vdec(rkpipe_t *pipe)
{
    drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264;
    if (pipe->inputType == RKPIPE_INPUT_VDEC_H265) {
        enCodecType = DRM_CODEC_TYPE_H265;
    } else {
        enCodecType = DRM_CODEC_TYPE_JPEG;
    }

    drm_vdec_chn_attr_t stVdecChnAttr;
    memset(&stVdecChnAttr, 0x00, sizeof(stVdecChnAttr));
    stVdecChnAttr.enCodecType = enCodecType;
    stVdecChnAttr.enMode = VIDEO_MODE_STREAM;
    stVdecChnAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
    if ((enCodecType == DRM_CODEC_TYPE_JPEG) || (enCodecType == DRM_CODEC_TYPE_MJPEG)) {
        stVdecChnAttr.enMode = VIDEO_MODE_FRAME;
    }

    int ret = drm_mpi_vdec_create_channel(pipe->vdec.vdecChannel, (const drm_vdec_chn_attr_t *)&stVdecChnAttr);
    if (ret) {
        rkmpi_error("create vdec channel vdec:[%d] failed, return:[%d]", pipe->vdec.vdecChannel, ret);
        return -1;
    }

    if (!pipe->vdec.vdecFile.empty() && (access(pipe->vdec.vdecFile.c_str(), F_OK) == 0)) {
        pthread_create(&pipe->vdec.threadId, NULL, vdecProcessThread, pipe);
    }

    return 0;
}

int rkpipe_release_vdec(rkpipe_t *pipe)
{
    if (!pipe->vdec.vdecFile.empty() && (access(pipe->vdec.vdecFile.c_str(), F_OK) == 0)) {
        pthread_join(pipe->vdec.threadId, NULL);
    }

    return drm_mpi_vdec_destroy_channel(pipe->vdec.vdecChannel);
}

API_END_NAMESPACE(mpi)
