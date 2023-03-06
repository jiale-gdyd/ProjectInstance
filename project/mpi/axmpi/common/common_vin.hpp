#ifndef MPI_AXMPI_COMMON_VIN_HPP
#define MPI_AXMPI_COMMON_VIN_HPP

#include <stdio.h>
#include <stdarg.h>
#include <utils/export.h>

#include <ax_vin_api.h>
#include <ax_sys_api.h>
#include <ax_mipi_api.h>
#include <ax_base_type.h>
#include <ax_isp_3a_api.h>
#include <ax_isp_common.h>
#include <ax_interpreter_external_api.h>

#include "common_config.h"

API_BEGIN_NAMESPACE(media)

#define MAX_SNS_NUM                         2
#define DEF_ISP_BUF_BLOCK_NUM               (10)

#define AX_ALIGN_UP_SAMPLE(x, align)        (((x) + ((align) - 1)) & ~((align)-1))
#define AX_ALIGN_DOWN_SAMPLE(x, align)      ((x) & ~((align)-1))

enum {
    SAMPLE_SNS_TYPE_NONE      = -1,

    OMNIVISION_OS04A10        = 0,
    OMNIVISION_OS04A10_MASTER = 1,
    OMNIVISION_OS04A10_SLAVE  = 2,
    OMNIVISION_OS08A20        = 3,

    SONY_IMX334               = 20,

    GALAXYCORE_GC4653         = 30,

    SENSOR_DVP                = 40,

    SENSOR_BT601              = 50,
    SENSOR_BT656              = 51,
    SENSOR_BT1120             = 52,

    MIPI_YUV                  = 60,

    SAMPLE_SNS_TYPE_BUTT,
};

enum {
    COMM_ISP_SUCCESS         = 0x0,
    COMM_ISP_ERR_CODE_FAILED = 0x1,
    COMM_ISP_ERR_CODE_PTR_NULL,
    COMM_ISP_ERR_CODE_INVALID_ADDRESS,
    COMM_ISP_ERR_CODE_ILLEGAL_PARAMS,
};

enum {
    COMM_ISP_RAW0 = 0,
    COMM_ISP_RAW1,
    COMM_ISP_YUV_MAIN,
    COMM_ISP_YUV_SUB1,
    COMM_ISP_YUV_SUB2,
    COMM_ISP_BUF_MAX
};

enum {
    SNS_MODE_NONE   = 0,
    SNS_LINEAR_MODE = 1,
    SNS_HDR_2X_MODE = 2,
    SNS_HDR_3X_MODE = 3,
    SNS_HDR_4X_MODE = 4,
    SNS_HDR_MODE_BUTT
};

typedef AX_DEV_ATTR_T axisp_dev_attr_t;
typedef AX_SNS_ATTR_T axisp_sns_attr_t;
typedef AX_PIPE_ATTR_T axisp_pipe_attr_t;
typedef AX_VIN_CHN_ATTR_T axisp_vin_chn_attr_t;
typedef AX_ISP_AE_REGFUNCS_T axisp_ae_regfunc_t;
typedef AX_ISP_AWB_REGFUNCS_T axisp_awb_refunc_t;
typedef AX_ISP_LSC_REGFUNCS_T axisp_lsc_refunc_t;
typedef AX_POOL_FLOORPLAN_T axisp_pool_floorplant_t;
typedef AX_SENSOR_REGISTER_FUNC_T axisp_sensor_regfunc_t;

int axisp_initTx();
int axisp_deinitTx();

int axisp_regsiter_sns(uint8_t pipeId, uint8_t devId, int eSnsType);
int axisp_unregsiter_sns(uint8_t pipeId);

int axisp_register_ae_alglib(uint8_t pipeId, int eSnsType, bool bUser3A, axisp_ae_regfunc_t *pAeFunc);
int axisp_unregister_ae_alglib(uint8_t pipeId);

int axisp_register_awb_alglib(uint8_t pipeId, int eSnsType, bool bUser3A, axisp_awb_refunc_t *pAwbFunc);
int axisp_unregister_awb_alglib(uint8_t pipeId);

int axisp_register_lsc_alglib(uint8_t pipeId, int eSnsType, bool bUser3A, axisp_lsc_refunc_t *pLscFunc);
int axisp_unregister_lsc_alglib(uint8_t pipeId);

axisp_sensor_regfunc_t *axisp_get_snsobj(int eSnsType);

int axisp_set_mipi_attr(uint8_t devId, int eSnsType, bool bMaster);
int axisp_set_sns_attr(uint8_t pipeId, int eSnsType, int eRawType, int eHdrMode);

int axisp_set_pipe_attr(uint8_t pipeId, int eSnsType, int ePixelFmt, int eHdrMode);
int axisp_set_device_attr(uint8_t devId, int eSnsType, int ePixelFmt, int eHdrMode);

int axisp_set_channel_attr(uint8_t pipeId, int eSnsType);
int axisp_get_sns_attr(uint8_t pipeId, axisp_sns_attr_t *ptSnsAttr);

int axisp_set_mipiTx_attr(uint8_t mipiDevId, int eSnsType, int eHdrMode, bool bIspBypass);

int axisp_openTx(uint8_t devId, int eSnsType, bool bIspBypass);
int axisp_closeTx(uint8_t devId);

int axisp_set_device_attrEx(uint8_t devId, int eSnsType, int ePixelFmt, int eHdrMode, int eWorkMode, bool bImgEnable, bool bNonImgEnable, bool bIspBypass);

int axisp_raw_file_write(char *pfile, char *ptr, int size);

int axisp_pool_init(int eSnsMode, int eSnsType, int raw_type, axisp_pool_floorplant_t *pPoolFloorPlan, uint32_t gDumpFrame, uint8_t eRunMode);
int axisp_get_sns_config(int eSnsType, axisp_sns_attr_t *ptSnsAttr, axsns_clk_attr_t *ptSnsClkAttr, axisp_dev_attr_t *pDevAttr, axisp_pipe_attr_t *pPipeAttr, axisp_vin_chn_attr_t *pChnAttr);

API_END_NAMESPACE(media)

#endif
