#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define __AXERA_PIPELINE_HPP_INSIDE__
#include "../../axapi.h"
#include "common_vo.hpp"
#undef __AXERA_PIPELINE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

int common_axvo_stop_device(uint32_t voDev)
{
    int ret = AX_VO_Disable((VO_DEV)voDev);
    if (ret != 0) {
        axmpi_error("disable vo device:[%d] failed, return:[%d]", voDev, ret);
        return -1;
    }

    return 0;
}

int common_axvo_start_device(uint32_t voDev, axvo_pub_attr_t *pstPubAttr)
{
    int ret = AX_VO_SetPubAttr((VO_DEV)voDev, (const VO_PUB_ATTR_S *)pstPubAttr);
    if (ret != 0) {
        axmpi_error("set vo device:[%d] attribute failed, return:[%d]", voDev, ret);
        return -1;
    }

    ret = AX_VO_Enable((VO_DEV)voDev);
    if (ret != 0) {
        axmpi_error("enable vo device:[%d] failed, return:[%d]", voDev, ret);
        return -1;
    }

    return 0;
}

int common_axvo_stop_layer(uint32_t voLayer)
{
    int ret = AX_VO_DisableVideoLayer((VO_LAYER)voLayer);
    if (ret != 0) {
        axmpi_error("diable vo layer:[%d] failed, return:[%d]", voLayer, ret);
        return -1;
    }

    return 0;
}

int common_axvo_start_layer(uint32_t voLayer, const axvo_video_layer_attr_t *pstLayerAttr)
{
    int ret = AX_VO_SetVideoLayerAttr((VO_LAYER)voLayer, (const VO_VIDEO_LAYER_ATTR_S *)pstLayerAttr);
    if (ret != 0) {
        axmpi_error("set vo layer:[%d] attribute failed, return:[%d]", voLayer, ret);
        return -1;
    }

    ret = AX_VO_EnableVideoLayer((VO_LAYER)voLayer);
    if (ret != 0) {
        axmpi_error("enable vo layer:[%d] failed, return:[%d]", voLayer, ret);
        return -1;
    }

    return 0;
}

int common_axvo_stop_channel(uint32_t voLayer, int enMode)
{
    int wndNum = 0;

    switch (enMode) {
        case VO_MODE_1MUX:
            wndNum = 1;
            break;

        case VO_MODE_2MUX:
            wndNum = 2;
            break;

        case VO_MODE_4MUX:
            wndNum = 4;
            break;

        case VO_MODE_2X4:
        case VO_MODE_8MUX:
            wndNum = 8;
            break;

        case VO_MODE_9MUX:
            wndNum = 9;
            break;

        case VO_MODE_16MUX:
            wndNum = 16;
            break;

        case VO_MODE_25MUX:
            wndNum = 25;
            break;

        case VO_MODE_36MUX:
            wndNum = 36;
            break;

        case VO_MODE_49MUX:
            wndNum = 49;
            break;

        case VO_MODE_64MUX:
            wndNum = 64;
            break;

        default:
            axmpi_warn("Unsupported mode:[%d]", enMode);
            return -1;
    }

    for (int i = 0; i < wndNum; i++) {
        int ret = AX_VO_DisableChn((VO_LAYER)voLayer, (VO_CHN)i);
        if (ret != 0) {
            axmpi_error("diable vo layer:[%d] channel:[%d] failed, return:[%d]", voLayer, i, ret);
            return -2;
        }
    }

    return 0;
}

