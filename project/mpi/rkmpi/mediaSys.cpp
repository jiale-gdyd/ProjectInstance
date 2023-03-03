#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/prctl.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaSys.hpp"
#include "../private.h"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaSys::MediaSys()
{

}

MediaSys::~MediaSys()
{
    deinit();
}

int MediaSys::init()
{
    return drm_mpi_system_init();
}

void MediaSys::deinit()
{

}

int MediaSys::bind(drm_chn_t src, drm_chn_t dst)
{
    return drm_mpi_system_bind((const drm_chn_t *)&src, (const drm_chn_t *)&dst);
}

int MediaSys::unbind(drm_chn_t src, drm_chn_t dst)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&src, (const drm_chn_t *)&dst);
}

int MediaSys::bindViVo(drm_chn_t vi, drm_chn_t vo)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vi, (const drm_chn_t *)&vo);
}

int MediaSys::bindViRga(drm_chn_t vi, drm_chn_t rga)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vi, (const drm_chn_t *)&rga);
}

int MediaSys::bindRgaVo(drm_chn_t rga, drm_chn_t vo)
{
    return drm_mpi_system_bind((const drm_chn_t *)&rga, (const drm_chn_t *)&vo);
}

int MediaSys::bindViVenc(drm_chn_t vi, drm_chn_t venc)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vi, (const drm_chn_t *)&venc);
}

int MediaSys::bindViVmix(drm_chn_t vi, drm_chn_t vmix)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vi, (const drm_chn_t *)&vmix);
}

int MediaSys::bindVdecVo(drm_chn_t vdec, drm_chn_t vo)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vdec, (const drm_chn_t *)&vo);
}

int MediaSys::bindVmixVo(drm_chn_t vmix, drm_chn_t vo)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vmix, (const drm_chn_t *)&vo);
}

int MediaSys::bindRgaRga(drm_chn_t rgaSrc, drm_chn_t rgaDst)
{
    return drm_mpi_system_bind((const drm_chn_t *)&rgaSrc, (const drm_chn_t *)&rgaDst);
}

int MediaSys::bindRgaVenc(drm_chn_t rga, drm_chn_t venc)
{
    return drm_mpi_system_bind((const drm_chn_t *)&rga, (const drm_chn_t *)&venc);
}

int MediaSys::bindRgaVmix(drm_chn_t rga, drm_chn_t vmix)
{
    return drm_mpi_system_bind((const drm_chn_t *)&rga, (const drm_chn_t *)&vmix);
}

int MediaSys::bindVdecRga(drm_chn_t vdec, drm_chn_t rga)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vdec, (const drm_chn_t *)&rga);
}

int MediaSys::bindVdecVenc(drm_chn_t vdec, drm_chn_t venc)
{
    return drm_mpi_system_bind((const drm_chn_t *)&vdec, (const drm_chn_t *)&venc);
}

int MediaSys::unbindViVo(drm_chn_t vi, drm_chn_t vo)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vi, (const drm_chn_t *)&vo);
}

int MediaSys::unbindViRga(drm_chn_t vi, drm_chn_t rga)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vi, (const drm_chn_t *)&rga);
}

int MediaSys::unbindRgaVo(drm_chn_t rga, drm_chn_t vo)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&rga, (const drm_chn_t *)&vo);
}

int MediaSys::unbindViVenc(drm_chn_t vi, drm_chn_t venc)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vi, (const drm_chn_t *)&venc);
}

int MediaSys::unbindViVmix(drm_chn_t vi, drm_chn_t vmix)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vi, (const drm_chn_t *)&vmix);
}

int MediaSys::unbindVdecVo(drm_chn_t vdec, drm_chn_t vo)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vdec, (const drm_chn_t *)&vo);
}

int MediaSys::unbindVmixVo(drm_chn_t vmix, drm_chn_t vo)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vmix, (const drm_chn_t *)&vo);
}

int MediaSys::unbindRgaVenc(drm_chn_t rga, drm_chn_t venc)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&rga, (const drm_chn_t *)&venc);
}

int MediaSys::unbindRgaVmix(drm_chn_t rga, drm_chn_t vmix)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&rga, (const drm_chn_t *)&vmix);
}

int MediaSys::unbindVdecRga(drm_chn_t vdec, drm_chn_t rga)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vdec, (const drm_chn_t *)&rga);
}

