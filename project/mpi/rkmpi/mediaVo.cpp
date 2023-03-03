#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "rkmpi.h"
#include "mediaVo.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaVo::MediaVo()
{
    for (int i = DRM_VO_CHANNEL_00; i < DRM_VO_CHANNEL_BUTT; ++i) {
        mBindRgaVo[i] = false;
        mBindRgaRga[i] = false;
        mVoChnStarted[i] = false;

        for (int j = 0; j < DRM_RGA_CHANNEL_BUTT; ++j) {
            for (int k = 0; k < DRM_RGA_CHANNEL_BUTT; ++k) {
                mBindRgaRga2[i][j][k] = false;
                for (int y = 0; y < DRM_VMIX_CHANNEL_BUTT; ++y) {
                    mBindRgaVmix[i][y][k] = false;
                    mBindVmixVo[i][y] = false;
                }
            }
        }
    }
}

MediaVo::~MediaVo()
{
    drm_chn_t stSrcChn, stDstChn;

    for (int i = DRM_VO_CHANNEL_00; i < DRM_VO_CHANNEL_BUTT; ++i) {
        if (mBindRgaVo[i]) {
            stSrcChn.enModId = MOD_ID_RGA;
            stSrcChn.s32ChnId = mZoomRgaChn[i];
            stSrcChn.s32DevId = 0;
            stDstChn.enModId = MOD_ID_VO;
            stDstChn.s32ChnId = mZoomVoChn[i];
            stDstChn.s32DevId = 0;
            drm_mpi_system_unbind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        }

        if (mBindRgaRga[i]) {
            stSrcChn.enModId = MOD_ID_RGA;
            stSrcChn.s32ChnId = mCropRgaChn[i];
            stSrcChn.s32DevId = 0;
            stDstChn.enModId = MOD_ID_RGA;
            stDstChn.s32ChnId = mZoomRgaChn[i];
            stDstChn.s32DevId = 0;
            drm_mpi_system_unbind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        }

        for (int zidx = 0; zidx < DRM_RGA_CHANNEL_BUTT; ++zidx) {
            for (int cidx = 0; cidx < DRM_RGA_CHANNEL_BUTT; ++cidx) {
                // 1.解绑RGA裁剪通道和RGA缩放通道
                if (mBindRgaRga2[i][zidx][cidx]) {
                    stSrcChn.enModId = MOD_ID_RGA;
                    stSrcChn.s32ChnId = mCropRgaChn2[i][cidx];
                    stSrcChn.s32DevId = 0;
                    stDstChn.enModId = MOD_ID_RGA;
                    stDstChn.s32ChnId = mZoomRgaChn2[i][zidx];
                    stDstChn.s32DevId = 0;
                    drm_mpi_system_unbind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
                    mBindRgaRga2[i][zidx][cidx] = false;
                }
            }
        }

        for (int midx = 0; midx < DRM_VMIX_CHANNEL_BUTT; ++midx) {
            for (int ridx = 0; ridx < DRM_RGA_CHANNEL_BUTT; ++ridx) {
                // 2.解绑定RGA缩放通道和VMIX通道
                if (mBindRgaVmix[i][midx][ridx]) {
                    stSrcChn.enModId = MOD_ID_RGA;
                    stSrcChn.s32ChnId = mZoomRgaChn2[i][ridx];
                    stSrcChn.s32DevId = 0;
                    stDstChn.enModId = MOD_ID_VMIX;
                    stDstChn.s32ChnId = midx;
                    stDstChn.s32DevId = 0;
                    drm_mpi_system_unbind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
                    mBindRgaVmix[i][midx][ridx] = false;
                }
            }

            // 3.解绑定VMIX通道和VO通道
            if (mBindVmixVo[i][midx]) {
                stSrcChn.enModId = MOD_ID_VMIX;
                stSrcChn.s32ChnId = 0;
                stSrcChn.s32DevId = 0;
                stDstChn.enModId = MOD_ID_VO;
                stDstChn.s32ChnId = i;
                stDstChn.s32DevId = 0;
                drm_mpi_system_unbind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
                mBindVmixVo[i][midx] = false;
            }
        }

        mBindRgaVo[i] = false;
        mBindRgaRga[i] = false;
        destroyVoChn(i);
    }
}

int MediaVo::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