int common_axvo_start_channel(uint32_t voLayer, int enMode, uint32_t u32FifoDepth)
{
    int row = 0, col = 0;
    int wndNum = 0, square = 0;

    switch (enMode) {
        case VO_MODE_1MUX:
            wndNum = 1;
            square = 1;
            break;

        case VO_MODE_2MUX:
            wndNum = 2;
            square = 2;
            break;

        case VO_MODE_4MUX:
            wndNum = 4;
            square = 2;
            break;

        case VO_MODE_8MUX:
            wndNum = 8;
            square = 3;
            break;

        case VO_MODE_9MUX:
            wndNum = 9;
            square = 3;
            break;

        case VO_MODE_16MUX:
            wndNum = 16;
            square = 4;
            break;

        case VO_MODE_25MUX:
            wndNum = 25;
            square = 5;
            break;

        case VO_MODE_36MUX:
            wndNum = 36;
            square = 6;
            break;

        case VO_MODE_49MUX:
            wndNum = 49;
            square = 7;
            break;

        case VO_MODE_64MUX:
            wndNum = 64;
            square = 8;
            break;

        case VO_MODE_2X4:
            wndNum = 8;
            square = 3;
            row = 4;
            col = 2;
            break;

        default:
            axmpi_warn("Unsupported mode:[%d]", enMode);
            return -1;
    }

    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    int ret = AX_VO_GetVideoLayerAttr((VO_LAYER)voLayer, &stLayerAttr);
    if (ret != 0) {
        axmpi_error("get vo layer:[%d] attribute failed, return:[%d]", voLayer, ret);
        return -2;
    }

    VO_CHN_ATTR_S stChnAttr;
    uint32_t u32Width = stLayerAttr.stImageSize.u32Width;
    uint32_t u32Height = stLayerAttr.stImageSize.u32Height;

    for (int i = 0; i < wndNum; ++i) {
        if ((enMode == VO_MODE_1MUX)
            || (enMode == VO_MODE_2MUX)
            || (enMode == VO_MODE_4MUX)
            || (enMode == VO_MODE_8MUX)
            || (enMode == VO_MODE_9MUX)
            || (enMode == VO_MODE_16MUX)
            || (enMode == VO_MODE_25MUX)
            || (enMode == VO_MODE_36MUX)
            || (enMode == VO_MODE_49MUX)
            || (enMode == VO_MODE_64MUX))
        {
            stChnAttr.stRect.u32X = ALIGN_DOWN((u32Width / square) * (i % square), 16);
            stChnAttr.stRect.u32Y = ALIGN_DOWN((u32Height / square) * (i / square), 16);
            stChnAttr.stRect.u32Width = ALIGN_DOWN(u32Width / square, 16);
            stChnAttr.stRect.u32Height = ALIGN_DOWN(u32Height / square, 2);
        } else if (enMode == VO_MODE_2X4) {
            stChnAttr.stRect.u32X = ALIGN_DOWN((u32Width / col) * (i % col), 16);
            stChnAttr.stRect.u32Y = ALIGN_DOWN((u32Height / row) * (i / col), 16);
            stChnAttr.stRect.u32Width = ALIGN_DOWN(u32Width / col, 16);
            stChnAttr.stRect.u32Height = ALIGN_DOWN(u32Height / row, 2);
        }

        stChnAttr.u32FifoDepth = u32FifoDepth;
        ret = AX_VO_SetChnAttr((VO_LAYER)voLayer, (VO_CHN)i, (const VO_CHN_ATTR_S *)&stChnAttr);
        if (ret != 0) {
            axmpi_error("set vo layer:[%d] channel:[%d] attribute failed, return:[%d]", voLayer, i, ret);
            return -3;
        }

        ret = AX_VO_EnableChn((VO_LAYER)voLayer, (VO_CHN)i);
        if (ret != 0) {
            axmpi_error("enable vo layer:[%d] channel:[%d] failed, return:[%d]", voLayer, i, ret);
            return -4;
        }
    }

    return 0;
}

int common_axvo_stop(axvo_config_t *pstVoConfig)
{
    if (!pstVoConfig) {
        axmpi_error("Invalid parameter, maybe nullptr");
        return -1;
    }

    uint32_t voDev = 0;
    uint32_t voLayer = 0;
    int sEnableGLayer = 0;
    uint32_t graphicLayer = 0;
    int enVoMode = VO_MODE_BUTT;
    axvo_layer_config_t *pstVoLayerConf = NULL;

    voDev = pstVoConfig->voDev;
    graphicLayer = pstVoConfig->graphicLayer;
    sEnableGLayer = pstVoConfig->s32EnableGLayer;

    for (int i = 0; i < pstVoConfig->u32LayerNr; ++i) {
        pstVoLayerConf = &pstVoConfig->stVoLayer[i];
        voLayer = pstVoLayerConf->voLayer;
        enVoMode = pstVoLayerConf->enVoMode;

        common_axvo_stop_channel(voLayer, enVoMode);
        common_axvo_stop_layer(voLayer);
        AX_VO_UnBindVideoLayer(voLayer, voDev);
    }

    if (sEnableGLayer) {
        AX_VO_UnBindGraphicLayer(graphicLayer, voDev);
    }

    return common_axvo_stop_device(voDev);
}

