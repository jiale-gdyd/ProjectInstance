#include <string.h>
#include "rkpipe.hpp"
#include "../rkmpi.h"

API_BEGIN_NAMESPACE(mpi)

int rkpipe_create_vo(rkpipe_t *pipe)
{
    drm_vo_chn_attr_t stVoAttr;
    memset(&stVoAttr, 0x00, sizeof(stVoAttr));
    stVoAttr.u16Zpos = pipe->vo.voDispZPos;
    stVoAttr.pcDevNode = pipe->vo.device.c_str();
    stVoAttr.enImgType = (drm_image_type_e)pipe->vo.voDispPixFormat;
    stVoAttr.emPlaneType = (drm_plane_type_e)pipe->vo.voDispLayer;
    stVoAttr.stImgRect.s32X = 0;
    stVoAttr.stImgRect.s32Y = 0;
    stVoAttr.stImgRect.u32Width = pipe->vo.swapWH ? pipe->vo.voDispHeight : pipe->vo.voDispWidth;
    stVoAttr.stImgRect.u32Height = pipe->vo.swapWH ? pipe->vo.voDispWidth : pipe->vo.voDispHeight;
    stVoAttr.stDispRect.s32X = pipe->vo.voDispXOffset;
    stVoAttr.stDispRect.s32Y = pipe->vo.voDispXOffset;
    stVoAttr.stDispRect.u32Width = pipe->vo.swapWH ? pipe->vo.voDispHeight : pipe->vo.voDispWidth;
    stVoAttr.stDispRect.u32Height = pipe->vo.swapWH ? pipe->vo.voDispWidth : pipe->vo.voDispHeight;

    int ret = drm_mpi_vo_create_channel(pipe->vo.voChannel, (const drm_vo_chn_attr_t *)&stVoAttr);
    if (ret) {
        rkmpi_error("create vo channel:[%d] failed, return:[%d]", pipe->vo.voChannel, ret);
        return ret;
    }

    return 0;
}

int rkpipe_release_vo(rkpipe_t *pipe)
{
    return drm_mpi_vo_destroy_channel(pipe->vo.voChannel);
}

API_END_NAMESPACE(mpi)
