#include <stdio.h>
#include <string.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaVmix.hpp"
#include "../private.h"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaVmix::MediaVmix()
{
    for (int i = DRM_VMIX_DEVICE_00; i < DRM_VMIX_DEVICE_BUTT; ++i) {
        for (int j = DRM_VMIX_CHANNEL_00; j < DRM_VMIX_CHANNEL_BUTT; ++j) {
            mVmixChnStarted[i][j] = false;
        }
    }
}

MediaVmix::~MediaVmix()
{
    for (int i = DRM_VMIX_DEVICE_00; i < DRM_VMIX_DEVICE_BUTT; ++i) {
        for (int j = DRM_VMIX_CHANNEL_00; j < DRM_VMIX_CHANNEL_BUTT; ++j) {
            destroyVmixChn(i, j);
        }
    }
}

int MediaVmix::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

media_buffer_t MediaVmix::getFrame(int modeIdChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, modeIdChn, s32MilliSec);
}

void *MediaVmix::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaVmix::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaVmix::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

bool MediaVmix::vmixChnStart(int vmixDev, int vmixChn)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        return false;
    }

    if ((vmixChn < DRM_VMIX_DEVICE_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT)) {
        return false;
    }

    return mVmixChnStarted[vmixDev][vmixChn];
}

int MediaVmix::createVmixChn(int vmixDev, size_t mixOutFps, size_t mixOutWidth, size_t mixOutHeight, bool bEnableBufPool, size_t bufPoolCnt, drm_image_type_e inImgType, std::vector<vmix_chn_info_t> inInfo, std::vector<vmix_chn_info_t> outInfo, bool bDrawLine, std::vector<vmix_line_info_t> lineInfo)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        mpi_error("invalid vmix device:[%d]", vmixDev);
        return -1;
    }

    if ((inInfo.size() != outInfo.size()) || (inInfo.size() == 0) || (outInfo.size() == 0)) {
        mpi_error("invalid input parameter, inInfo.size:[%u] != outInfo.size:[%u]", inInfo.size(), outInfo.size());
        return -1;
    }

    VMIX_DEV_INFO_S stDevInfo;
    size_t u16ChnCnt = inInfo.size();

    memset(&stDevInfo, 0x00, sizeof(stDevInfo));
    stDevInfo.enImgType = inImgType;
    stDevInfo.u16ChnCnt = 0;
    stDevInfo.u16Fps = mixOutFps;
    stDevInfo.u32ImgWidth = mixOutWidth;
    stDevInfo.u32ImgHeight = mixOutHeight;
    stDevInfo.bEnBufPool = bEnableBufPool;
    stDevInfo.u16BufPoolCnt = bufPoolCnt;

    for (size_t i = 0; i < u16ChnCnt; ++i) {
        stDevInfo.stChnInfo[i].stInRect.s32X = inInfo[i].offsetX;
        stDevInfo.stChnInfo[i].stInRect.s32Y = inInfo[i].offsetY;
        stDevInfo.stChnInfo[i].stInRect.u32Width = inInfo[i].areaWidth;
        stDevInfo.stChnInfo[i].stInRect.u32Height = inInfo[i].areaHeight;

        stDevInfo.stChnInfo[i].stOutRect.s32X = outInfo[i].offsetX;
        stDevInfo.stChnInfo[i].stOutRect.s32Y = outInfo[i].offsetY;
        stDevInfo.stChnInfo[i].stOutRect.u32Width = outInfo[i].areaWidth;
        stDevInfo.stChnInfo[i].stOutRect.u32Height = outInfo[i].areaHeight;

        stDevInfo.stChnInfo[i].u16Rotaion = outInfo[i].rotateAngle;
        stDevInfo.stChnInfo[i].enFlip = (drm_rga_flip_e)outInfo[i].enFlip;

        stDevInfo.u16ChnCnt++;
    }

    int ret = drm_mpi_vmix_create_device(vmixDev, &stDevInfo);
    if (ret) {
        mpi_error("create vmix device:[%d] failed, return:[%d]", vmixDev, ret);
        return -1;
    }

    size_t enableChnCnt = 0;
    for (size_t idx = 0; idx < u16ChnCnt; ++idx) {
        ret = drm_mpi_vmix_enable_channel(vmixDev, idx);
        if (ret) {
            mpi_error("enable vmix device:[%d] channel:[%u] failed, return:[%d]", vmixDev, idx, ret);
            continue;
        }

        enableChnCnt += 1;
        mVmixChnStarted[vmixDev][idx] = true;
    }

    if (bDrawLine && (lineInfo.size() > 0)) {
        drm_vmix_line_info_t stVmLine;
        memset(&stVmLine, 0x00, sizeof(stVmLine));

        stVmLine.u32LineCnt = 0;
        for (size_t idx = 0; idx < lineInfo.size(); ++idx) {
            stVmLine.stLines[stVmLine.u32LineCnt].s32X = lineInfo[idx].offsetX;
            stVmLine.stLines[stVmLine.u32LineCnt].s32Y = lineInfo[idx].offsetY;
            stVmLine.stLines[stVmLine.u32LineCnt].u32Width = lineInfo[idx].width;
            stVmLine.stLines[stVmLine.u32LineCnt].u32Height = lineInfo[idx].height;
            stVmLine.u8Enable[stVmLine.u32LineCnt] = 1;
            stVmLine.u32Color = lineInfo[idx].lineColor;
            stVmLine.u32LineCnt++;
        }

        ret = drm_mpi_vmix_set_layout_line(vmixDev, stDevInfo.u16ChnCnt - 1, &stVmLine);
        if (ret) {
            mpi_error("set vmix device:[%d] line info failed, diable area line, return:[%d]", vmixDev, ret);
        }
    }

    ret = (enableChnCnt > 0 ? 0 : -1);
    if (ret != 0) {
        mpi_error("enable vmix device:[%d] failed, enableChnCnt:[%d], return:[%d]", vmixDev, enableChnCnt, ret);
    }

    return ret;
}