int common_axvo_start(axvo_config_t *pstVoConfig)
{
    int i;
    int ret = 0;
    int enVoMode = 0;
    uint32_t voDev = 0;
    uint32_t voLayer = 0;
    int sEnableGLayer = 0;
    uint32_t graphicLayer = 0;
    uint32_t u32FifoDepth = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    int enVoIntfType = VO_INTF_DSI0;
    int enIntfSync = VO_OUTPUT_1080P30;
    VO_SIZE_S stDefImageSize = {1920, 1080};
    axvo_layer_config_t *pstVoLayerConf = NULL;
    VO_VIDEO_LAYER_ATTR_S *pstLayerAttr  = NULL;
    VO_RECT_S stDefDispRect = {0, 0, 1920, 1080};

    memset(&stVoPubAttr, 0, sizeof(stVoPubAttr));

    if (!pstVoConfig) {
        axmpi_error("Invalid parameter, maybe nullptr");
        return -1;
    }

    voDev = pstVoConfig->voDev;
    enIntfSync = pstVoConfig->enIntfSync;
    enVoIntfType = pstVoConfig->enVoIntfType;
    graphicLayer = pstVoConfig->graphicLayer;
    u32FifoDepth = pstVoConfig->u32FifoDepth;
    sEnableGLayer = pstVoConfig->s32EnableGLayer;

    stVoPubAttr.stReso = pstVoConfig->stReso;
    stVoPubAttr.enIntfSync = (VO_INTF_SYNC_E)enIntfSync;
    stVoPubAttr.enIntfType = (VO_INTF_TYPE_E)enVoIntfType;

    ret = common_axvo_start_device(voDev, &stVoPubAttr);
    if (ret != 0) {
        axmpi_error("common_axvo_start_device failed, return:[%d]", ret);
        return -2;
    }

    for (i = 0; i < pstVoConfig->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConfig->stVoLayer[i];
        voLayer = pstVoLayerConf->voLayer;
        pstLayerAttr = &pstVoLayerConf->stVoLayerAttr;
        enVoMode = pstVoLayerConf->enVoMode;

        pstLayerAttr->u32FifoDepth = pstVoConfig->u32FifoDepth;
        ret = common_axvo_start_layer(voLayer, pstLayerAttr);
        if (ret != 0) {
            axmpi_error("common_axvo_start_layer failed, return:[%d]", ret);
            goto failed_exit;
        }

        ret = common_axvo_start_channel(voLayer, enVoMode, u32FifoDepth);
        if (ret != 0) {
            common_axvo_stop_layer(voLayer);
            axmpi_error("common_axvo_start_channel failed, return:[%d]", ret);
            goto failed_exit;
        }

        ret = AX_VO_BindVideoLayer((VO_LAYER)voLayer, (VO_DEV)voDev);
        if (ret != 0) {
            common_axvo_stop_channel(voLayer, enVoMode);
            common_axvo_stop_layer(voLayer);
            axmpi_error("AX_VO_BindVideoLayer failed, return:[%d]", ret);
            goto failed_exit;
        }
    }

    if (sEnableGLayer) {
        ret = AX_VO_BindGraphicLayer((GRAPHIC_LAYER)graphicLayer, (VO_DEV)voDev);
        if (ret != 0) {
            axmpi_error("AX_VO_BindGraphicLayer failed, return:[%d]", ret);
        }
    }

failed_exit:
    if (ret) {
        for (i -= 1; i >= 0; i--) {
            pstVoLayerConf = &pstVoConfig->stVoLayer[i];
            voLayer = pstVoLayerConf->voLayer;
            enVoMode= pstVoLayerConf->enVoMode;

            common_axvo_stop_channel(voLayer, enVoMode);
            common_axvo_stop_layer(voLayer);
            AX_VO_UnBindVideoLayer((VO_LAYER)voLayer, (VO_DEV)voDev);
        }

        common_axvo_stop_device(voDev);
    }

    return 0;
}

API_END_NAMESPACE(media)
