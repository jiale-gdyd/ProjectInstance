#ifndef MPI_AXMPI_COMMON_CAMERA_HPP
#define MPI_AXMPI_COMMON_CAMERA_HPP

#include <pthread.h>
#include <utils/export.h>
#include <ax_base_type.h>
#include <ax_sensor_struct.h>

#include "common_vin.hpp"

API_BEGIN_NAMESPACE(media)

#define MAX_CAMERAS                     (4)
#define MAX_FILE_NAME_CHAR_SIZE         (128)

typedef struct {
    bool                  bOpen;
    bool                  bUser3a;
    int                   eHdrMode;
    int                   eSnsType;
    int                   eSrcId;
    int                   eSrcType;
    int                   eRawType;
    int                   ePixelFmt;
    int                   nRxDev;
    int                   nTxDev;
    uint8_t               nDevId;
    uint8_t               nPipeId;
    pthread_t             tIspProcThread;
    pthread_t             tTxTransferThread;
    pthread_t             tIspAFProcThread;
    char                  szTuningFileName[MAX_FILE_NAME_CHAR_SIZE];
    AX_SNS_ATTR_T         stSnsAttr;
    axsns_clk_attr_t      stSnsClkAttr;
    AX_DEV_ATTR_T         stDevAttr;
    AX_PIPE_ATTR_T        stPipeAttr;
    AX_VIN_CHN_ATTR_T     stChnAttr;
    AX_ISP_AE_REGFUNCS_T  tAeFuncs;
    AX_ISP_AWB_REGFUNCS_T tAwbFuncs;
    AX_ISP_LSC_REGFUNCS_T tLscFuncs;
} axcam_t;

int axcam_init();
int axcam_deinit();

int axcam_open(axcam_t *cam);
int axcam_close(axcam_t *cam);

int axcam_dvp_open(axcam_t *cam);
int axcam_dvp_close(axcam_t *cam);

API_END_NAMESPACE(media)

#endif
