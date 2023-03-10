#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <rtsp/rtspServerWrapper.hpp>

#include "rkpipe.hpp"
#include "../rkmpi.h"

API_BEGIN_NAMESPACE(mpi)

extern bool checkRtspSessionPipeId(uint32_t pipeId);
extern rtsp::rtsp_server_t getRtspServerHandler();
extern rtsp::rtsp_session_t getRtspServerSession(uint32_t pipeId);

static void *vencGetFrameProcessThread(void *arg)
{
    int ret = 0;
    media_buffer_t mediaFrame = NULL;
    rkpipe_t *pipe = reinterpret_cast<rkpipe_t *>(arg);

    while (!pipe->quitThread) {
        mediaFrame = drm_mpi_system_get_media_buffer(MOD_ID_VENC, pipe->venc.vencChannel, 200);
        if (mediaFrame) {
            if ((pipe->outputType == RKPIPE_OUTPUT_RTSP_H264) || (pipe->outputType == RKPIPE_OUTPUT_RTSP_H265)) {
                if (checkRtspSessionPipeId(pipe->pipelineId)) {
                    rtsp::rtsp_buff_t buff = {0};
                    buff.vbuff = drm_mpi_mb_get_ptr(mediaFrame);
                    buff.vsize = drm_mpi_mb_get_size(mediaFrame);
                    buff.vtimstamp = drm_mpi_mb_get_timestamp(mediaFrame);

                    rtsp::rtsp_push(getRtspServerHandler(), getRtspServerSession(pipe->pipelineId), &buff);
                }
            }

            /* 使用者不能在这个回调函数占用太长时间 */
            if (pipe->callback) {
                rkpipe_buffer_t buffer;
                buffer.id = pipe->pipelineId;
                buffer.size = drm_mpi_mb_get_size(mediaFrame);
                buffer.viraddr = drm_mpi_mb_get_ptr(mediaFrame);
                buffer.timestamp = drm_mpi_mb_get_timestamp(mediaFrame);
                buffer.userdata = pipe;
                pipe->callback(&buffer, pipe->userdata);
            }

            drm_mpi_mb_release_buffer(mediaFrame);
        } else {
            usleep(10000);
        }
    }

    return NULL;
}

int rkpipe_create_venc(rkpipe_t *pipe)
{
    drm_venc_chn_attr_t stVencChnAttr;
    memset(&stVencChnAttr, 0x00, sizeof(stVencChnAttr));

    drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264;
    if (pipe->venc.vencCodecType == RKPIPE_OUTPUT_VENC_MJPG) {
        enCodecType = DRM_CODEC_TYPE_MJPEG;
    } else if (pipe->venc.vencCodecType == RKPIPE_OUTPUT_VENC_H265) {
        enCodecType = DRM_CODEC_TYPE_H265;
    }

    switch (enCodecType) {
        case DRM_CODEC_TYPE_MJPEG:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_MJPEG;
            stVencChnAttr.stRcAttr.enRcMode = (drm_venc_rc_mode_e)pipe->venc.vencRcMode;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32BitRate = pipe->venc.vencBitrate;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = pipe->venc.vencFrameRate;
            stVencChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = pipe->venc.vencFrameRate;
            break;

        case DRM_CODEC_TYPE_H265:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_H265;
            stVencChnAttr.stRcAttr.enRcMode = (drm_venc_rc_mode_e)pipe->venc.vencRcMode;
            stVencChnAttr.stRcAttr.stH265Cbr.u32Gop = pipe->venc.vencIFrameInternal;
            stVencChnAttr.stRcAttr.stH265Cbr.u32BitRate = pipe->venc.vencBitrate;
            stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = pipe->venc.vencFrameRate;
            stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = pipe->venc.vencFrameRate;
            break;

        case DRM_CODEC_TYPE_H264:
        default:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_H264;
            stVencChnAttr.stRcAttr.enRcMode = (drm_venc_rc_mode_e)pipe->venc.vencRcMode;
            stVencChnAttr.stRcAttr.stH264Cbr.u32Gop = pipe->venc.vencIFrameInternal;
            stVencChnAttr.stRcAttr.stH264Cbr.u32BitRate = pipe->venc.vencBitrate;
            stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = pipe->venc.vencFrameRate;
            stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = pipe->venc.vencFrameRate;
            break;
    }

    stVencChnAttr.stVencAttr.u32Profile = pipe->venc.vencProfile;
    stVencChnAttr.stVencAttr.imageType = DRM_IMAGE_TYPE_NV12;
    stVencChnAttr.stVencAttr.u32PicWidth = pipe->venc.vencWidth;
    stVencChnAttr.stVencAttr.u32VirWidth = pipe->venc.vencWidth;
    stVencChnAttr.stVencAttr.u32PicHeight = pipe->venc.vencHeight;
    stVencChnAttr.stVencAttr.u32VirHeight = pipe->venc.vencHeight;

    int ret = drm_mpi_venc_create_channel(pipe->venc.vencChannel, &stVencChnAttr);
    if (ret) {
        rkmpi_error("create vecn channel:[%d] failed, return:[%d]", pipe->venc.vencChannel, ret);
        return ret;
    }

    int rotation = VENC_ROTATION_0;
    if (pipe->venc.vencRotateAngle == 90) {
        rotation = VENC_ROTATION_90;
    } else if (pipe->venc.vencRotateAngle == 180) {
        rotation = VENC_ROTATION_180;
    } else if (pipe->venc.vencRotateAngle == 270) {
        rotation = VENC_ROTATION_270;
    }

    ret = drm_mpi_venc_set_rotation(pipe->venc.vencChannel, (drm_venc_rotation_e)rotation);
    if (ret) {
        rkmpi_warn("set vecn channel:[%d] rotation:[%d] failed, return:[%d]", pipe->venc.vencChannel, rotation, ret);
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_VENC, pipe->venc.vencChannel, pipe->venc.bufDepth);
    if (ret) {
        rkmpi_warn("vecn channel:[%d] set buff depth failed, return:[%d]", pipe->venc.vencChannel, ret);
    }

    pthread_create(&pipe->venc.threadId, NULL, vencGetFrameProcessThread, pipe);
    return 0;
}

int rkpipe_release_venc(rkpipe_t *pipe)
{
    pthread_join(pipe->venc.threadId, NULL);
    return drm_mpi_venc_destroy_channel(pipe->venc.vencChannel);
}

API_END_NAMESPACE(mpi)