int MediaSys::unbindVdecVenc(drm_chn_t vdec, drm_chn_t venc)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&vdec, (const drm_chn_t *)&venc);
}

int MediaSys::unbindRgaRga(drm_chn_t rgaSrc, drm_chn_t rgaDst)
{
    return drm_mpi_system_unbind((const drm_chn_t *)&rgaSrc, (const drm_chn_t *)&rgaDst);
}

int MediaSys::bindViVo(int viDevId, int viChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return bindViVo(stSrcChn, stDstChn);
}

int MediaSys::bindViRga(int viDevId, int viChn, int rgaDevId, int rgaChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = rgaChn;
    stDstChn.s32DevId = rgaDevId;
    stDstChn.enModId = MOD_ID_RGA;

    return bindViRga(stSrcChn, stDstChn);
}

int MediaSys::bindRgaVo(int rgaDevId, int rgaChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaChn;
    stSrcChn.s32DevId = rgaDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return bindRgaVo(stSrcChn, stDstChn);
}

int MediaSys::bindViVenc(int viDevId, int viChn, int vencDevId, int vencChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = vencChn;
    stDstChn.s32DevId = vencDevId;
    stDstChn.enModId = MOD_ID_VENC;

    return bindViVenc(stSrcChn, stDstChn);
}

int MediaSys::bindViVmix(int viDevId, int viChn, int vmixDevId, int vmixChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = vmixChn;
    stDstChn.s32DevId = vmixDevId;
    stDstChn.enModId = MOD_ID_VMIX;

    return bindViVmix(stSrcChn, stDstChn);
}

int MediaSys::bindVdecVo(int vdecDevId, int vdecChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vdecChn;
    stSrcChn.s32DevId = vdecDevId;
    stSrcChn.enModId = MOD_ID_VDEC;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return bindVdecVo(stSrcChn, stDstChn);
}

int MediaSys::bindVmixVo(int vmixDevId, int vmixChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vmixChn;
    stSrcChn.s32DevId = vmixDevId;
    stSrcChn.enModId = MOD_ID_VMIX;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return bindVmixVo(stSrcChn, stDstChn);
}

int MediaSys::bindRgaVenc(int rgaDevId, int rgaChn, int vencDevId, int vencChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaChn;
    stSrcChn.s32DevId = rgaDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = vencChn;
    stDstChn.s32DevId = vencDevId;
    stDstChn.enModId = MOD_ID_VENC;

    return bindRgaVenc(stSrcChn, stDstChn);
}

int MediaSys::bindRgaVmix(int rgaDevId, int rgaChn, int vmixDevId, int vmixChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaChn;
    stSrcChn.s32DevId = rgaDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = vmixChn;
    stDstChn.s32DevId = vmixDevId;
    stDstChn.enModId = MOD_ID_VMIX;

    return bindRgaVmix(stSrcChn, stDstChn);
}

int MediaSys::bindVdecRga(int vdecDevId, int vdecChn, int rgaDevId, int rgaChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vdecChn;
    stSrcChn.s32DevId = vdecDevId;
    stSrcChn.enModId = MOD_ID_VDEC;

    stDstChn.s32ChnId = rgaChn;
    stDstChn.s32DevId = rgaDevId;
    stDstChn.enModId = MOD_ID_RGA;

    return bindVdecRga(stSrcChn, stDstChn);
}

int MediaSys::bindVdecVenc(int vdecDevId, int vdecChn, int vencDevId, int vencChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vdecChn;
    stSrcChn.s32DevId = vdecDevId;
    stSrcChn.enModId = MOD_ID_VDEC;

    stDstChn.s32ChnId = vencChn;
    stDstChn.s32DevId = vencDevId;
    stDstChn.enModId = MOD_ID_VENC;

    return bindVdecVenc(stSrcChn, stDstChn);
}

int MediaSys::bindRgaRga(int rgaSrcDevId, int rgaSrcChn, int rgaDstDevId, int rgaDstChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaSrcChn;
    stSrcChn.s32DevId = rgaSrcDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = rgaDstChn;
    stDstChn.s32DevId = rgaDstDevId;
    stDstChn.enModId = MOD_ID_RGA;

    return bindRgaRga(stSrcChn, stDstChn);
}

