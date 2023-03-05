#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "../axmpi.h"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

#include "common_camera.hpp"

API_BEGIN_NAMESPACE(media)

int axcam_init()
{
    int axRet = AX_VIN_Init();
    if (0 != axRet) {
        axmpi_error("AX_VIN_Init failed, return:[%d]", axRet);
        return -1;
    }

    axRet = AX_MIPI_RX_Init();
    if (0 != axRet) {
        axmpi_error("AX_MIPI_RX_Init failed, return:[%d]", axRet);
        return -2;
    }

    return 0;
}

int axcam_deinit()
{
    AX_MIPI_RX_DeInit();

    int axRet = AX_VIN_Deinit();
    if (0 != axRet) {
        axmpi_error("AX_VIN_DeInit failed, return:[%d]", axRet);
        return -1;
    }

    return axRet;
}

int axcam_open(axcam_t *cam)
{
    int axRet;
    int nRxDev = cam->nRxDev;
    int eSnsType = cam->eSnsType;
    uint8_t nDevId = cam->nDevId;
    uint8_t nPipeId = cam->nPipeId;
    AX_PIPE_ATTR_T tPipeAttr = {0};
    AX_VIN_SNS_DUMP_ATTR_T tDumpAttr;
    char *pFile = cam->szTuningFileName;
    AX_VIN_DEV_BIND_PIPE_T tDevBindPipe = {0};

    memset(&tDumpAttr, 0, sizeof(tDumpAttr));

    tDevBindPipe.nNum = 1;
    tDevBindPipe.nPipeId[0] = nPipeId;

    axRet = AX_VIN_Create(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_Create failed, rerturn:[%d]", nPipeId, axRet);
        return -1;
    }

    axRet = axisp_regsiter_sns(nPipeId, nDevId, eSnsType);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d], nDevId:[%d] COMMON_ISP_RegisterSns failed, rerturn:[%d]", nPipeId, nDevId, axRet);
        return -2;
    }

    axRet = AX_VIN_SetRunMode(nPipeId, AX_ISP_PIPELINE_NORMAL);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_SetRunMode failed, rerturn:[%d]", nPipeId, axRet);
        return -3;
    }

    axRet = AX_VIN_SetSnsAttr(nPipeId, &cam->stSnsAttr);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_SetSnsAttr failed, rerturn:[%d]", nPipeId, axRet);
        return -4;
    }

    axRet = AX_VIN_OpenSnsClk(nPipeId, cam->stSnsClkAttr.nSnsClkIdx, (AX_SNS_CLK_RATE_E)cam->stSnsClkAttr.eSnsClkRate);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_OpenSnsClk failed, rerturn:[%d]", nPipeId, axRet);
        return -5;
    }

    axRet = AX_VIN_SetDevAttr(nDevId, &cam->stDevAttr);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_SetDevAttr failed, rerturn:[%d]", nDevId, axRet);
        return -6;
    }

    axRet = axisp_set_mipi_attr(nRxDev, eSnsType, false);
    if (0 != axRet) {
        axmpi_error("nRxDev:[%d] AX_MIPI_RX_SetAttr failed, rerturn:[%d]", nRxDev, axRet);
        return -7;
    }

    axRet = AX_VIN_SetChnAttr(nPipeId, &cam->stChnAttr);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_SetChnAttr failed, rerturn:[%d]", nPipeId, axRet);
        return -8;
    }

    axRet = AX_VIN_SetPipeAttr(nPipeId, &cam->stPipeAttr);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VI_SetPipeAttr failed, rerturn:[%d]", nPipeId, axRet);
        return -9;
    }

    axRet = AX_VIN_SetDevBindPipe(nDevId, &tDevBindPipe);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_SetDevBindPipe failed, rerturn:[%d]", nDevId, axRet);
        return -10;
    }

    axRet = AX_ISP_Open(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_ISP_Open failed, rerturn:[%d]", nPipeId, axRet);
        return -11;
    }

    axRet = axisp_register_ae_alglib(nPipeId, eSnsType, cam->bUser3a, &cam->tAeFuncs);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] RegisterAeAlgLib failed, rerturn:[%d]", nPipeId, axRet);
        return -12;
    }

    axRet = axisp_register_awb_alglib(nPipeId, eSnsType, cam->bUser3a,  &cam->tAwbFuncs);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] RegisterAwbAlgLib failed, rerturn:[%d]", nPipeId, axRet);
        return -13;
    }

    axRet = axisp_register_lsc_alglib(nPipeId, eSnsType, cam->bUser3a, &cam->tLscFuncs);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] RegisterLscAlgLib failed, rerturn:[%d]", nPipeId, axRet);
        return -14;
    }

    axRet = AX_ISP_LoadBinParams(nPipeId, pFile);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_ISP_LoadBinParams %s will user sensor.h", nPipeId, pFile);
        // return -15;
    }

    axRet = AX_VIN_Start(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_Start failed, rerturn:[%d]", nPipeId, axRet);
        return -16;
    }

    if (cam->eSrcType != AX_PIPE_FRAME_SOURCE_TYPE_DEV) {
        axRet = AX_VIN_SetPipeFrameSource(nPipeId, (AX_ISP_PIPE_FRAME_SOURCE_ID_E)cam->eSrcId, (AX_ISP_PIPE_FRAME_SOURCE_TYPE_E)cam->eSrcType);
        if (0 != axRet) {
            axmpi_error("nPipeId:[%d] AX_VIN_SetPipeFrameSource failed, rerturn:[%d]", nPipeId, axRet);
            return -17;
        }
    }

    axRet = AX_VIN_EnableDev(nDevId);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_EnableDev failed, rerturn:[%d]", nDevId, axRet);
        return -18;
    }

    AX_VIN_GetPipeAttr(nPipeId, &tPipeAttr);

    if (AX_PIPE_SOURCE_DEV_OFFLINE == tPipeAttr.ePipeDataSrc) {
        tDumpAttr.bEnable = AX_TRUE;
        tDumpAttr.nDepth = 2;
        axRet = AX_VIN_SetSnsDumpAttr(nDevId, &tDumpAttr);
        if (0 != axRet) {
            axmpi_error("nDevId:[%d] AX_VIN_SetSnsDumpAttr failed, rerturn:[%d]", nDevId, axRet);
            return -19;
        }
    }

    axRet = AX_VIN_StreamOn(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_StreamOn failed, rerturn:[%d]", nPipeId, axRet);
        return -20;
    }

    return 0;
}