int MediaVmix::destroyVmixChn(int vmixDev, int vmixChn)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        return -1;
    }

    if ((vmixChn < DRM_VMIX_DEVICE_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT)) {
        return -1;
    }

    if (mVmixChnStarted[vmixDev][vmixChn] == false) {
        return 0;
    }

    int ret = drm_mpi_vmix_disable_channel(vmixDev, vmixChn);
    ret |= drm_mpi_vmix_destroy_device(vmixDev);
    if (ret) {
        return ret;
    }

    mVmixChnStarted[vmixDev][vmixChn] = false;
    return 0;
}

int MediaVmix::showVmixChn(int vmixDev, int vmixChn)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        return -1;
    }

    if ((vmixChn < DRM_VMIX_DEVICE_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT)) {
        return -1;
    }

    if (mVmixChnStarted[vmixDev][vmixChn] == false) {
        return -1;
    }

    return drm_mpi_vmix_show_channel(vmixDev, vmixChn);
}

int MediaVmix::hideVmixChn(int vmixDev, int vmixChn)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        return -1;
    }

    if ((vmixChn < DRM_VMIX_DEVICE_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT)) {
        return -1;
    }

    if (mVmixChnStarted[vmixDev][vmixChn] == false) {
        return -1;
    }

    return drm_mpi_vmix_hide_channel(vmixDev, vmixChn);
}

int MediaVmix::setVmixRgnBitmap(int vmixDev, int vmixChn, const drm_osd_region_info_t *pstRgnInfo, const drm_bitmap_t *pstBitmap)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        return -1;
    }

    if ((vmixChn < DRM_VMIX_DEVICE_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT)) {
        return -1;
    }

    return drm_mpi_vmix_region_set_bitmap(vmixDev, vmixChn, pstRgnInfo, pstBitmap);
}

int MediaVmix::setVmixRgnCover(int vmixDev, int vmixChn, const drm_osd_region_info_t *pstRgnInfo, const drm_cover_info_t *pstCoverInfo)
{
    if ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT)) {
        return -1;
    }

    if ((vmixChn < DRM_VMIX_DEVICE_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT)) {
        return -1;
    }

    return drm_mpi_vmix_region_set_cover(vmixDev, vmixChn, pstRgnInfo, pstCoverInfo);
}

API_END_NAMESPACE(media)
