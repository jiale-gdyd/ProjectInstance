#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaRga.hpp"
#include "../private.h"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaRga::MediaRga()
{
    for (int i = DRM_RGA_CHANNEL_00; i < DRM_RGA_CHANNEL_BUTT; ++i) {
        mRgaChnStarted[i] = false;
    }
}

MediaRga::~MediaRga()
{
    for (int i = DRM_RGA_CHANNEL_00; i < DRM_RGA_CHANNEL_BUTT; ++i) {
        destroyRgaChn(i);
    }
}

bool MediaRga::rgaChnStart(int rgaChn)
{
    if ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) {
        mpi_error("invalid rga channel:[%d]", rgaChn);
        return false;
    }

    return mRgaChnStarted[rgaChn];
}

int MediaRga::destroyRgaChn(int rgaChn)
{
    if ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) {
        return -1;
    }

    if (mRgaChnStarted[rgaChn] == false) {
        return 0;
    }

    int ret = drm_mpi_rga_destroy_channel(rgaChn);
    if (ret) {
        return ret;
    }

    mRgaChnStarted[rgaChn] = false;
    return 0;
}

int MediaRga::createRgaChn(int rgaChn, int bufPoolCnt, int bufDepth, drm_image_type_e enInPixFmt, drm_image_type_e enOutPixFmt, int rotation, drm_rga_flip_e enFlip, bool bWH2HW, size_t inWidth, size_t inHeight, size_t inXoffset, size_t inYoffset, size_t outWidth, size_t outHeight, size_t outXoffset, size_t outYoffset)
{
    if ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) {
        mpi_error("invalid rga channel:[%d]", rgaChn);
        return -1;
    }

    if (mRgaChnStarted[rgaChn] == true) {
        mpi_error("rga channel:[%d] had started", rgaChn);
        return 0;
    }

    int ret = 0;
    drm_rga_attr_t stRgaAttr;

    memset(&stRgaAttr, 0x00, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = true;
    stRgaAttr.enFlip = enFlip;
    stRgaAttr.u16Rotaion = rotation;
    stRgaAttr.u16BufPoolCnt = bufPoolCnt;
    stRgaAttr.stImgIn.u32X = inXoffset;
    stRgaAttr.stImgIn.u32Y = inYoffset;
    stRgaAttr.stImgIn.imgType = enInPixFmt;
    stRgaAttr.stImgIn.u32Width = inWidth;
    stRgaAttr.stImgIn.u32Height = inHeight;
    stRgaAttr.stImgIn.u32HorStride = inWidth;
    stRgaAttr.stImgIn.u32VirStride = inHeight;
    stRgaAttr.stImgOut.u32X = outXoffset;
    stRgaAttr.stImgOut.u32Y = outYoffset;
    stRgaAttr.stImgOut.imgType = enOutPixFmt;
    stRgaAttr.stImgOut.u32Width = bWH2HW ? outHeight : outWidth;
    stRgaAttr.stImgOut.u32Height = bWH2HW ? outWidth : outHeight;
    stRgaAttr.stImgOut.u32HorStride = bWH2HW ? outHeight : outWidth;
    stRgaAttr.stImgOut.u32VirStride = bWH2HW ? outWidth : outHeight;

    ret = drm_mpi_rga_create_channel(rgaChn, &stRgaAttr);
    if (ret) {
        mpi_error("create rga channel:[%d] failed, return:[%d]", rgaChn, ret);
        return ret;
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_RGA, rgaChn, bufDepth);
    if (ret) {
        mpi_error("rga channel:[%d] set buff depth failed, return:[%d]", rgaChn, ret);
    }

    mRgaChnStarted[rgaChn] = true;
    return 0;
}