int axcam_close(axcam_t *cam)
{
    int axRet = 0;
    AX_PIPE_ATTR_T tPipeAttr;
    uint8_t nDevId = cam->nDevId;
    uint8_t nPipeId = cam->nPipeId;
    AX_VIN_SNS_DUMP_ATTR_T tDumpAttr;

    memset(&tPipeAttr, 0, sizeof(tPipeAttr));
    memset(&tDumpAttr, 0, sizeof(tDumpAttr));

    AX_VIN_StreamOff(nPipeId);
    AX_VIN_GetPipeAttr(nPipeId, &tPipeAttr);

    if (AX_PIPE_SOURCE_DEV_OFFLINE == tPipeAttr.ePipeDataSrc) {
        tDumpAttr.bEnable = AX_FALSE;
        axRet = AX_VIN_SetSnsDumpAttr(nDevId, &tDumpAttr);
        if (0 != axRet) {
            axmpi_error("nDevId:[%d] AX_VIN_SetSnsDumpAttr failed, rerturn:[%d]", nDevId, axRet);
            return -1;
        }
    }

    axRet = AX_VIN_CloseSnsClk(cam->stSnsClkAttr.nSnsClkIdx);
    if (0 != axRet) {
        axmpi_error("AX_VIN_CloseSnsClk failed, rerturn:[%d]", axRet);
        return -2;
    }

    axRet = AX_VIN_DisableDev(nDevId);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_DisableDev failed, rerturn:[%d]", nDevId, axRet);
        return -3;
    }

    axRet = AX_VIN_Stop(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_Stop failed, rerturn:[%d]", nPipeId, axRet);
        return -4;
    }

    axisp_unregister_ae_alglib(nPipeId);
    axisp_unregister_awb_alglib(nPipeId);
    if (cam->bUser3a) {
        axisp_unregister_lsc_alglib(nPipeId);
    }

    axRet = AX_ISP_Close(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_ISP_Close failed, rerturn:[%d]", nPipeId, axRet);
        return -5;
    }

    axisp_unregsiter_sns(nPipeId);
    AX_VIN_Destory(nPipeId);

    return 0;
}

