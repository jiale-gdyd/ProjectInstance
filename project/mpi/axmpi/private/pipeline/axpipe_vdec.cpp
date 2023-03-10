#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <ax_sys_api.h>
#include <ax_vdec_api.h>
#include <ax_buffer_tool.h>

#include "private.hpp"
#include "../utilities/log.hpp"

namespace axpi {
static int framePoolInit(int grpId, uint32_t frameSize, uint32_t *poolId)
{
    int ret = 0;
    uint32_t u32PoolId = 0;
    AX_POOL_CONFIG_T stPoolConfig;

    memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
    stPoolConfig.MetaSize = 512;
    stPoolConfig.BlkCnt = 10;
    stPoolConfig.BlkSize = frameSize;
    stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
    memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
    strcpy((char *)stPoolConfig.PartitionName, "anonymous");

    u32PoolId = AX_POOL_CreatePool(&stPoolConfig);
    if (u32PoolId == AX_INVALID_POOLID) {
        axmpi_error("AX_POOL_CreatePool failed, return:[%X]", u32PoolId);
        return -1;
    }

    *poolId = u32PoolId;
    ret = AX_VDEC_AttachPool(grpId, u32PoolId);
    if (ret != 0) {
        AX_POOL_MarkDestroyPool(u32PoolId);
        axmpi_error("attach vdec group:[%d] to pool:[%X] failed, return:[%d]", grpId, u32PoolId, ret);
        return -2;
    }

    return 0;
}

int axpipe_create_vdec(axpipe_t *pipe)
{
    if (pipe->vdec.channel >= VDEC_GRP_COUNT) {
        axmpi_error("Invalid vdec group:[%d], only suppord:[%d:%d]", pipe->vdec.channel, 0, VDEC_GRP_COUNT - 1);
        return -1;
    }

    AX_VDEC_GRP_ATTR_S gGrpAttr;
    memset(&gGrpAttr, 0, sizeof(AX_VDEC_GRP_ATTR_S));

    switch (pipe->inType) {
        case AXPIPE_INPUT_VDEC_H264: {
            gGrpAttr.enType = PT_H264;
            gGrpAttr.u32PicWidth = 1920;
            gGrpAttr.u32PicHeight = 1080;
            gGrpAttr.u32StreamBufSize = 8 * 1024 * 1024;
            gGrpAttr.u32FrameBufCnt = 10;
            gGrpAttr.enLinkMode = AX_LINK_MODE;

            uint32_t u32PoolId;
            uint32_t frameSize = 0;

            int ret = AX_VDEC_CreateGrp(pipe->vdec.channel, &gGrpAttr);
            if (ret != 0) {
                axmpi_error("vdec group:[%d] create failed, return:[%d]", pipe->vdec.channel, ret);
                return -2;
            }

            frameSize = AX_VDEC_GetPicBufferSize(1920, 108, PT_H264);
            ret = framePoolInit(pipe->vdec.channel, frameSize, &u32PoolId);
            if (ret != 0) {
                axmpi_error("framePoolInit failed, return:[%d]", ret);
                return -3;
            }

            pipe->vdec.poolId = u32PoolId;
            ret = AX_VDEC_StartRecvStream(pipe->vdec.channel);
            if (ret != 0) {
                AX_VDEC_DestroyGrp(pipe->vdec.channel);
                axmpi_error("vdec group:[%d] recv stream failed, return:[%d]", pipe->vdec.channel, ret);
                return -4;
            }

            break;
        }

        case AXPIPE_INPUT_VDEC_JPEG: {
            uint32_t u32PoolId;
            AX_POOL_CONFIG_T stPoolConfig;
            uint32_t frameSize = AX_VDEC_GetPicBufferSize(4096, 4096, PT_JPEG);

            memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
            stPoolConfig.MetaSize = 512;
            stPoolConfig.BlkCnt = 10;
            stPoolConfig.BlkSize = frameSize;
            stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
            memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
            strcpy((char *)stPoolConfig.PartitionName, "anonymous");

            u32PoolId = AX_POOL_CreatePool(&stPoolConfig);
            if (u32PoolId == AX_INVALID_POOLID) {
                axmpi_error("AX_POOL_CreatePool failed");
                return -5;
            }

            pipe->vdec.poolId = u32PoolId;
            break;
        }

        default:
            break;
    }

    return 0;
}

int axpipe_release_vdec(axpipe_t *pipe)
{
    axpipe_release_jdec(pipe);
    AX_POOL_MarkDestroyPool(pipe->vdec.poolId);

    return 0;
}

int axpipe_create_jdec(axpipe_t *pipe)
{
    if (pipe->vdec.channel >= VDEC_GRP_COUNT) {
        axmpi_error("Invalid vdec group:[%d], only suppord:[%d:%d]", pipe->vdec.channel, 0, VDEC_GRP_COUNT - 1);
        return -1;
    }

    AX_VDEC_GRP_ATTR_S gGrpAttr;
    memset(&gGrpAttr, 0, sizeof(AX_VDEC_GRP_ATTR_S));

    gGrpAttr.enType = PT_JPEG;
    gGrpAttr.u32PicWidth = 1920;
    gGrpAttr.u32PicHeight = 1080;
    gGrpAttr.u32StreamBufSize = 8 * 1024 * 1024;
    gGrpAttr.u32FrameBufCnt = 10;
    gGrpAttr.enLinkMode = AX_LINK_MODE;

    int ret = AX_VDEC_CreateGrp(pipe->vdec.channel, &gGrpAttr);
    if (ret != 0) {
        axmpi_error("vdec group:[%d] create failed, return:[%d]", pipe->vdec.channel, ret);
        return -2;
    }

    ret = AX_VDEC_AttachPool(pipe->vdec.channel, pipe->vdec.poolId);
    if (ret != 0) {
        axmpi_error("vdec group:[%d] attach to poolId:[%X] failed, return:[%d]", pipe->vdec.channel, pipe->vdec.poolId, ret);
        return -3;
    }

    ret = AX_VDEC_StartRecvStream(pipe->vdec.channel);
    if (ret != 0) {
        AX_VDEC_DestroyGrp(pipe->vdec.channel);
        axmpi_error("vdec group:[%d] start recv stream failed, return:[%d]", pipe->vdec.channel, ret);
        return -4;
    }

    return 0;
}

int axpipe_release_jdec(axpipe_t *pipe)
{
    AX_VDEC_StopRecvStream(pipe->vdec.channel);
    AX_VDEC_DetachPool(pipe->vdec.channel);
    AX_VDEC_DestroyGrp(pipe->vdec.channel);

    return 0;
}
}
