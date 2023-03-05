#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define __AXERA_PIPELINE_HPP_INSIDE__
#include "private.hpp"
#include "vo/common_vo.hpp"
#undef __AXERA_PIPELINE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

#define AXVO_DEV0               0

typedef struct {
    pthread_t tid;
    bool      forceStop;
    uint32_t  layerId;
    uint32_t  channelId;
    uint32_t  userPoolId;
} axvo_chn_thread_param_t;

static axvo_config_t sg_stVoConfig;
static axvo_chn_thread_param_t sg_stChnThreadParam = {0};

static int parseVoPubAttr(const char *str, axvo_config_t *pstVoConf)
{
    char *p, *end;

    if (!str || !pstVoConf) {
        axmpi_error("Invalid parameters");
        return -1;
    }

    p = (char *)str;
    if (strstr(p, "dpi")) {
        pstVoConf->enVoIntfType = VO_INTF_DPI;
    } else if (strstr(p, "dsi0")) {
        pstVoConf->enVoIntfType = VO_INTF_DSI0;
    } else if (strstr(p, "dsi1")) {
        pstVoConf->enVoIntfType = VO_INTF_DSI1;
    } else if (strstr(p, "2dsi")) {
        pstVoConf->enVoIntfType = VO_INTF_2DSI;
        pstVoConf->u32LayerNr = 2;
    } else if (strstr(p, "bt656")) {
        pstVoConf->enVoIntfType = VO_INTF_BT656;
    } else if (strstr(p, "bt1120")) {
        pstVoConf->enVoIntfType = VO_INTF_BT1120;
    } else {
        axmpi_error("Unsupported interface type:[%s]", p);
        return -2;
    }

    end = strstr(p, "@");
    p = end + 1;
    pstVoConf->stReso.u32Width = strtoul(p, &end, 10);

    end = strstr(p, "x");
    p = end + 1;
    pstVoConf->stReso.u32Height = strtoul(p, &end, 10);

    end = strstr(p, "@");
    p = end + 1;
    pstVoConf->stReso.u32RefreshRate = strtoul(p, &end, 10);

    return 0;
}

static int axoReleasePool(uint32_t poolId)
{
    int ret = AX_POOL_MarkDestroyPool(poolId);
    if (ret != 0) {
        axmpi_error("AX_POOL_MarkDestroyPool failed, return:[%d]", ret);
        return -1;
    }

    return 0;
}

static int axvoCreatePool(uint32_t blkCnt, uint64_t blkSize, uint64_t metaSize, uint32_t *poolId)
{
    AX_POOL_CONFIG_T stPoolCfg;

    memset(&stPoolCfg, 0, sizeof(stPoolCfg));
    stPoolCfg.MetaSize = metaSize;
    stPoolCfg.BlkCnt = blkCnt;
    stPoolCfg.BlkSize = blkSize;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((char *)stPoolCfg.PartitionName, "anonymous");

    *poolId = AX_POOL_CreatePool(&stPoolCfg);
    if (*poolId == AX_INVALID_POOLID) {
        axmpi_error("AX_POOL_CreatePool failed");
        return -1;
    }

    return 0;
}

int axpipe_release_vo()
{
    axvo_layer_config_t *pstVoLayer;
    axvo_video_layer_attr_t *pstVoLayerAttr;
    axvo_config_t *pstVoConf = &sg_stVoConfig;
    axvo_chn_thread_param_t *pstChnThreadParam = &sg_stChnThreadParam;

    common_axvo_stop(pstVoConf);
    AX_VO_Deinit();

    for (int i = 0; i < pstVoConf->u32LayerNr; ++i) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (i != 0) {
            if (pstChnThreadParam->userPoolId != ~0) {
                axoReleasePool(pstChnThreadParam->userPoolId);
            }
        }

        if (pstVoLayerAttr->u32PoolId != ~0) {
            axoReleasePool(pstVoLayerAttr->u32PoolId);
        }
    }

    AX_SYS_Deinit();
    return 0;
}