int axcam_dvp_open(axcam_t *cam)
{
    int axRet;
    int nRxDev = cam->nRxDev;
    int eSnsType = cam->eSnsType;
    uint8_t nDevId = cam->nDevId;
    uint8_t nPipeId = cam->nPipeId;
    AX_VIN_SNS_DUMP_ATTR_T tDumpAttr;
    AX_VIN_DEV_BIND_PIPE_T tDevBindPipe;

    memset(&tDumpAttr, 0, sizeof(tDumpAttr));
    memset(&tDevBindPipe, 0, sizeof(tDevBindPipe));

    tDevBindPipe.nNum = 1;
    tDevBindPipe.nPipeId[0] = nPipeId;

    axRet = AX_VIN_Create(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_Create failed, rerturn:[%d]", nPipeId, axRet);
        return -1;
    }

    if (eSnsType == MIPI_YUV){
        axRet = axisp_set_mipi_attr(nRxDev, eSnsType, AX_TRUE);
        if (0 != axRet) {
            axmpi_error("axisp_set_mipi_attr failed, rerturn:[%d]", axRet);
            return -2;
        }
    }

    axRet = AX_VIN_SetRunMode(nPipeId, AX_ISP_PIPELINE_NONE_NPU);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_SetRunMode failed, rerturn:[%d]", nPipeId, axRet);
        return -3;
    }

    axRet = AX_VIN_SetDevAttr(nDevId, &cam->stDevAttr);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_SetDevAttr failed, rerturn:[%d]", nDevId, axRet);
        return -4;
    }

    axRet = AX_VIN_SetChnAttr(nPipeId, &cam->stChnAttr);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_SetChnAttr failed, rerturn:[%d]", nPipeId, axRet);
        return -5;
    }

    axRet = AX_VIN_SetPipeAttr(nPipeId, &cam->stPipeAttr);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VI_SetPipeAttr failed, rerturn:[%d]", nPipeId, axRet);
        return -6;
    }

    axRet = AX_VIN_SetDevBindPipe(nDevId, &tDevBindPipe);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_SetDevBindPipe failed, rerturn:[%d]", nDevId, axRet);
        return -7;
    }

    axRet = AX_ISP_Open(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_ISP_Open failed, rerturn:[%d]", nPipeId, axRet);
        return -8;
    }

    axRet = AX_VIN_Start(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_Start failed, rerturn:[%d]", nPipeId, axRet);
        return -9;
    }

    axRet = AX_VIN_EnableDev(nDevId);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_EnableDev failed, rerturn:[%d]", nDevId, axRet);
        return -10;
    }

    tDumpAttr.nDepth = 2;
    tDumpAttr.bEnable = AX_TRUE;

    axRet = AX_VIN_SetSnsDumpAttr(nDevId, &tDumpAttr);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_SetSnsDumpAttr failed, rerturn:[%d]", nDevId, axRet);
        return -11;
    }

    return 0;
}

int axcam_dvp_close(axcam_t *cam)
{
    int axRet = 0;
    uint8_t nDevId = cam->nDevId;
    uint8_t nPipeId = cam->nPipeId;

    axRet = AX_VIN_DisableDev(nDevId);
    if (0 != axRet) {
        axmpi_error("nDevId:[%d] AX_VIN_DisableDev failed, return:[%d]", nDevId, axRet);
        return -1;
    }

    axRet = AX_VIN_Stop(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_VIN_Stop failed, return:[%d]", nPipeId, axRet);
        return -2;
    }

    axRet = AX_ISP_Close(nPipeId);
    if (0 != axRet) {
        axmpi_error("nPipeId:[%d] AX_ISP_Close failed, return:[%d]", nPipeId, axRet);
        return -3;
    }

    AX_VIN_Destory(nPipeId);
    return 0;
}

API_END_NAMESPACE(media)
