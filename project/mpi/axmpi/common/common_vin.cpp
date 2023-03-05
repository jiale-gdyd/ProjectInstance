#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "../axmpi.h"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

#include "common_vin.hpp"
#include "common_config.h"
#include "common_camera.hpp"

API_BEGIN_NAMESPACE(media)

extern AX_SENSOR_REGISTER_FUNC_T gSnsgc4653Obj;
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10Obj;
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10SlaveObj;
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10MasterObj;

static int rawFmtToCommFmt(int eRawType)
{
    int img_format = AX_FORMAT_INVALID;

    switch (eRawType) {
        case AX_RT_RAW8:
            img_format = AX_FORMAT_BAYER_RAW_8BPP;
            break;

        case AX_RT_RAW10:
            img_format = AX_FORMAT_BAYER_RAW_10BPP;
            break;

        case AX_RT_RAW12:
            img_format = AX_FORMAT_BAYER_RAW_12BPP;
            break;

        case AX_RT_RAW14:
            img_format = AX_FORMAT_BAYER_RAW_14BPP;
            break;

        case AX_RT_RAW16:
            img_format = AX_FORMAT_BAYER_RAW_16BPP;
            break;

        default:
            axmpi_error("comm not support this data type:[%d]", eRawType);
            img_format = AX_FORMAT_BAYER_RAW_10BPP;
            break;
    }

    return img_format;
}

int axisp_set_mipi_attr(uint8_t devId, int eSnsType, bool bMaster)
{
    int nRet = 0;
    AX_MIPI_RX_ATTR_S tMipiAttr;

    memset(&tMipiAttr, 0, sizeof(tMipiAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tMipiAttr, &gOs04a10MipiAttr, sizeof(AX_MIPI_RX_ATTR_S));
            break;

        case SONY_IMX334:
            memcpy(&tMipiAttr, &gImx334MipiAttr, sizeof(AX_MIPI_RX_ATTR_S));
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tMipiAttr, &gGc4653MipiAttr, sizeof(AX_MIPI_RX_ATTR_S));
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tMipiAttr, &gOs08a20MipiAttr, sizeof(AX_MIPI_RX_ATTR_S));
            break;

        case MIPI_YUV:
            memcpy(&tMipiAttr, &gMIPI_YUVMipiAttr, sizeof(AX_MIPI_RX_ATTR_S));
            break;

        default:
            memcpy(&tMipiAttr, &gOs04a10MipiAttr, sizeof(AX_MIPI_RX_ATTR_S));
            break;
    }

    nRet = AX_MIPI_RX_Reset((AX_MIPI_RX_DEV_E)devId);
    if (0 != nRet) {
        axmpi_error("AX_MIPI_RX_Reset devId:[%d] failed, return:[%d]", devId, nRet);
        return -1;
    }

    if (devId == 0) {
        tMipiAttr.ePhySel = AX_MIPI_RX_PHY0_SEL_LANE_0_1_2_3;
    } else if (devId == 1) {
        tMipiAttr.ePhySel = AX_MIPI_RX_PHY1_SEL_LANE_0_1_2_3;
    } else if (devId == 2) {
        if (bMaster == AX_TRUE) {
            tMipiAttr.ePhySel = AX_MIPI_RX_PHY1_SEL_LANE_0_1_2_3;
        } else {
            tMipiAttr.ePhySel = AX_MIPI_RX_PHY2_SEL_LANE_0_1_2_3;
        }
    } else {
        axmpi_error("devId:[%d] ePhySel is not supported", devId);
        return -2;
    }

    nRet = AX_MIPI_RX_SetAttr((AX_MIPI_RX_DEV_E)devId, &tMipiAttr);
    if (0 != nRet) {
        axmpi_error("AX_MIPI_RX_SetAttr failed, return:[%d]", nRet);
        return -3;
    }

    return 0;
}

int axisp_set_sns_attr(uint8_t pipeId, int eSnsType, int eRawType, int eHdrMode)
{
    int nRet = 0;
    AX_SNS_ATTR_T tSnsAttr;

    memset(&tSnsAttr, 0, sizeof(tSnsAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tSnsAttr, &gOs04a10SnsAttr, sizeof(AX_SNS_ATTR_T));
            break;

        case SONY_IMX334:
            memcpy(&tSnsAttr, &gImx334SnsAttr, sizeof(AX_SNS_ATTR_T));
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tSnsAttr, &gGc4653SnsAttr, sizeof(AX_SNS_ATTR_T));
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tSnsAttr, &gOs08a20SnsAttr, sizeof(AX_SNS_ATTR_T));
            break;

        case SENSOR_DVP:
            memcpy(&tSnsAttr, &gDVPSnsAttr, sizeof(AX_SNS_ATTR_T));
            break;

        default:
            memcpy(&tSnsAttr, &gOs04a10SnsAttr, sizeof(AX_SNS_ATTR_T));
            break;
    }

    tSnsAttr.eRawType = (AX_RAW_TYPE_E)eRawType;
    tSnsAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;

    nRet = AX_VIN_SetSnsAttr(pipeId, &tSnsAttr);
    if (0 != nRet) {
        axmpi_error("pipeId:[%d] AX_VIN_SetSnsAttr pipeId failed, return:[%d]", pipeId, nRet);
        return -1;
    }

    return 0;
}

