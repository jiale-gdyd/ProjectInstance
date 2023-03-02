#include <stdio.h>
#include <string.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaVenc.hpp"
#include "../private.h"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaVenc::MediaVenc()
{
    for (int i = DRM_VENC_CHANNEL_00; i < DRM_VENC_CHANNEL_BUTT; ++i) {
        mVencChnStarted[i] = false;
    }
}

MediaVenc::~MediaVenc()
{
    for (int i = DRM_VENC_CHANNEL_00; i < DRM_VENC_CHANNEL_BUTT; ++i) {
        destroyVencChn(i);
    }
}

bool MediaVenc::vencChnStart(int vencChn)
{
    if ((vencChn < DRM_VENC_CHANNEL_00) || (vencChn >= DRM_VENC_CHANNEL_BUTT)) {
        mpi_error("invalid vecn channel:[%d]", vencChn);
        return false;
    }

    return mVencChnStarted[vencChn];
}

int MediaVenc::destroyVencChn(int vencChn)
{
    if ((vencChn < DRM_VENC_CHANNEL_00) || (vencChn >= DRM_VENC_CHANNEL_BUTT)) {
        return -1;
    }

    if (mVencChnStarted[vencChn] == false) {
        return 0;
    }

    int ret = drm_mpi_venc_destroy_channel(vencChn);
    if (ret) {
        return ret;
    }

    mVencChnStarted[vencChn] = false;
    return 0;
}

int MediaVenc::createVencChn(int vencChn, drm_venc_chn_attr_t *stVencChnAttr)
{
    if ((vencChn < DRM_VENC_CHANNEL_00) || (vencChn >= DRM_VENC_CHANNEL_BUTT)) {
        mpi_error("invalid vecn channel:[%d]", vencChn);
        return -1;
    }

    if (mVencChnStarted[vencChn] == true) {
        mpi_error("vecn channel:[%d] had started", vencChn);
        return 0;
    }

    int ret = drm_mpi_venc_create_channel(vencChn, stVencChnAttr);
    if (ret) {
        mpi_error("create vecn channel:[%d] failed, return:[%d]", vencChn, ret);
        return ret;
    }

    mVencChnStarted[vencChn] = true;
    return 0;
}

int MediaVenc::createVencChn(int vencChn, int bufDepth, size_t frameWidth, size_t frameHeight, drm_image_type_e imageType, drm_codec_type_e enCodecType, drm_venc_rc_mode_e rcMode, size_t frameRate, size_t profile, OutCallbackFunction callback, size_t bitrate, size_t iFrmInterval, int rotation)
{
    if ((vencChn < DRM_VENC_CHANNEL_00) || (vencChn >= DRM_VENC_CHANNEL_BUTT)) {
        mpi_error("invalid vecn channel:[%d]", vencChn);
        return -1;
    }

    if (mVencChnStarted[vencChn] == true) {
        mpi_error("vecn channel:[%d] had started", vencChn);
        return 0;
    }

    int ret = -1;
    drm_venc_chn_attr_t stVencChnAttr;

    memset(&stVencChnAttr, 0x00, sizeof(stVencChnAttr));
    switch (enCodecType) {
        case DRM_CODEC_TYPE_MJPEG:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_MJPEG;
            stVencChnAttr.stRcAttr.enRcMode = rcMode;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32BitRate = bitrate;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = frameRate;
            stVencChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = frameRate;
            break;

        case DRM_CODEC_TYPE_H265:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_H265;
            stVencChnAttr.stRcAttr.enRcMode = rcMode;
            stVencChnAttr.stRcAttr.stH265Cbr.u32Gop = iFrmInterval;
            stVencChnAttr.stRcAttr.stH265Cbr.u32BitRate = bitrate;
            stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = frameRate;
            stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = frameRate;
            break;

        case DRM_CODEC_TYPE_H264:
        default:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_H264;
            stVencChnAttr.stRcAttr.enRcMode = rcMode;
            stVencChnAttr.stRcAttr.stH264Cbr.u32Gop = iFrmInterval;
            stVencChnAttr.stRcAttr.stH264Cbr.u32BitRate = bitrate;
            stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = frameRate;
            stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = frameRate;
            break;
    }

    stVencChnAttr.stVencAttr.u32Profile = profile;
    stVencChnAttr.stVencAttr.imageType = imageType;
    stVencChnAttr.stVencAttr.u32PicWidth = frameWidth;
    stVencChnAttr.stVencAttr.u32VirWidth = frameWidth;
    stVencChnAttr.stVencAttr.u32PicHeight = frameHeight;
    stVencChnAttr.stVencAttr.u32VirHeight = frameHeight;

    ret = drm_mpi_venc_create_channel(vencChn, &stVencChnAttr);
    if (ret) {
        mpi_error("create vecn channel:[%d] failed, return:[%d]", vencChn, ret);
        return ret;
    }

    ret = drm_mpi_venc_set_rotation(vencChn, (drm_venc_rotation_e)rotation);
    if (ret) {
        mpi_error("set vecn channel:[%d] rotation:[%d] failed, return:[%d]", vencChn, rotation, ret);
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_VENC, vencChn, bufDepth);
    if (ret) {
        mpi_error("vecn channel:[%d] set buff depth failed, return:[%d]", vencChn, ret);
    }

    if (callback) {
        drm_chn_t stEncChn;
        stEncChn.s32DevId = 0;
        stEncChn.s32ChnId = vencChn;
        stEncChn.enModId = MOD_ID_VENC;

        ret = drm_mpi_system_register_output_callback((const drm_chn_t *)&stEncChn, (OutCallbackFunction)callback);
        if (ret) {
            mpi_error("register vecn channel:[%d] output callback failed, return:[%d]", vencChn, ret);
            return ret;
        }
    }

    mVencChnStarted[vencChn] = true;
    return 0;
}