int MediaSys::unbindViVo(int viDevId, int viChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return unbindViVo(stSrcChn, stDstChn);
}

int MediaSys::unbindViRga(int viDevId, int viChn, int rgaDevId, int rgaChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = rgaChn;
    stDstChn.s32DevId = rgaDevId;
    stDstChn.enModId = MOD_ID_RGA;

    return unbindViRga(stSrcChn, stDstChn);
}

int MediaSys::unbindRgaVo(int rgaDevId, int rgaChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaChn;
    stSrcChn.s32DevId = rgaDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return unbindRgaVo(stSrcChn, stDstChn);
}

int MediaSys::unbindViVenc(int viDevId, int viChn, int vencDevId, int vencChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = vencChn;
    stDstChn.s32DevId = vencDevId;
    stDstChn.enModId = MOD_ID_VENC;

    return unbindViVenc(stSrcChn, stDstChn);
}

int MediaSys::unbindViVmix(int viDevId, int viChn, int vmixDevId, int vmixChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = viChn;
    stSrcChn.s32DevId = viDevId;
    stSrcChn.enModId = MOD_ID_VI;

    stDstChn.s32ChnId = vmixChn;
    stDstChn.s32DevId = vmixDevId;
    stDstChn.enModId = MOD_ID_VMIX;

    return unbindViVmix(stSrcChn, stDstChn);
}

int MediaSys::unbindVdecVo(int vdecDevId, int vdecChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vdecChn;
    stSrcChn.s32DevId = vdecDevId;
    stSrcChn.enModId = MOD_ID_VDEC;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return unbindVdecVo(stSrcChn, stDstChn);
}

int MediaSys::unbindVmixVo(int vmixDevId, int vmixChn, int voDevId, int voChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vmixChn;
    stSrcChn.s32DevId = vmixDevId;
    stSrcChn.enModId = MOD_ID_VMIX;

    stDstChn.s32ChnId = voChn;
    stDstChn.s32DevId = voDevId;
    stDstChn.enModId = MOD_ID_VO;

    return unbindVmixVo(stSrcChn, stDstChn);
}

int MediaSys::unbindRgaVenc(int rgaDevId, int rgaChn, int vencDevId, int vencChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaChn;
    stSrcChn.s32DevId = rgaDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = vencChn;
    stDstChn.s32DevId = vencDevId;
    stDstChn.enModId = MOD_ID_VENC;

    return unbindRgaVenc(stSrcChn, stDstChn);
}

int MediaSys::unbindRgaVmix(int rgaDevId, int rgaChn, int vmixDevId, int vmixChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaChn;
    stSrcChn.s32DevId = rgaDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = vmixChn;
    stDstChn.s32DevId = vmixDevId;
    stDstChn.enModId = MOD_ID_VMIX;

    return unbindRgaVmix(stSrcChn, stDstChn);
}

int MediaSys::unbindVdecRga(int vdecDevId, int vdecChn, int rgaDevId, int rgaChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vdecChn;
    stSrcChn.s32DevId = vdecDevId;
    stSrcChn.enModId = MOD_ID_VDEC;

    stDstChn.s32ChnId = rgaChn;
    stDstChn.s32DevId = rgaDevId;
    stDstChn.enModId = MOD_ID_RGA;

    return unbindVdecRga(stSrcChn, stDstChn);
}

int MediaSys::unbindVdecVenc(int vdecDevId, int vdecChn, int vencDevId, int vencChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = vdecChn;
    stSrcChn.s32DevId = vdecDevId;
    stSrcChn.enModId = MOD_ID_VDEC;

    stDstChn.s32ChnId = vencChn;
    stDstChn.s32DevId = vencDevId;
    stDstChn.enModId = MOD_ID_VENC;

    return unbindVdecVenc(stSrcChn, stDstChn);
}

int MediaSys::unbindRgaRga(int rgaSrcDevId, int rgaSrcChn, int rgaDstDevId, int rgaDstChn)
{
    drm_chn_t stSrcChn, stDstChn;
    memset(&stSrcChn, 0x00, sizeof(stSrcChn));
    memset(&stDstChn, 0x00, sizeof(stDstChn));

    stSrcChn.s32ChnId = rgaSrcChn;
    stSrcChn.s32DevId = rgaSrcDevId;
    stSrcChn.enModId = MOD_ID_RGA;

    stDstChn.s32ChnId = rgaDstChn;
    stDstChn.s32DevId = rgaDstDevId;
    stDstChn.enModId = MOD_ID_RGA;

    return unbindRgaRga(stSrcChn, stDstChn);
}

