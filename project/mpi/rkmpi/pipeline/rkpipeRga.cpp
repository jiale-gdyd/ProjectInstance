#include <string.h>
#include "rkpipe.hpp"
#include "../rkmpi.h"

API_BEGIN_NAMESPACE(mpi)

int rkpipe_create_rga(rkpipe_t *pipe)
{
    uint16_t rotation = 0;
    drm_rga_attr_t stRgaAttr;
    drm_rga_flip_e flip = RGA_FLIP_NULL;

    if (pipe->rga.flipMode == 1) {
        flip = RGA_FLIP_H;
    } else if (pipe->rga.flipMode == 2) {
        flip = RGA_FLIP_V;
    } else if (pipe->rga.flipMode == 3) {
        flip = RGA_FLIP_HV;
    }

    if (pipe->rga.rotateAngle == 90) {
        rotation = 90;
    } else if (pipe->rga.rotateAngle == 180) {
        rotation = 180;
    } else if (pipe->rga.rotateAngle == 270) {
        rotation = 270;
    }

    memset(&stRgaAttr, 0x00, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = true;
    stRgaAttr.enFlip = flip;
    stRgaAttr.u16Rotaion = rotation;
    stRgaAttr.u16BufPoolCnt = pipe->rga.bufPoolCnt;
    stRgaAttr.stImgIn.u32X = pipe->rga.inputXOffset;
    stRgaAttr.stImgIn.u32Y = pipe->rga.inputYOffset;
    stRgaAttr.stImgIn.imgType = (drm_image_type_e)pipe->rga.rgaInPixFmt;
    stRgaAttr.stImgIn.u32Width = pipe->rga.inputWidth;
    stRgaAttr.stImgIn.u32Height = pipe->rga.inputHeight;
    stRgaAttr.stImgIn.u32HorStride = pipe->rga.inputWidth;
    stRgaAttr.stImgIn.u32VirStride = pipe->rga.inputHeight;
    stRgaAttr.stImgOut.u32X = pipe->rga.outputXOffset;
    stRgaAttr.stImgOut.u32Y = pipe->rga.outputYOffset;
    stRgaAttr.stImgOut.imgType = (drm_image_type_e)pipe->rga.rgaOutPixFmt;
    stRgaAttr.stImgOut.u32Width = pipe->rga.swapWH ? pipe->rga.outputHeight : pipe->rga.outputWidth;
    stRgaAttr.stImgOut.u32Height = pipe->rga.swapWH ? pipe->rga.outputWidth : pipe->rga.outputHeight;
    stRgaAttr.stImgOut.u32HorStride = pipe->rga.swapWH ? pipe->rga.outputHeight : pipe->rga.outputWidth;
    stRgaAttr.stImgOut.u32VirStride = pipe->rga.swapWH ? pipe->rga.outputWidth : pipe->rga.outputHeight;

    int ret = drm_mpi_rga_create_channel(pipe->rga.rgaChannel, &stRgaAttr);
    if (ret) {
        rkmpi_error("create rga channel:[%d] failed, return:[%d]", pipe->rga.rgaChannel, ret);
        return ret;
    }

    ret = drm_mpi_system_set_media_buffer_depth(MOD_ID_RGA, pipe->rga.rgaChannel, pipe->rga.bufDepth);
    if (ret) {
        rkmpi_error("rga channel:[%d] set buff depth failed, return:[%d]", pipe->rga.rgaChannel, ret);
    }

    return 0;
}

int rkpipe_release_rga(rkpipe_t *pipe)
{
    return drm_mpi_rga_destroy_channel(pipe->rga.rgaChannel);
}

API_END_NAMESPACE(mpi)
