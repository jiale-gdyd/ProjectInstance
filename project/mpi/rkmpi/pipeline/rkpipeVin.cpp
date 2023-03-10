#include <string.h>
#include "rkpipe.hpp"
#include "../rkmpi.h"

API_BEGIN_NAMESPACE(mpi)

int rkpipe_create_vin(rkpipe_t *pipe)
{
    drm_vi_chn_attr_t stViAttr;
    memset(&stViAttr, 0x00, sizeof(stViAttr));
    stViAttr.u32BufCnt = pipe->vin.bufPoolCnt;
    stViAttr.enPixFmt = (drm_image_type_e)pipe->vin.pixFormat;
    stViAttr.u32Width = pipe->vin.vinWidth;
    stViAttr.u32Height = pipe->vin.vinHeight;
    stViAttr.pcVideoNode = pipe->vin.device.c_str();
    stViAttr.enWorkMode = DRM_VI_WORK_MODE_NORMAL;
    if (pipe->vin.allocDMA) {
        stViAttr.enBufType = DRM_VI_CHN_BUF_TYPE_DMA;
    } else {
        stViAttr.enBufType = DRM_VI_CHN_BUF_TYPE_MMAP;
    }

    int ret = drm_mpi_vi_set_channel_attribute(pipe->vin.vinChannel, (const drm_vi_chn_attr_t *)&stViAttr);
    if (ret) {
        rkmpi_error("set vin channel:[%d] attribute failed, return:[%d]", pipe->vin.vinChannel, ret);
        return -1;
    }

    ret = drm_mpi_vi_enable_channel(pipe->vin.vinChannel);
    if (ret) {
        rkmpi_error("enable vin channel:[%d] failed, return:[%d]", pipe->vin.vinChannel, ret);
        return -2;
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_VI, pipe->vin.vinChannel, pipe->vin.bufDepth);
    if (ret) {
        rkmpi_warn("vin channel:[%d] set buff depth failed, return:[%d]", pipe->vin.vinChannel, ret);
    }

    return 0;
}

int rkpipe_release_vin(rkpipe_t *pipe)
{
    return drm_mpi_vi_disable_channel(pipe->vin.vinChannel);
}

API_END_NAMESPACE(mpi)