int MediaSys::bindViVo(int viChn, int voChn)
{
    return bindViVo(0, viChn, 0, voChn);
}

int MediaSys::bindViRga(int viChn, int rgaChn)
{
    return bindViRga(0, viChn, 0, rgaChn);
}

int MediaSys::bindRgaVo(int rgaChn, int voChn)
{
    return bindRgaVo(0, rgaChn, 0, voChn);
}

int MediaSys::bindViVenc(int viChn, int vencChn)
{
    return bindViVenc(0, viChn, 0, vencChn);
}

int MediaSys::bindViVmix(int viChn, int vmixChn)
{
    return bindViVmix(0, viChn, 0, vmixChn);
}

int MediaSys::bindVdecVo(int vdecChn, int voChn)
{
    return bindVdecVo(0, vdecChn, 0, voChn);
}

int MediaSys::bindVmixVo(int vmixChn, int voChn)
{
    return bindVmixVo(0, vmixChn, 0, voChn);
}

int MediaSys::bindRgaVenc(int rgaChn, int vencChn)
{
    return bindRgaVenc(0, rgaChn, 0, vencChn);
}

int MediaSys::bindRgaVmix(int rgaChn, int vmixChn)
{
    return bindRgaVmix(0, rgaChn, 0, vmixChn);
}

int MediaSys::bindVdecRga(int vdecChn, int rgaChn)
{
    return bindVdecRga(0, vdecChn, 0, rgaChn);
}

int MediaSys::bindVdecVenc(int vdecChn, int vencChn)
{
    return bindVdecVenc(0, vdecChn, 0, vencChn);
}

int MediaSys::bindRgaRga(int rgaSrcChn, int rgaDstChn)
{
    return bindRgaRga(0, rgaSrcChn, 0, rgaDstChn);
}

int MediaSys::unbindViVo(int viChn, int voChn)
{
    return unbindViVo(0, viChn, 0, voChn);
}

int MediaSys::unbindViRga(int viChn, int rgaChn)
{
    return unbindViRga(0, viChn, 0, rgaChn);
}

int MediaSys::unbindRgaVo(int rgaChn, int voChn)
{
    return unbindRgaVo(0, rgaChn, 0, voChn);
}

int MediaSys::unbindViVenc(int viChn, int vencChn)
{
    return unbindViVenc(0, viChn, 0, vencChn);
}

int MediaSys::unbindViVmix(int viChn, int vmixChn)
{
    return unbindViVmix(0, viChn, 0, vmixChn);
}

int MediaSys::unbindVdecVo(int vdecChn, int voChn)
{
    return unbindVdecVo(0, vdecChn, 0, voChn);
}

int MediaSys::unbindVmixVo(int vmixChn, int voChn)
{
    return unbindVmixVo(0, vmixChn, 0, voChn);
}

int MediaSys::unbindRgaVenc(int rgaChn, int vencChn)
{
    return unbindRgaVenc(0, rgaChn, 0, vencChn);
}

int MediaSys::unbindRgaVmix(int rgaChn, int vmixChn)
{
    return unbindRgaVmix(0, rgaChn, 0, vmixChn);
}

int MediaSys::unbindVdecRga(int vdecChn, int rgaChn)
{
    return unbindVdecRga(0, vdecChn, 0, rgaChn);
}

int MediaSys::unbindVdecVenc(int vdecChn, int vencChn)
{
    return unbindVdecVenc(0, vdecChn, 0, vencChn);
}

int MediaSys::unbindRgaRga(int rgaSrcChn, int rgaDstChn)
{
    return unbindRgaRga(0, rgaSrcChn, 0, rgaDstChn);
}

int MediaSys::setChnFrameRate(int modeId, int modeIdChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen)
{
    drm_fps_attr_t fpsAttr;
    memset(&fpsAttr, 0x00, sizeof(fpsAttr));
    fpsAttr.s32FpsInNum = fpsInNum;
    fpsAttr.s32FpsInDen = fpsInDen;
    fpsAttr.s32FpsOutNum = fpsOutNum;
    fpsAttr.s32FpsOutDen = fpsOutDen;

    return drm_mpi_system_set_framerate((mod_id_e)modeId, modeIdChn, &fpsAttr);
}

