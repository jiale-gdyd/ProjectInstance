#include <stdio.h>
#include <string.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaVin.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaVin::MediaVin()
{
    for (int i = DRM_VI_CHANNEL_00; i < DRM_VI_CHANNEL_BUTT; ++i) {
        mViChnStarted[i] = false;
    }
}

MediaVin::~MediaVin()
{
    for (int i = DRM_VI_CHANNEL_00; i < DRM_VI_CHANNEL_BUTT; ++i) {
        destroyViChn(i);
    }
}

bool MediaVin::viChnStarted(int vinChn)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        printf("invalid vin channel:[%d]\n", vinChn);
        return false;
    }

    return mViChnStarted[vinChn];
}

int MediaVin::destroyViChn(int vinChn)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        return -1;
    }

    if (mViChnStarted[vinChn] == false) {
        return 0;
    }

    drm_mpi_vi_disable_user_picture(vinChn);
    int ret = drm_mpi_vi_disable_channel(vinChn);
    if (ret) {
        return ret;
    }

    mViChnStarted[vinChn] = false;
    return 0;
}

int MediaVin::createViChn(int vinChn, int bufPoolCnt, int bufDepth, const char *devNode, size_t capWidth, size_t capHeight, drm_image_type_e enPixFmt, bool bAllocWithDMA)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        printf("invalid vin channel:[%d]\n", vinChn);
        return -1;
    }

    if (mViChnStarted[vinChn] == true) {
        printf("vin channel:[%d] had started\n", vinChn);
        return 0;
    }

    int ret = 0;
    drm_vi_chn_attr_t stViAttr;

    memset(&stViAttr, 0x00, sizeof(stViAttr));
    stViAttr.u32BufCnt = bufPoolCnt;
    stViAttr.enPixFmt = enPixFmt;
    stViAttr.u32Width = capWidth;
    stViAttr.u32Height = capHeight;
    stViAttr.pcVideoNode = devNode;
    stViAttr.enWorkMode = DRM_VI_WORK_MODE_NORMAL;
    if (bAllocWithDMA) {
        stViAttr.enBufType = DRM_VI_CHN_BUF_TYPE_DMA;
    } else {
        stViAttr.enBufType = DRM_VI_CHN_BUF_TYPE_MMAP;
    }

    drm_mpi_vi_set_channel_attribute(vinChn, (const drm_vi_chn_attr_t *)&stViAttr);
    if (ret) {
        printf("set vin channel:[%d] attribute failed, return:[%d]\n", vinChn, ret);
        return ret;
    }

    ret = drm_mpi_vi_enable_channel(vinChn);
    if (ret) {
        printf("enable vin channel:[%d] failed, return:[%d]\n", vinChn, ret);
        return ret;
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_VI, vinChn, bufDepth);
    if (ret) {
        printf("vin channel:[%d] set buff depth failed, return:[%d]\n", vinChn, ret);
    }

    mViChnStarted[vinChn] = true;
    return 0;
}

int MediaVin::setViChnUserPic(int vinChn, size_t width, size_t height, void *picData, drm_image_type_e enPixFmt)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        return -1;
    }

    drm_vi_userpic_attr_t attr;
    attr.u32Width = width;
    attr.u32Height = height;
    attr.pvPicPtr = picData;
    attr.enPixFmt = enPixFmt;

    return drm_mpi_vi_set_user_picture(vinChn, &attr);
}

int MediaVin::enableViChnUserPic(int vinChn)
{
    return drm_mpi_vi_enable_user_picture(vinChn);
}

int MediaVin::disableViChnUserPic(int vinChn)
{
    return drm_mpi_vi_disable_user_picture(vinChn);
}

int MediaVin::setViChnFrameRate(int vinChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        return -1;
    }

    drm_fps_attr_t fpsAttr;
    memset(&fpsAttr, 0x00, sizeof(fpsAttr));
    fpsAttr.s32FpsInNum = fpsInNum;
    fpsAttr.s32FpsInDen = fpsInDen;
    fpsAttr.s32FpsOutNum = fpsOutNum;
    fpsAttr.s32FpsOutDen = fpsOutDen;

    return drm_mpi_system_set_framerate(MOD_ID_VI, vinChn, &fpsAttr);
}

int MediaVin::startViChnStream(int vinChn)
{
    return drm_mpi_vi_start_stream(vinChn);
}

int MediaVin::getViChnCaptureStatus(int vinChn)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        printf("invalid vin channel:[%d]\n", vinChn);
        return -1;
    }

    if (mViChnStarted[vinChn] == false) {
        printf("vin channel:[%d] not start\n", vinChn);
        return -1;
    }

    printf("not supported vin channel:[%d] capture status\n", vinChn);
    return -1;
}

int MediaVin::getViChnStatus(int vinChn, size_t timeoutS)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        printf("invalid vin channel:[%d]\n", vinChn);
        return -1;
    }

    if (mViChnStarted[vinChn] == false) {
        printf("vin channel:[%d] not start\n", vinChn);
        return -1;
    }

    int ret = drm_mpi_vi_get_status(vinChn);
    if (ret == 3) {
        // printf("vin channel:[%d] timeout\n", vinChn);
        return timeoutS;
    }

    return ret;
}

int MediaVin::setViRgnBitmap(int vinChn, const drm_osd_region_info_t *pstRgnInfo, const drm_bitmap_t *pstBitmap)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        printf("invalid vin channel:[%d]\n", vinChn);
        return -1;
    }

    return drm_mpi_vi_region_set_bitmap(vinChn, (const drm_osd_region_info_t *)pstRgnInfo, (const drm_bitmap_t *)pstBitmap);
}

int MediaVin::setViRgnCover(int vinChn, const drm_osd_region_info_t *pstRgnInfo, const drm_cover_info_t *pstCoverInfo)
{
    if ((vinChn < DRM_VI_CHANNEL_00) || (vinChn >= DRM_VI_CHANNEL_BUTT)) {
        printf("invalid vin channel:[%d]\n", vinChn);
        return -1;
    }

    return drm_mpi_vi_region_set_cover(vinChn, (const drm_osd_region_info_t *)pstRgnInfo, (const drm_cover_info_t *)pstCoverInfo);
}

media_buffer_t MediaVin::getFrame(int vinChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, vinChn, s32MilliSec);
}

int MediaVin::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

void *MediaVin::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaVin::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaVin::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

API_END_NAMESPACE(media)