int axpipe_create_vo(std::string str, axpipe_t *pipe)
{
    memset(&sg_stVoConfig, 0, sizeof(sg_stVoConfig));

    //  device
    sg_stVoConfig.voDev = AXVO_DEV0;
    sg_stVoConfig.enVoIntfType = VO_INTF_DSI0;
    sg_stVoConfig.enIntfSync = VO_OUTPUT_USER;
    sg_stVoConfig.stReso.u32Width = 1280;
    sg_stVoConfig.stReso.u32Height = 800;
    sg_stVoConfig.stReso.u32RefreshRate = 45;
    sg_stVoConfig.u32LayerNr = 1;

    // layer0
    sg_stVoConfig.stVoLayer[0].voLayer = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.stDispRect = {0, 0, 1280, 800};
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.stImageSize = {1280, 800};
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.enPixFmt = AX_YUV420_SEMIPLANAR;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.enLayerSync = VO_LAYER_SYNC_PRIMARY;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32PrimaryChnId = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32FrameRate = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32FifoDepth = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32ChnNr = 2;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32BkClr = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.enLayerBuf = VO_LAYER_OUT_BUF_POOL;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32InplaceChnId = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u32PoolId = 0;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.enDispatchMode = VO_LAYER_OUT_TO_FIFO;
    sg_stVoConfig.stVoLayer[0].stVoLayerAttr.u64KeepChnPrevFrameBitmap = 0x1;
    sg_stVoConfig.stVoLayer[0].enVoMode = VO_MODE_1MUX;

    // layer1
    sg_stVoConfig.stVoLayer[1].voLayer = 1;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.stDispRect = {0, 0, 1280, 800};
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.stImageSize = {1280, 800};
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.enPixFmt = AX_YUV420_SEMIPLANAR;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.enLayerSync = VO_LAYER_SYNC_NORMAL;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32PrimaryChnId = 0;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32FrameRate = 0;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32FifoDepth = 0;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32ChnNr = 16;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32BkClr = 0;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.enLayerBuf = VO_LAYER_OUT_BUF_POOL;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32InplaceChnId = 0;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u32PoolId = 0;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.enDispatchMode = VO_LAYER_OUT_TO_FIFO;
    sg_stVoConfig.stVoLayer[1].stVoLayerAttr.u64KeepChnPrevFrameBitmap = ~0x0;
    sg_stVoConfig.stVoLayer[1].enVoMode = VO_MODE_1MUX;

    int i, ret;
    uint64_t blkSize = 0;
    axvo_config_t *pstVoConf;
    axvo_layer_config_t *pstVoLayer;
    axvo_video_layer_attr_t *pstVoLayerAttr;
    axvo_chn_thread_param_t *pstChnThreadParam = &sg_stChnThreadParam;

    ret = parseVoPubAttr(str.c_str(), &sg_stVoConfig);
    if (ret != 0) {
        return -1;
    }

    pstVoConf = &sg_stVoConfig;
    ret = AX_SYS_Init();
    if (ret != 0) {
        axmpi_error("AX_SYS_Init failed, return:[%d]", ret);
        return -2;
    }

    ret = AX_VO_Init();
    if (ret != 0) {
        axmpi_error("AX_VO_Init failed, return:[%d]", ret);
        goto FAILED_EXIT0;
    }

    for (i = 0; i < pstVoConf->u32LayerNr; ++i) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (i == 0) {
            switch (pipe->ivpsConfig.ivpsRotate) {
                case 90:
                case 270:
                    pstVoLayerAttr->stImageSize.u32Width = pipe->ivpsConfig.ivpsHeight;
                    pstVoLayerAttr->stImageSize.u32Height = pipe->ivpsConfig.ivpsWidth;
                    break;

                default:
                    pstVoLayerAttr->stImageSize.u32Width = pipe->ivpsConfig.ivpsWidth;
                    pstVoLayerAttr->stImageSize.u32Height = pipe->ivpsConfig.ivpsHeight;
                    break;
            }
        } else {
            pstVoLayer->enVoMode = VO_MODE_1MUX;
            pstVoLayerAttr->stImageSize.u32Width = (pstVoConf->stReso.u32Width + 0xF) & (~0xF);
            pstVoLayerAttr->stImageSize.u32Height = pstVoConf->stReso.u32Height;
            pstVoLayerAttr->stDispRect.u32X = pstVoConf->stReso.u32Width;
            pstVoLayerAttr->u32ChnNr = 1;
        }

        pstVoLayerAttr->stDispRect.u32Width = pstVoLayerAttr->stImageSize.u32Width;
        pstVoLayerAttr->stDispRect.u32Height = pstVoLayerAttr->stImageSize.u32Height;

        pstVoLayerAttr->u32PoolId = ~0;
        blkSize = pstVoLayerAttr->stImageSize.u32Width * pstVoLayerAttr->stImageSize.u32Height * 3 / 2;
        ret = axvoCreatePool(3, blkSize, 512, &pstVoLayerAttr->u32PoolId);
        if (ret != 0) {
            axmpi_error("axvoCreatePool failed, return:[%d]", ret);
            goto FAILED_EXIT1;
        }
    }

    ret = common_axvo_start(pstVoConf);
    if (ret != 0) {
        axmpi_error("common_axvo_start failed, return:[%d]", ret);
        goto FAILED_EXIT1;
    }

    return 0;

FAILED_EXIT1:
    for (i = 0; i < pstVoConf->u32LayerNr; ++i) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (pstVoLayerAttr->u32PoolId != ~0) {
            axoReleasePool(pstVoLayerAttr->u32PoolId);
        }
    }

    AX_VO_Deinit();

FAILED_EXIT0:
    AX_SYS_Deinit();
    return ret;
}

API_END_NAMESPACE(media)