int MediaSys::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

media_buffer_t MediaSys::getFrame(int modeIdChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, modeIdChn, s32MilliSec);
}

void *MediaSys::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaSys::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaSys::getFrameFd(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_fd(pstFrameInfo);
}

int MediaSys::getDmaDeviceFd(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_device_fd(pstFrameInfo);
}

int MediaSys::getDmaBufferHandle(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_handle(pstFrameInfo);
}

size_t MediaSys::getFrameSize(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_size(pstFrameInfo);
}

int MediaSys::setFrameSize(media_buffer_t pstFrameInfo, size_t size)
{
    return drm_mpi_mb_set_size(pstFrameInfo, size);
}

mod_id_e MediaSys::getFrameModeId(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_mode_id(pstFrameInfo);
}

int MediaSys::getFrameChnId(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_channel_id(pstFrameInfo);
}

uint64_t MediaSys::getFrameTimestamp(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_timestamp(pstFrameInfo);
}

int MediaSys::setFrameTimestamp(media_buffer_t pstFrameInfo, uint64_t timestamp)
{
    return drm_mpi_mb_set_timestamp(pstFrameInfo, timestamp);
}

int MediaSys::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

media_buffer_t MediaSys::createFrame(size_t size, bool bHardware, unsigned char flags)
{
    return drm_mpi_mb_create_buffer(size, bHardware, flags);
}

media_buffer_t MediaSys::createImageFrame(mb_image_info_t *pstImageInfo, bool bHardware, unsigned char flags)
{
    return drm_mpi_mb_create_image_buffer(pstImageInfo, bHardware, flags);
}

media_buffer_t MediaSys::convertToImageFrame(media_buffer_t pstFrameInfo, mb_image_info_t *pstImageInfo)
{
    return drm_mpi_mb_convert_to_image_buffer(pstFrameInfo, pstImageInfo);
}

int MediaSys::getFrameImageInfo(media_buffer_t pstFrameInfo, mb_image_info_t *pstImageInfo)
{
    return drm_mpi_mb_get_image_info(pstFrameInfo, pstImageInfo);
}

media_buffer_t MediaSys::copyFrame(media_buffer_t pstFrameInfo, bool bZeroCopy)
{
    return drm_mpi_mb_copy(pstFrameInfo, bZeroCopy);
}

int MediaSys::getFrameFlags(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_flag(pstFrameInfo);
}

int MediaSys::beginCPUAccess(media_buffer_t mediaFrame, bool readOnly)
{
    return drm_mpi_mb_start_cpu_access(mediaFrame, readOnly);
}

int MediaSys::endCPUAccess(media_buffer_t mediaFrame, bool readOnly)
{
    return drm_mpi_mb_finish_cpu_access(mediaFrame, readOnly);
}

media_buffer_pool_t MediaSys::createFramePool(mb_pool_param_t *pstPoolParam)
{
    return drm_mpi_mb_pool_create(pstPoolParam);
}

int MediaSys::releaseFramePool(media_buffer_pool_t pstFramePool)
{
    return drm_mpi_mb_pool_destroy(pstFramePool);
}

media_buffer_t MediaSys::getPoolFrame(media_buffer_pool_t pstFramePool, bool bBlock)
{
    return drm_mpi_mb_pool_get_buffer(pstFramePool, bBlock);
}

int MediaSys::registerMediaOutCb(drm_chn_t stChn, OutCallbackFunction callback)
{
    return drm_mpi_system_register_output_callback((const drm_chn_t *)&stChn, callback);
}

int MediaSys::registerMediaOutCbEx(drm_chn_t stChn, OutCallbackFunctionEx callback, void *user_data)
{
    return drm_mpi_system_register_output_callbackEx((const drm_chn_t *)&stChn, callback, user_data);
}

bool MediaSys::setupEnableRGAFlushCache(bool bEnable)
{
    drm_mpi_rga_set_flush_cache(bEnable);
    return drm_mpi_rga_get_flush_cache();
}

API_END_NAMESPACE(media)