media_buffer_t MediaVo::getFrame(int modeIdChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, modeIdChn, s32MilliSec);
}

void *MediaVo::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaVo::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaVo::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

bool MediaVo::voChnStart(int voChn)
{
    if ((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT)) {
        rkmpi_error("invalid vo channel:[%d]", voChn);
        return false;
    }

    return mVoChnStarted[voChn];
}

int MediaVo::destroyVoChn(int voChn)
{
    if ((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT)) {
        rkmpi_error("invalid vo channel:[%d]", voChn);
        return -1;
    }

    if (mVoChnStarted[voChn] == false) {
        return 0;
    }

    int ret = drm_mpi_vo_destroy_channel(voChn);
    if (ret) {
        return ret;
    }

    mVoChnStarted[voChn] = false;
    return 0;
}

int MediaVo::createVoChn(int voChn, int zPOS, drm_plane_type_e enDispLayer, size_t dispWidth, size_t dispHeight, size_t dispXoffset, size_t dispYoffset, bool bWH2HW, drm_image_type_e enPixFmt, const char *dispCard)
{
    if ((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT)) {
        rkmpi_error("invalid vo channel:[%d]", voChn);
        return -1;
    }

    if (mVoChnStarted[voChn] == true) {
        rkmpi_error("vo channel:[%d] had started", voChn);
        return 0;
    }

    int ret = 0;
    drm_vo_chn_attr_t stVoAttr;

    memset(&stVoAttr, 0x00, sizeof(stVoAttr));
    stVoAttr.u16Zpos = zPOS;
    stVoAttr.pcDevNode = dispCard;
    stVoAttr.enImgType = enPixFmt;
    stVoAttr.emPlaneType = enDispLayer;
    stVoAttr.stImgRect.s32X = 0;
    stVoAttr.stImgRect.s32Y = 0;
    stVoAttr.stImgRect.u32Width = bWH2HW ? dispHeight : dispWidth;
    stVoAttr.stImgRect.u32Height = bWH2HW ? dispWidth : dispHeight;
    stVoAttr.stDispRect.s32X = dispXoffset;
    stVoAttr.stDispRect.s32Y = dispYoffset;
    stVoAttr.stDispRect.u32Width = bWH2HW ? dispHeight : dispWidth;
    stVoAttr.stDispRect.u32Height = bWH2HW ? dispWidth : dispHeight;

    ret = drm_mpi_vo_create_channel(voChn, (const drm_vo_chn_attr_t *)&stVoAttr);
    if (ret) {
        rkmpi_error("create vo channel:[%d] failed, return:[%d]", voChn, ret);
        return ret;
    }

    mVoChnStarted[voChn] = true;
    return 0;
}

int MediaVo::setVoChnFrameRate(int voChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen)
{
    if ((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT)) {
        return -1;
    }

    drm_fps_attr_t fpsAttr;
    memset(&fpsAttr, 0x00, sizeof(fpsAttr));
    fpsAttr.s32FpsInNum = fpsInNum;
    fpsAttr.s32FpsInDen = fpsInDen;
    fpsAttr.s32FpsOutNum = fpsOutNum;
    fpsAttr.s32FpsOutDen = fpsOutDen;

    return drm_mpi_system_set_framerate(MOD_ID_VO, voChn, &fpsAttr);
}

int MediaVo::convertVoChn(int voChn, int rgaChn, media_buffer_t mediaFrame, int s32MilliSec, size_t waitConvUs)
{
    if (((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT)) || ((rgaChn < DRM_RGA_CHANNEL_00) || (rgaChn >= DRM_RGA_CHANNEL_BUTT)) || !mediaFrame) {
        return -1;
    }

    int ret = sendFrame(rgaChn, mediaFrame, MOD_ID_RGA);
    if (ret) {
        return ret;
    }

    usleep(waitConvUs);

    media_buffer_t mb = getFrame(rgaChn, s32MilliSec, MOD_ID_RGA);
    ret = sendFrame(voChn, mb, MOD_ID_VO);
    releaseFrame(mb);

    return ret;
}