int MediaVenc::createVencChnEx(int vencChn, int bufDepth, size_t frameWidth, size_t frameHeight, drm_image_type_e imageType, drm_codec_type_e enCodecType, drm_venc_rc_mode_e rcMode, size_t frameRate, size_t profile, OutCallbackFunctionEx callback, void *userData, size_t bitrate, size_t iFrmInterval, int rotation)
{
    if ((vencChn < DRM_VENC_CHANNEL_00) || (vencChn >= DRM_VENC_CHANNEL_BUTT)) {
        mpi_error("invalid vecn channel:[%d]", vencChn);
        return -1;
    }

    if (mVencChnStarted[vencChn] == true) {
        mpi_error("vecn channel:[%d] had started", vencChn);
        return 0;
    }

    int ret = -1;
    drm_venc_chn_attr_t stVencChnAttr;

    memset(&stVencChnAttr, 0x00, sizeof(stVencChnAttr));
    switch (enCodecType) {
        case DRM_CODEC_TYPE_MJPEG:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_MJPEG;
            stVencChnAttr.stRcAttr.enRcMode = rcMode;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32BitRate = bitrate;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = frameRate;
            stVencChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = frameRate;
            break;

        case DRM_CODEC_TYPE_H265:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_H265;
            stVencChnAttr.stRcAttr.enRcMode = rcMode;
            stVencChnAttr.stRcAttr.stH265Cbr.u32Gop = iFrmInterval;
            stVencChnAttr.stRcAttr.stH265Cbr.u32BitRate = bitrate;
            stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = frameRate;
            stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = frameRate;
            break;

        case DRM_CODEC_TYPE_H264:
        default:
            stVencChnAttr.stVencAttr.enType = DRM_CODEC_TYPE_H264;
            stVencChnAttr.stRcAttr.enRcMode = rcMode;
            stVencChnAttr.stRcAttr.stH264Cbr.u32Gop = iFrmInterval;
            stVencChnAttr.stRcAttr.stH264Cbr.u32BitRate = bitrate;
            stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
            stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = frameRate;
            stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = frameRate;
            break;
    }

    stVencChnAttr.stVencAttr.u32Profile = profile;
    stVencChnAttr.stVencAttr.imageType = imageType;
    stVencChnAttr.stVencAttr.u32PicWidth = frameWidth;
    stVencChnAttr.stVencAttr.u32VirWidth = frameWidth;
    stVencChnAttr.stVencAttr.u32PicHeight = frameHeight;
    stVencChnAttr.stVencAttr.u32VirHeight = frameHeight;

    ret = drm_mpi_venc_create_channel(vencChn, &stVencChnAttr);
    if (ret) {
        mpi_error("create vecn channel:[%d] failed, return:[%d]", vencChn, ret);
        return ret;
    }

    ret = drm_mpi_venc_set_rotation(vencChn, (drm_venc_rotation_e)rotation);
    if (ret) {
        mpi_error("set vecn channel:[%d] rotation:[%d] failed, return:[%d]", vencChn, rotation, ret);
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_VENC, vencChn, bufDepth);
    if (ret) {
        mpi_error("vecn channel:[%d] set buff depth failed, return:[%d]", vencChn, ret);
    }

    if (callback) {
        drm_chn_t stEncChn;
        stEncChn.s32DevId = 0;
        stEncChn.s32ChnId = vencChn;
        stEncChn.enModId = MOD_ID_VENC;

        ret = drm_mpi_system_register_output_callbackEx((const drm_chn_t *)&stEncChn, (OutCallbackFunctionEx)callback, userData);
        if (ret) {
            mpi_error("register vecn channel:[%d] output callback failed, return:[%d]", vencChn, ret);
            return ret;
        }
    }

    mVencChnStarted[vencChn] = true;
    return 0;
}

int MediaVenc::setVencChnFrameRate(int vencChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen)
{
    if ((vencChn < DRM_VENC_CHANNEL_00) || (vencChn >= DRM_VENC_CHANNEL_BUTT)) {
        return -1;
    }

    drm_fps_attr_t fpsAttr;
    memset(&fpsAttr, 0x00, sizeof(fpsAttr));
    fpsAttr.s32FpsInNum = fpsInNum;
    fpsAttr.s32FpsInDen = fpsInDen;
    fpsAttr.s32FpsOutNum = fpsOutNum;
    fpsAttr.s32FpsOutDen = fpsOutDen;

    return drm_mpi_system_set_framerate(MOD_ID_VENC, vencChn, &fpsAttr);
}

int MediaVenc::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

media_buffer_t MediaVenc::getFrame(int vencChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, vencChn, s32MilliSec);
}

void *MediaVenc::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaVenc::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaVenc::setFrameSize(media_buffer_t pstFrameInfo, unsigned int size)
{
    return drm_mpi_mb_set_size(pstFrameInfo, size);
}

media_buffer_t MediaVenc::createFrame(unsigned int size, bool bHardware, unsigned char flag)
{
    return drm_mpi_mb_create_buffer(size, bHardware, flag);
}

media_buffer_t MediaVenc::createImageFrame(mb_image_info_t *pstImageInfo, bool bHardware, unsigned char flag)
{
    return drm_mpi_mb_create_image_buffer(pstImageInfo, bHardware, flag);
}

int MediaVenc::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

API_END_NAMESPACE(media)