int MediaRga::setRgaChnFrameRate(int rgaChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen)
{
    if ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) {
        mpi_error("invalid rga channel:[%d]", rgaChn);
        return -1;
    }

    drm_fps_attr_t fpsAttr;
    memset(&fpsAttr, 0x00, sizeof(fpsAttr));
    fpsAttr.s32FpsInNum = fpsInNum;
    fpsAttr.s32FpsInDen = fpsInDen;
    fpsAttr.s32FpsOutNum = fpsOutNum;
    fpsAttr.s32FpsOutDen = fpsOutDen;

    return drm_mpi_system_set_framerate(MOD_ID_RGA, rgaChn, &fpsAttr);
}

int MediaRga::getRgaChnAttr(int rgaChn, drm_rga_attr_t *pstRgaAttr)
{
    if ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) {
        mpi_error("invalid rga channel:[%d]", rgaChn);
        return -1;
    }

    std::lock_guard<std::mutex> lock(mRgaAttrMutex[rgaChn]);
    return drm_mpi_rga_get_channel_attribute(rgaChn, pstRgaAttr);
}

int MediaRga::setRgnChnAttr(int rgaChn, int rotation, drm_rga_flip_e enFlip, bool bWH2HW, size_t inWidth, size_t inHeight, size_t inXoffset, size_t inYoffset, size_t outWidth, size_t outHeight, size_t outXoffset, size_t outYoffset)
{
    if ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) {
        mpi_error("invalid rga channel:[%d]", rgaChn);
        return -1;
    }

    std::lock_guard<std::mutex> lock(mRgaAttrMutex[rgaChn]);

    if (mRgaChnStarted[rgaChn] == false) {
        return -1;
    }

    drm_rga_attr_t stRgaAttr;
    memset(&stRgaAttr, 0x00, sizeof(stRgaAttr));
    stRgaAttr.enFlip = enFlip;
    stRgaAttr.u16Rotaion = rotation;
    stRgaAttr.stImgIn.u32X = inXoffset;
    stRgaAttr.stImgIn.u32Y = inYoffset;
    stRgaAttr.stImgIn.u32Width = inWidth;
    stRgaAttr.stImgIn.u32Height = inHeight;
    stRgaAttr.stImgIn.u32HorStride = inWidth;
    stRgaAttr.stImgIn.u32VirStride = inHeight;
    stRgaAttr.stImgOut.u32X = outXoffset;
    stRgaAttr.stImgOut.u32Y = outYoffset;
    stRgaAttr.stImgOut.u32Width = bWH2HW ? outHeight : outWidth;
    stRgaAttr.stImgOut.u32Height = bWH2HW ? outWidth : outHeight;
    stRgaAttr.stImgOut.u32HorStride = bWH2HW ? outHeight : outWidth;
    stRgaAttr.stImgOut.u32VirStride = bWH2HW ? outWidth : outHeight;

    return drm_mpi_rga_set_channel_attribute(rgaChn, (const drm_rga_attr_t *)&stRgaAttr);
}

media_buffer_t MediaRga::getFrame(int modeIdChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, modeIdChn, s32MilliSec);
}

int MediaRga::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

int MediaRga::getFrameFd(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_fd(pstFrameInfo);
}

void *MediaRga::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaRga::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaRga::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

media_buffer_t MediaRga::convertFrame(int modeIdChn, media_buffer_t origMediaFrame, int s32MilliSec, size_t waitConvUs)
{
    drm_mpi_system_send_media_buffer(MOD_ID_RGA, modeIdChn, origMediaFrame);
    usleep(waitConvUs);
    return drm_mpi_system_get_media_buffer(MOD_ID_RGA, modeIdChn, s32MilliSec);
}

int MediaRga::setRgaRgnBitmap(int rgaChn, const drm_osd_region_info_t *pstRgnInfo, const drm_bitmap_t *pstBitmap)
{
    return drm_mpi_rga_region_set_bitmap(rgaChn, pstRgnInfo, pstBitmap);
}

int MediaRga::setRgaRgnCover(int rgaChn, const drm_osd_region_info_t *pstRgnInfo, const drm_cover_info_t *pstCoverInfo)
{
    return drm_mpi_rga_region_set_cover(rgaChn, pstRgnInfo, pstCoverInfo);
}

API_END_NAMESPACE(media)