int MediaVo::cropZoomVoChn(int voChn, int cropRgaChn, int zoomRgaChn, media_buffer_t mediaFrame, int s32MilliSec, size_t waitConvUs)
{
    if (((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT))
        || ((cropRgaChn < DRM_RGA_CHANNEL_00) || (cropRgaChn >= DRM_RGA_CHANNEL_BUTT)
        || ((zoomRgaChn < DRM_RGA_CHANNEL_00) || (zoomRgaChn >= DRM_RGA_CHANNEL_BUTT)))
        || !mediaFrame)
    {
        rkmpi_error("invalid input parameter");
        return -1;
    }

    int ret = sendFrame(cropRgaChn, mediaFrame, MOD_ID_RGA);
    if (ret) {
        rkmpi_error("send mediaFrame to rga chn:[%d] failed, return:[%d]", cropRgaChn, ret);
        return ret;
    }

    usleep(waitConvUs);
    media_buffer_t mediaBuffer = getFrame(cropRgaChn, s32MilliSec, MOD_ID_RGA);
    if (mediaBuffer == NULL) {
        rkmpi_error("get mediaFrame from rga chn:[%d] failed, return NULL", cropRgaChn);
        return -1;
    }

    ret = sendFrame(zoomRgaChn, mediaBuffer, MOD_ID_RGA);
    if (ret) {
        releaseFrame(mediaBuffer);
        rkmpi_error("send mediaFrame to rga chn:[%d] failed, return:[%d]", zoomRgaChn, ret);

        return ret;
    }

    usleep(waitConvUs);
    releaseFrame(mediaBuffer);
    mediaBuffer = NULL;
    usleep(waitConvUs);

    mediaBuffer = getFrame(zoomRgaChn, s32MilliSec, MOD_ID_RGA);
    if (mediaBuffer == NULL) {
        rkmpi_error("get mediaFrame from rga chn:[%d] failed, return NULL", zoomRgaChn);
        return -2;
    }

    ret = sendFrame(voChn, mediaBuffer, MOD_ID_VO);
    releaseFrame(mediaBuffer);

    return ret;
}

int MediaVo::sendZoomVoChnWithBind(media_buffer_t mediaFrame, int voChn, int cropRgaChn, int zoomRgaChn)
{
    if (((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT))
        || ((cropRgaChn < DRM_RGA_CHANNEL_00) || (cropRgaChn >= DRM_RGA_CHANNEL_BUTT)
        || ((zoomRgaChn < DRM_RGA_CHANNEL_00) || (zoomRgaChn >= DRM_RGA_CHANNEL_BUTT)))
        || !mediaFrame)
    {
        rkmpi_error("invalid input parameter");
        return -1;
    }

    if (mBindRgaVo[voChn] && mBindRgaRga[voChn]) {
        return sendFrame(cropRgaChn, mediaFrame, MOD_ID_RGA);
    }

    mZoomVoChn[voChn] = voChn;
    mCropRgaChn[voChn] = cropRgaChn;
    mZoomRgaChn[voChn] = zoomRgaChn;

    int ret = -1;
    drm_chn_t stSrcChn, stDstChn;

    if (!mBindRgaRga[voChn]) {
        stSrcChn.enModId = MOD_ID_RGA;
        stSrcChn.s32ChnId = cropRgaChn;
        stSrcChn.s32DevId = 0;
        stDstChn.enModId = MOD_ID_RGA;
        stDstChn.s32ChnId = zoomRgaChn;
        stDstChn.s32DevId = 0;
        ret = drm_mpi_system_bind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        if (ret < 0) {
            rkmpi_error("bind rga chn:[%d] and rga chn:[%d] failed, return:[%d]", cropRgaChn, zoomRgaChn, ret);
            return ret;
        }
    }
    mBindRgaRga[voChn] = true;

    if (!mBindRgaVo[voChn]) {
        stSrcChn.enModId = MOD_ID_RGA;
        stSrcChn.s32ChnId = zoomRgaChn;
        stSrcChn.s32DevId = 0;
        stDstChn.enModId = MOD_ID_VO;
        stDstChn.s32ChnId = voChn;
        stDstChn.s32DevId = 0;
        ret = drm_mpi_system_bind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        if (ret < 0) {
            rkmpi_error("bind rga chn:[%d] and vo chn:[%d] failed, return:[%d]", zoomRgaChn, voChn, ret);
            return ret;
        }
    }
    mBindRgaVo[voChn] = true;

    return 0;
}