int axisp_get_sns_config(int eSnsType, axisp_sns_attr_t *ptSnsAttr, axsns_clk_attr_t *ptSnsClkAttr, axisp_dev_attr_t *pDevAttr, axisp_pipe_attr_t *pPipeAttr, axisp_vin_chn_attr_t *pChnAttr)
{
    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(ptSnsAttr, &gOs04a10SnsAttr, sizeof(AX_SNS_ATTR_T));
            memcpy(ptSnsClkAttr, &gOs04a10SnsClkAttr, sizeof(axsns_clk_attr_t));
            memcpy(pDevAttr, &gOs04a10DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gOs04a10PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gOs04a10ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SONY_IMX334:
            memcpy(ptSnsAttr, &gImx334SnsAttr, sizeof(AX_SNS_ATTR_T));
            memcpy(ptSnsClkAttr, &gImx334SnsClkAttr, sizeof(axsns_clk_attr_t));
            memcpy(pDevAttr, &gImx334DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gImx334PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gImx334ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case GALAXYCORE_GC4653:
            memcpy(ptSnsAttr, &gGc4653SnsAttr, sizeof(AX_SNS_ATTR_T));
            memcpy(ptSnsClkAttr, &gGc4653SnsClkAttr, sizeof(axsns_clk_attr_t));
            memcpy(pDevAttr, &gGc4653DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gGc4653PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gGc4653ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case OMNIVISION_OS08A20:
            memcpy(ptSnsAttr, &gOs08a20SnsAttr, sizeof(AX_SNS_ATTR_T));
            memcpy(ptSnsClkAttr, &gOs08a20SnsClkAttr, sizeof(axsns_clk_attr_t));
            memcpy(pDevAttr, &gOs08a20DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gOs08a20PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gOs08a20ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_DVP:
            memcpy(ptSnsAttr, &gDVPSnsAttr, sizeof(AX_SNS_ATTR_T));
            memcpy(ptSnsClkAttr, &gDVPSnsClkAttr, sizeof(axsns_clk_attr_t));
            memcpy(pDevAttr, &gDVPDevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gDVPPipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gDVPChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_BT601:
            memcpy(pDevAttr, &gBT601DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gBT601PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gBT601ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_BT656:
            memcpy(pDevAttr, &gBT656DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gBT656PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gBT656ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_BT1120:
            memcpy(pDevAttr, &gBT1120DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gBT1120PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gBT1120ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case MIPI_YUV:
            memcpy(pDevAttr, &gMIPI_YUVDevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gMIPI_YUVPipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gMIPI_YUVChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        default:
            memcpy(ptSnsAttr, &gOs04a10SnsAttr, sizeof(AX_SNS_ATTR_T));
            memcpy(ptSnsClkAttr, &gOs04a10SnsClkAttr, sizeof(axsns_clk_attr_t));
            memcpy(pDevAttr, &gOs04a10DevAttr, sizeof(AX_DEV_ATTR_T));
            memcpy(pPipeAttr, &gOs04a10PipeAttr, sizeof(AX_PIPE_ATTR_T));
            memcpy(pChnAttr, &gOs04a10ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;
    }

    return 0;
}

int axisp_set_device_attr(uint8_t devId, int eSnsType, int ePixelFmt, int eHdrMode)
{
    int nRet = 0;
    AX_DEV_ATTR_T tDevAttr;

    memset(&tDevAttr, 0, sizeof(tDevAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tDevAttr, &gOs04a10DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case SONY_IMX334:
            memcpy(&tDevAttr, &gImx334DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tDevAttr, &gGc4653DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tDevAttr, &gOs08a20DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case SENSOR_DVP:
            memcpy(&tDevAttr, &gDVPDevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case SENSOR_BT601:
            memcpy(&tDevAttr, &gBT601DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case SENSOR_BT656:
            memcpy(&tDevAttr, &gBT656DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case SENSOR_BT1120:
            memcpy(&tDevAttr, &gBT1120DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case MIPI_YUV:
            memcpy(&tDevAttr, &gMIPI_YUVDevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        default:
            memcpy(&tDevAttr, &gOs04a10DevAttr, sizeof(AX_DEV_ATTR_T));
            break;
    }

    tDevAttr.ePixelFmt = (AX_IMG_FORMAT_E)ePixelFmt;
    tDevAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;

    nRet = AX_VIN_SetDevAttr(devId, &tDevAttr);
    if (0 != nRet) {
        axmpi_error("devId:[%d] AX_VIN_SetDevAttr failed, return:[%d]", devId, nRet);
        return -1;
    }

    return 0;
}

int axisp_set_pipe_attr(uint8_t pipeId, int eSnsType, int ePixelFmt, int eHdrMode)
{
    int nRet = 0;
    AX_PIPE_ATTR_T tPipeAttr;

    memset(&tPipeAttr, 0, sizeof(tPipeAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tPipeAttr, &gOs04a10PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case SONY_IMX334:
            memcpy(&tPipeAttr, &gImx334PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tPipeAttr, &gGc4653PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tPipeAttr, &gOs08a20PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case SENSOR_DVP:
            memcpy(&tPipeAttr, &gDVPPipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case SENSOR_BT601:
            memcpy(&tPipeAttr, &gBT601PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case SENSOR_BT656:
            memcpy(&tPipeAttr, &gBT656PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case SENSOR_BT1120:
            memcpy(&tPipeAttr, &gBT1120PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        case MIPI_YUV:
            memcpy(&tPipeAttr, &gMIPI_YUVPipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;

        default:
            memcpy(&tPipeAttr, &gOs04a10PipeAttr, sizeof(AX_PIPE_ATTR_T));
            break;
    }

    tPipeAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;
    tPipeAttr.ePixelFmt = (AX_IMG_FORMAT_E)ePixelFmt;

    nRet = AX_VIN_SetPipeAttr(pipeId, &tPipeAttr);
    if (0 != nRet) {
        axmpi_error("pipeId:[%d] AX_VI_SetPipeAttr failed, return:[%d]", pipeId, nRet);
        return -1;
    }

    return 0;
}

int axisp_set_channel_attr(uint8_t pipeId, int eSnsType)
{
    int nRet = 0;
    AX_VIN_CHN_ATTR_T tChnAttr;

    memset(&tChnAttr, 0, sizeof(tChnAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tChnAttr, &gOs04a10ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SONY_IMX334:
            memcpy(&tChnAttr, &gImx334ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tChnAttr, &gGc4653ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tChnAttr, &gOs08a20ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_DVP:
            memcpy(&tChnAttr, &gDVPChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_BT601:
            memcpy(&tChnAttr, &gBT601ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_BT656:
            memcpy(&tChnAttr, &gBT656ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case SENSOR_BT1120:
            memcpy(&tChnAttr, &gBT1120ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        case MIPI_YUV:
            memcpy(&tChnAttr, &gMIPI_YUVChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;

        default:
            memcpy(&tChnAttr, &gOs04a10ChnAttr, sizeof(AX_VIN_CHN_ATTR_T));
            break;
    }

    nRet = AX_VIN_SetChnAttr(pipeId, &tChnAttr);
    if (0 != nRet) {
        axmpi_error("pipeId:[%d] AX_VIN_SetChnAttr failed, return:[%d]", pipeId, nRet);
        return -1;
    }

    return 0;
}

int axisp_get_sns_attr(uint8_t pipeId, axisp_sns_attr_t *ptSnsAttr)
{
    int nRet = AX_VIN_GetSnsAttr(pipeId, ptSnsAttr);
    if (0 != nRet) {
        axmpi_error("pipeId:[%d] AX_VIN_GetSnsAttr failed, return:[%d]", pipeId, nRet);
        return -1;
    }

    return 0;
}

int axisp_raw_file_write(char *pfile, char *ptr, int size)
{
    int nRet = 0;
    int w_size = 0;
    FILE *pFile = NULL;

    pFile = fopen(pfile, "wb");
    if (pFile) {
        w_size = fwrite(ptr, 1, size, pFile);
        if (w_size != size) {
            axmpi_error("[%s] wirte raw file failed, w_size:[%d], size:[%d]", pfile, w_size, size);
            nRet = -1;
        }

        fclose(pFile);
    }

    return nRet;
}

static int axisp_get_i2c_device_node(uint8_t devId)
{
    int nBusNum = 0;
    char id[10] = {0};
    FILE *pFile = NULL;
    uint8_t board_id = 0;

    pFile = fopen("/sys/devices/platform/hwinfo/board_id", "r");
    if (pFile) {
        fread(&id[0], 10, 1, pFile);
        fclose(pFile);
    } else {
        axmpi_error("open /sys/devices/platform/hwinfo/board_id failed");
    }

    board_id = atoi(id);
    if (0 == strncmp("F", id, 1)) {
       board_id = 15;
    }

    axmpi_info("get board_id:[%d]", board_id);

    if ((0 == board_id) || (1 == board_id)) {
        if ((0 == devId) || (1 == devId)) {
            nBusNum = 0;
        } else {
            nBusNum = 1;
        }
    } else if ((2 == board_id) || (3 == board_id) || (15 == board_id)) {
        if (0 == devId) {
            nBusNum = 0;
        } else if (1 == devId) {
            nBusNum = 1;
        } else {
            nBusNum = 6;
        }
    } else {
        axmpi_error("get board id failed, board_id:[%d]", board_id);
        return -1;
    }

    return nBusNum;
}

axisp_sensor_regfunc_t *axisp_get_snsobj(int eSnsType)
{
    axisp_sensor_regfunc_t *ptSnsHdl = NULL;

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
            // ptSnsHdl = &gSnsos04a10Obj;
            break;

        case OMNIVISION_OS04A10_MASTER:
            // ptSnsHdl = &gSnsos04a10MasterObj;
            break;

        case OMNIVISION_OS04A10_SLAVE:
            // ptSnsHdl = &gSnsos04a10SlaveObj;
            break;

        case GALAXYCORE_GC4653:
            // ptSnsHdl = &gSnsgc4653Obj;
            break;

        default:
            // ptSnsHdl = &gSnsos04a10Obj;
            break;
    }

    return ptSnsHdl;
}

static int axisp_get_sns_bus_type(int eSnsType)
{
    int enBusType;

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
        case SONY_IMX334:
        case GALAXYCORE_GC4653:
        case OMNIVISION_OS08A20:
        case SENSOR_DVP:
        case SENSOR_BT601:
        case SENSOR_BT656:
        case SENSOR_BT1120:
        case MIPI_YUV:
        case SAMPLE_SNS_TYPE_BUTT:
            enBusType = ISP_SNS_CONNECT_I2C_TYPE;
            break;

        default:
            enBusType = ISP_SNS_CONNECT_I2C_TYPE;
            break;
    }

    return enBusType;
}

static int regsiterSns(uint8_t pipeId, uint8_t devId, int eBusType, axisp_sensor_regfunc_t *ptSnsHdl)
{
    int axRet = 0;
    AX_SNS_COMMBUS_T tSnsBusInfo = {0};

    axRet = AX_VIN_RegisterSensor(pipeId, ptSnsHdl);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Register Sensor failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    if (ISP_SNS_CONNECT_I2C_TYPE == eBusType) {
        tSnsBusInfo.I2cDev = axisp_get_i2c_device_node(devId);
    } else {
        tSnsBusInfo.SpiDev.bit4SpiDev = axisp_get_i2c_device_node(devId);
        tSnsBusInfo.SpiDev.bit4SpiCs = 0;
    }

    if (NULL != ptSnsHdl->pfn_sensor_set_bus_info) {
        axRet = ptSnsHdl->pfn_sensor_set_bus_info(pipeId, tSnsBusInfo);
        if (0 != axRet) {
            axmpi_error("set sensor bus info failed, return:[%d", axRet);
            return -2;
        }
    } else {
        axmpi_error("not support set sensor bus info");
        return -3;
    }

    return 0;
}

int axisp_regsiter_sns(uint8_t pipeId, uint8_t devId, int eSnsType)
{
    int eBusType;
    axisp_sensor_regfunc_t *ptSnsHdl = NULL;

    ptSnsHdl = axisp_get_snsobj(eSnsType);
    if (NULL == ptSnsHdl) {
        axmpi_error("AX_ISP Get Sensor Object failed");
        return -1;
    }

    eBusType = axisp_get_sns_bus_type(eSnsType);
    return regsiterSns(pipeId, devId, eBusType, ptSnsHdl);
}

int axisp_unregsiter_sns(uint8_t pipeId)
{
    int axRet = AX_VIN_UnRegisterSensor(pipeId);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Unregister Sensor failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    return 0;
}

static int regsiterAeAlgLib(uint8_t pipeId, axisp_sensor_regfunc_t *ptSnsHdl, bool bUser3A, axisp_ae_regfunc_t *pAeFuncs)
{
    int axRet = 0;
    axisp_ae_regfunc_t tAeFuncs = {0};

    if (!bUser3A) {
        tAeFuncs.pfnAe_Init = AX_ISP_ALG_AeInit;
        tAeFuncs.pfnAe_Exit = AX_ISP_ALG_AeDeInit;
        tAeFuncs.pfnAe_Run = AX_ISP_ALG_AeRun;

        axRet = AX_ISP_ALG_AeRegisterSensor(pipeId, ptSnsHdl);
        if (axRet) {
            axmpi_error("pipeId:[%d] AX_ISP Register Sensor failed, return:[%d]", pipeId, axRet);
            return -1;
        }
    } else {
        tAeFuncs.pfnAe_Init = pAeFuncs->pfnAe_Init;
        tAeFuncs.pfnAe_Exit = pAeFuncs->pfnAe_Exit;
        tAeFuncs.pfnAe_Run  = pAeFuncs->pfnAe_Run;
    }

    axRet = AX_ISP_RegisterAeLibCallback(pipeId, &tAeFuncs);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Register ae callback failed, return:[%d]", pipeId, axRet);
        return -2;
    }

    return 0;
}

int axisp_register_ae_alglib(uint8_t pipeId, int eSnsType, bool bUser3A, axisp_ae_regfunc_t *pAeFunc)
{
    axisp_sensor_regfunc_t *ptSnsHdl = NULL;

    ptSnsHdl = axisp_get_snsobj(eSnsType);
    if (NULL == ptSnsHdl) {
        axmpi_error("AX_ISP Get Sensor Object failed");
        return -1;
    }

    return regsiterAeAlgLib(pipeId, ptSnsHdl, bUser3A, pAeFunc);
}

int axisp_unregister_ae_alglib(uint8_t pipeId)
{
    int axRet = AX_ISP_ALG_AeUnRegisterSensor(pipeId);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP ae un register sensor failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    axRet = AX_ISP_UnRegisterAeLibCallback(pipeId);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Unregister Sensor failed, return:[%d]", pipeId, axRet);
        return -2;
    }

    return 0;
}

static int registerAwbAlgLib(uint8_t pipeId, axisp_sensor_regfunc_t *ptSnsHdl, bool bUser3A, axisp_awb_refunc_t *pAwbFuncs)
{
    int axRet = 0;
    axisp_awb_refunc_t tAwbFuncs = {0};

    if (!bUser3A) {
        tAwbFuncs.pfnAwb_Init = AX_ISP_ALG_AwbInit;
        tAwbFuncs.pfnAwb_Exit = AX_ISP_ALG_AwbDeInit;
        tAwbFuncs.pfnAwb_Run = AX_ISP_ALG_AwbRun;
    } else {
        tAwbFuncs.pfnAwb_Init = pAwbFuncs->pfnAwb_Init;
        tAwbFuncs.pfnAwb_Exit = pAwbFuncs->pfnAwb_Exit;
        tAwbFuncs.pfnAwb_Run = pAwbFuncs->pfnAwb_Run;
    }

    axRet = AX_ISP_RegisterAwbLibCallback(pipeId, &tAwbFuncs);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Register awb callback failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    return 0;
}

int axisp_register_awb_alglib(uint8_t pipeId, int eSnsType, bool bUser3A, axisp_awb_refunc_t *pAwbFunc)
{
    axisp_sensor_regfunc_t *ptSnsHdl = NULL;

    ptSnsHdl = axisp_get_snsobj(eSnsType);
    if (NULL == ptSnsHdl) {
        axmpi_error("AX_ISP Get Sensor Object failed");
        return -1;
    }

    return registerAwbAlgLib(pipeId, ptSnsHdl, bUser3A, pAwbFunc);
}

int axisp_unregister_awb_alglib(uint8_t pipeId)
{
    int axRet = AX_ISP_UnRegisterAwbLibCallback(pipeId);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Unregister Sensor failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    return 0;
}

static int registerLscAlgLib(uint8_t pipeId, axisp_sensor_regfunc_t *ptSnsHdl, bool bUser3A, axisp_lsc_refunc_t *pLscFunc)
{
    if (!bUser3A) {
        return 0;
    }

    int axRet = AX_ISP_RegisterLscLibCallback(pipeId, pLscFunc);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Register Lsc callback failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    return 0;
}

int axisp_register_lsc_alglib(uint8_t pipeId, int eSnsType, bool bUser3A, axisp_lsc_refunc_t *pLscFunc)
{
    axisp_sensor_regfunc_t *ptSnsHdl = NULL;

    ptSnsHdl = axisp_get_snsobj(eSnsType);
    if (NULL == ptSnsHdl) {
        axmpi_error("AX_ISP Get Sensor Object failed");
        return -1;
    }

    return registerLscAlgLib(pipeId, ptSnsHdl, bUser3A, pLscFunc);
}

int axisp_unregister_lsc_alglib(uint8_t pipeId)
{
    int axRet = AX_ISP_UnRegisterLscLibCallback(pipeId);
    if (axRet) {
        axmpi_error("pipeId:[%d] AX_ISP Unregister Sensor failed, return:[%d]", pipeId, axRet);
        return -1;
    }

    return 0;
}

int axisp_pool_init(int eSnsMode, int eSnsType, int raw_type, axisp_pool_floorplant_t *pPoolFloorPlan, uint32_t gDumpFrame, uint8_t eRunMode)
{
    int sRet = 0;
    AX_DEV_ATTR_T *pDevAttr = NULL;
    AX_PIPE_ATTR_T *pPipeAttr = NULL;
    AX_VIN_CHN_ATTR_T *pChnAttr = NULL;

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            pPipeAttr = &gOs04a10PipeAttr;
            pChnAttr = &gOs04a10ChnAttr;
            pDevAttr = &gOs04a10DevAttr;
            break;

        case SONY_IMX334:
            pPipeAttr = &gImx334PipeAttr;
            pChnAttr = &gImx334ChnAttr;
            pDevAttr = &gImx334DevAttr;
            break;

        case GALAXYCORE_GC4653:
            pPipeAttr = &gGc4653PipeAttr;
            pChnAttr = &gGc4653ChnAttr;
            pDevAttr = &gGc4653DevAttr;
            break;

        case OMNIVISION_OS08A20:
            pPipeAttr = &gOs08a20PipeAttr;
            pChnAttr = &gOs08a20ChnAttr;
            pDevAttr = &gOs08a20DevAttr;
            break;

        case SENSOR_DVP:
            pPipeAttr = &gDVPPipeAttr;
            pChnAttr = &gDVPChnAttr;
            pDevAttr = &gDVPDevAttr;
            break;

        case SENSOR_BT601:
            pPipeAttr = &gBT601PipeAttr;
            pChnAttr = &gBT601ChnAttr;
            pDevAttr = &gBT601DevAttr;
            break;

        case SENSOR_BT656:
            pPipeAttr = &gBT656PipeAttr;
            pChnAttr = &gBT656ChnAttr;
            pDevAttr = &gBT656DevAttr;
            break;

        case SENSOR_BT1120:
            pPipeAttr = &gBT1120PipeAttr;
            pChnAttr = &gBT1120ChnAttr;
            pDevAttr = &gBT1120DevAttr;
            break;

        case MIPI_YUV:
            pPipeAttr = &gMIPI_YUVPipeAttr;
            pChnAttr = &gMIPI_YUVChnAttr;
            pDevAttr = &gMIPI_YUVDevAttr;
            break;

        default:
            pPipeAttr = &gOs04a10PipeAttr;
            pChnAttr = &gOs04a10ChnAttr;
            pDevAttr = &gOs04a10DevAttr;
            break;
    }

    sRet = AX_POOL_Exit();
    if (sRet) {
        axmpi_error("AX_POOL_Exit failed, return:[%d]", sRet);
    }

    memset(pPoolFloorPlan, 0, sizeof(AX_POOL_FLOORPLAN_T));
    pPoolFloorPlan->CommPool[COMM_ISP_RAW0].MetaSize   = 10 * 1024;
    if (eRunMode == 2) {
        pPoolFloorPlan->CommPool[COMM_ISP_RAW0].BlkSize = AX_VIN_GetImgBufferSize(pDevAttr->tDevImgRgn.nHeight, pDevAttr->tDevImgRgn.nWidth, AX_FORMAT_BAYER_RAW_14BPP, AX_TRUE);
    } else {
        pPoolFloorPlan->CommPool[COMM_ISP_RAW0].BlkSize = AX_VIN_GetImgBufferSize(pDevAttr->tDevImgRgn.nHeight, pDevAttr->tDevImgRgn.nWidth, (AX_IMG_FORMAT_E)rawFmtToCommFmt(raw_type), AX_TRUE);
    }

    if (gDumpFrame) {
        pPoolFloorPlan->CommPool[COMM_ISP_RAW0].BlkCnt = 40 + gDumpFrame;
    } else {
        pPoolFloorPlan->CommPool[COMM_ISP_RAW0].BlkCnt = 40;
    }
    pPoolFloorPlan->CommPool[COMM_ISP_RAW0].CacheMode = POOL_CACHE_MODE_NONCACHE;

    memset(pPoolFloorPlan->CommPool[COMM_ISP_RAW0].PartitionName, 0, sizeof(pPoolFloorPlan->CommPool[COMM_ISP_RAW0].PartitionName));
    strcpy((char *)pPoolFloorPlan->CommPool[COMM_ISP_RAW0].PartitionName, "anonymous");

    pPoolFloorPlan->CommPool[COMM_ISP_RAW1].MetaSize = 10 * 1024;
    pPoolFloorPlan->CommPool[COMM_ISP_RAW1].BlkSize = AX_VIN_GetImgBufferSize(pPipeAttr->nHeight, pPipeAttr->nWidth, AX_FORMAT_BAYER_RAW_16BPP, AX_TRUE);
    pPoolFloorPlan->CommPool[COMM_ISP_RAW1].BlkCnt = 5;
    pPoolFloorPlan->CommPool[COMM_ISP_RAW1].CacheMode = POOL_CACHE_MODE_NONCACHE;

    memset(pPoolFloorPlan->CommPool[COMM_ISP_RAW1].PartitionName, 0, sizeof(pPoolFloorPlan->CommPool[COMM_ISP_RAW1].PartitionName));
    strcpy((char *)pPoolFloorPlan->CommPool[COMM_ISP_RAW1].PartitionName, "anonymous");

    for (int i = COMM_ISP_YUV_MAIN; i <= COMM_ISP_YUV_SUB2; i++) {
        pPoolFloorPlan->CommPool[i].MetaSize = 10 * 1024;
        pPoolFloorPlan->CommPool[i].BlkSize = AX_VIN_GetImgBufferSize(pChnAttr->tChnAttr[i - COMM_ISP_YUV_MAIN].nHeight, pChnAttr->tChnAttr[i - COMM_ISP_YUV_MAIN].nWidthStride, AX_YUV420_SEMIPLANAR, AX_TRUE);

        pPoolFloorPlan->CommPool[i].BlkCnt = pChnAttr->tChnAttr[i - COMM_ISP_YUV_MAIN].nDepth;
        pPoolFloorPlan->CommPool[i].CacheMode = POOL_CACHE_MODE_NONCACHE;

        memset(pPoolFloorPlan->CommPool[i].PartitionName, 0, sizeof(pPoolFloorPlan->CommPool[i].PartitionName));
        strcpy((char *)pPoolFloorPlan->CommPool[i].PartitionName, "anonymous");
    }

    sRet = AX_POOL_SetConfig(pPoolFloorPlan);
    if (sRet) {
        axmpi_error("AX_POOL_SetConfig failed, return:[%d]", sRet);
        return -1;
    }

    sRet = AX_POOL_Init();
    if (sRet) {
        axmpi_error("AX_POOL_Init failed, return:[%d]", sRet);
        return -2;
    }

    return 0;
}

int axisp_initTx()
{
    int axRet = AX_MIPI_TX_Init();
    if (0 != axRet) {
        axmpi_error("AX_MIPI_TX_Init failed, return:[%d]", axRet);
        return -1;
    }

    return 0;
}

int axisp_deinitTx()
{
    int axRet = AX_MIPI_TX_DeInit();
    if (axRet != 0) {
        axmpi_error("AX_MIPI_TX_DeInit failed, return:[%d]", axRet);
        return -1;
    }

    return 0;
}

int axisp_set_mipiTx_attr(uint8_t mipiDevId, int eSnsType, int eHdrMode, bool bIspBypass)
{
    int nRet = 0;
    AX_MIPI_TX_ATTR_S tMipiTxAttr;

    memset(&tMipiTxAttr, 0, sizeof(tMipiTxAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            if (bIspBypass) {
                memcpy(&tMipiTxAttr, &gOs04a10MipiTxIspBypassAttr, sizeof(AX_MIPI_TX_ATTR_S));
            } else {
                memcpy(&tMipiTxAttr, &gOs04a10MipiTxAttr, sizeof(AX_MIPI_TX_ATTR_S));
            }
            break;

        case SONY_IMX334:
            if (bIspBypass) {
                memcpy(&tMipiTxAttr, &gImx334MipiTxIspBypassAttr, sizeof(AX_MIPI_TX_ATTR_S));
            } else {
                memcpy(&tMipiTxAttr, &gImx334MipiTxAttr, sizeof(AX_MIPI_TX_ATTR_S));
            }
            break;

        case GALAXYCORE_GC4653:
            if (bIspBypass) {
                memcpy(&tMipiTxAttr, &gGc4653MipiTxIspBypassAttr, sizeof(AX_MIPI_TX_ATTR_S));
            } else {
                memcpy(&tMipiTxAttr, &gGc4653MipiTxAttr, sizeof(AX_MIPI_TX_ATTR_S));
            }
            break;

        case OMNIVISION_OS08A20:
            if (bIspBypass) {
                memcpy(&tMipiTxAttr, &gOs08a20MipiTxIspBypassAttr, sizeof(AX_MIPI_TX_ATTR_S));
                if (eHdrMode == AX_SNS_HDR_2X_MODE) {
                    tMipiTxAttr.eImgDataType = AX_MIPI_DT_RAW10;
                    tMipiTxAttr.eDataRate = AX_MIPI_DATA_RATE_1500M;
                }
            } else {
                memcpy(&tMipiTxAttr, &gOs08a20MipiTxAttr, sizeof(AX_MIPI_TX_ATTR_S));
            }
            break;

        default:
            if (bIspBypass) {
                memcpy(&tMipiTxAttr, &gOs04a10MipiTxIspBypassAttr, sizeof(AX_MIPI_TX_ATTR_S));
            } else {
                memcpy(&tMipiTxAttr, &gOs04a10MipiTxAttr, sizeof(AX_MIPI_TX_ATTR_S));
            }
            break;
    }

    if ((bIspBypass) && (eHdrMode == AX_SNS_HDR_2X_MODE)) {
        tMipiTxAttr.eDolSplitNum = (AX_MIPI_DOL_NUM_E)2;
    }

    nRet = AX_MIPI_TX_Reset((AX_MIPI_TX_DEV_E)mipiDevId);
    if (0 != nRet) {
        axmpi_error("mipiDevId:[%d] AX_MIPI_TX_Reset failed, return:[%d]", mipiDevId, nRet);
        return -1;
    }

    nRet = AX_MIPI_TX_SetAttr((AX_MIPI_TX_DEV_E)mipiDevId, &tMipiTxAttr);
    if (0 != nRet) {
        axmpi_error("mipiDevId:[%d] AX_MIPI_TX_SetAttr failed, return:[%d]", mipiDevId, nRet);
        return -2;
    }

    return 0;
}

int axisp_openTx(uint8_t devId, int eSnsType, bool bIspBypass)
{
    int nRet = 0;
    AX_TX_IMG_INFO_T tTxImgInfo;

    memset(&tTxImgInfo, 0, sizeof(tTxImgInfo));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tTxImgInfo, &gOs04a10TxImgInfo, sizeof(AX_TX_IMG_INFO_T));
            if (bIspBypass == AX_TRUE) {
                tTxImgInfo.bIspBypass = AX_TRUE;
                tTxImgInfo.eImgFormat = AX_FORMAT_BAYER_RAW_10BPP;
            }
            break;

        case SONY_IMX334:
            memcpy(&tTxImgInfo, &gImx334TxImgInfo, sizeof(AX_TX_IMG_INFO_T));
            if (bIspBypass == AX_TRUE) {
                tTxImgInfo.bIspBypass = AX_TRUE;
                tTxImgInfo.eImgFormat = AX_FORMAT_BAYER_RAW_10BPP;
            }
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tTxImgInfo, &gGc4653TxImgInfo, sizeof(AX_TX_IMG_INFO_T));
            if (bIspBypass == AX_TRUE) {
                tTxImgInfo.bIspBypass = AX_TRUE;
                tTxImgInfo.eImgFormat = AX_FORMAT_BAYER_RAW_10BPP;
            }
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tTxImgInfo, &gOs08a20TxImgInfo, sizeof(AX_TX_IMG_INFO_T));
            if (bIspBypass == AX_TRUE) {
                tTxImgInfo.bIspBypass = AX_TRUE;
                tTxImgInfo.eImgFormat = AX_FORMAT_BAYER_RAW_12BPP;
            }
            break;

        default:
            memcpy(&tTxImgInfo, &gOs04a10TxImgInfo, sizeof(AX_TX_IMG_INFO_T));
            if (bIspBypass == AX_TRUE) {
                tTxImgInfo.bIspBypass = AX_TRUE;
                tTxImgInfo.eImgFormat = AX_FORMAT_BAYER_RAW_10BPP;
            }
            break;
    }

    nRet = AX_MIPI_TX_Start((AX_MIPI_TX_DEV_E)devId);
    if (0 != nRet) {
        axmpi_error("devId:[%d] AX_MIPI_TX_Start failed, return:[%d]", devId, nRet);
        return -1;
    }

    nRet = AX_VIN_TxOpen((AX_MIPI_TX_DEV_E)devId, &tTxImgInfo);
    if (0 != nRet) {
        axmpi_error("devId:[%d] AX_VIN_TxOpen failed, return:[%d]", devId, nRet);
        return -2;
    }

    return 0;
}

int axisp_closeTx(uint8_t devId)
{
    int nRet = AX_VIN_TxClose(devId);
    if (0 != nRet) {
        axmpi_error("devId:[%d] AX_VIN_TxClose failed, return:[%d]", devId, nRet);
        return -1;
    }

    nRet = AX_MIPI_TX_Stop((AX_MIPI_TX_DEV_E)devId);
    if (0 != nRet) {
        axmpi_error("devId:[%d] AX_MIPI_TX_Stop failed, return:[%d]", devId, nRet);
        return -2;
    }

    return 0;
}

int axisp_set_device_attrEx(uint8_t devId, int eSnsType, int ePixelFmt, int eHdrMode, int eWorkMode, bool bImgEnable, bool bNonImgEnable, bool bIspBypass)
{
    int nRet = 0;
    AX_DEV_ATTR_T tDevAttr;

    memset(&tDevAttr, 0, sizeof(tDevAttr));

    switch (eSnsType) {
        case OMNIVISION_OS04A10:
        case OMNIVISION_OS04A10_MASTER:
        case OMNIVISION_OS04A10_SLAVE:
            memcpy(&tDevAttr, &gOs04a10DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case SONY_IMX334:
            memcpy(&tDevAttr, &gImx334DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case GALAXYCORE_GC4653:
            memcpy(&tDevAttr, &gGc4653DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        case OMNIVISION_OS08A20:
            memcpy(&tDevAttr, &gOs08a20DevAttr, sizeof(AX_DEV_ATTR_T));
            break;

        default:
            memcpy(&tDevAttr, &gOs04a10DevAttr, sizeof(AX_DEV_ATTR_T));
            break;
    }

    tDevAttr.ePixelFmt = (AX_IMG_FORMAT_E)ePixelFmt;
    tDevAttr.eNonPixelFmt = (AX_IMG_FORMAT_E)ePixelFmt;
    tDevAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;
    tDevAttr.eDevWorkMode = (AX_ISP_DEV_WORK_MODE_E)eWorkMode;
    tDevAttr.bImgDataEnable = bImgEnable ? AX_TRUE : AX_FALSE;
    tDevAttr.bNonImgEnable = bNonImgEnable ? AX_TRUE : AX_FALSE;
    tDevAttr.bIspBypass = bIspBypass ? AX_TRUE : AX_FALSE;

    nRet = AX_VIN_SetDevAttr(devId, &tDevAttr);
    if (0 != nRet) {
        axmpi_error("devId:[%d AX_VIN_SetDevAttr failed, return:[%d]", devId, nRet);
        return -1;
    }

    return 0;
}

API_END_NAMESPACE(media)