int MediaVo::sendZoomRgaVmixVoChnWithBind(media_buffer_t mediaFrame, int vmixDev, int vmixChn, int voChn, int cropRgaChn, int zoomRgaChn)
{
    if (((voChn < DRM_VO_CHANNEL_00) || (voChn >= DRM_VO_CHANNEL_BUTT))
        || ((cropRgaChn < DRM_RGA_CHANNEL_00) || (cropRgaChn >= DRM_RGA_CHANNEL_BUTT))
        || ((zoomRgaChn < DRM_RGA_CHANNEL_00) || (zoomRgaChn >= DRM_RGA_CHANNEL_BUTT))
        || ((vmixDev < DRM_VMIX_DEVICE_00) || (vmixDev >= DRM_VMIX_DEVICE_BUTT))
        || ((vmixChn < DRM_VMIX_CHANNEL_00) || (vmixChn >= DRM_VMIX_CHANNEL_BUTT))
        || !mediaFrame)
    {
        rkmpi_error("invalid input parameter");
        return -1;
    }

    if (mBindVmixVo[voChn][vmixDev] && mBindRgaVmix[voChn][vmixChn][zoomRgaChn] && mBindRgaRga2[voChn][cropRgaChn][zoomRgaChn]) {
        return sendFrame(cropRgaChn, mediaFrame, MOD_ID_RGA);
    }

    mZoomVoChn2[voChn] = voChn;
    mCropRgaChn2[voChn][cropRgaChn] = cropRgaChn;
    mZoomRgaChn2[voChn][zoomRgaChn] = zoomRgaChn;

    int ret = -1;
    drm_chn_t stSrcChn, stDstChn;

    // 1.VMIX通道绑定到VO输出通道
    if (!mBindVmixVo[voChn][vmixDev]) {
        stSrcChn.enModId = MOD_ID_VMIX;
        stSrcChn.s32ChnId = 0;          // 通道号是无效的，填0即可(因为一个vmixDev有多个通道，所以一个vmixDev就是一个整体)
        stSrcChn.s32DevId = vmixDev;
        stDstChn.enModId = MOD_ID_VO;
        stDstChn.s32ChnId = voChn;
        stDstChn.s32DevId = 0;
        ret = drm_mpi_system_bind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        if (ret < 0) {
            rkmpi_error("bind vmix dev:[%d] chn:[%d] to vo chn:[%d] failed, return:[%d]", 0, vmixChn, voChn, ret);
            return ret;
        }
    }
    mBindVmixVo[voChn][vmixDev] = true;

    // 2.RGA缩放通道绑定到VMIX通道
    if (!mBindRgaVmix[voChn][vmixChn][zoomRgaChn]) {
        stSrcChn.enModId = MOD_ID_RGA;
        stSrcChn.s32ChnId = zoomRgaChn;
        stSrcChn.s32DevId = 0;
        stDstChn.enModId = MOD_ID_VMIX;
        stDstChn.s32ChnId = vmixChn;
        stDstChn.s32DevId = vmixDev;
        ret = drm_mpi_system_bind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        if (ret < 0) {
            rkmpi_error("bind rga chn:[%d] to vmix dev:[%d] chn:[%d] failed, return:[%d]", zoomRgaChn, 0, vmixChn, ret);
            return ret;
        }
    }
    mBindRgaVmix[voChn][vmixChn][zoomRgaChn] = true;

    // 3.RGA裁剪通道绑定到RGA缩放通道
    if (!mBindRgaRga2[voChn][cropRgaChn][zoomRgaChn]) {
        stSrcChn.enModId = MOD_ID_RGA;
        stSrcChn.s32ChnId = cropRgaChn;
        stSrcChn.s32DevId = 0;
        stDstChn.enModId = MOD_ID_RGA;
        stDstChn.s32ChnId = zoomRgaChn;
        stDstChn.s32DevId = 0;
        ret = drm_mpi_system_bind((const drm_chn_t *)&stSrcChn, (const drm_chn_t *)&stDstChn);
        if (ret < 0) {
            rkmpi_error("bind rga chn:[%d] to rga chn:[%d] failed, return:[%d]", cropRgaChn, zoomRgaChn, ret);
            return ret;
        }
    }
    mBindRgaRga2[voChn][cropRgaChn][zoomRgaChn] = true;

    return 0;
}

API_END_NAMESPACE(media)
