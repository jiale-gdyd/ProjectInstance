#ifndef MPI_AXMPI_COMMON_CONFIG_H
#define MPI_AXMPI_COMMON_CONFIG_H

#include <stdint.h>
#include <ax_vin_api.h>
#include <ax_mipi_api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t nSnsClkIdx;
    int     eSnsClkRate;
} axsns_clk_attr_t;

extern AX_MIPI_TX_ATTR_S gOs04a10MipiTxIspBypassAttr;
extern AX_MIPI_TX_ATTR_S gOs04a10MipiTxAttr;
extern AX_TX_IMG_INFO_T gOs04a10TxImgInfo;
extern AX_MIPI_RX_ATTR_S gOs04a10MipiAttr;
extern AX_SNS_ATTR_T gOs04a10SnsAttr;
extern axsns_clk_attr_t gOs04a10SnsClkAttr;
extern AX_DEV_ATTR_T gOs04a10DevAttr;
extern AX_PIPE_ATTR_T gOs04a10PipeAttr;
extern AX_VIN_CHN_ATTR_T gOs04a10ChnAttr;

extern AX_MIPI_TX_ATTR_S gImx334MipiTxIspBypassAttr;
extern AX_MIPI_TX_ATTR_S gImx334MipiTxAttr;
extern AX_TX_IMG_INFO_T gImx334TxImgInfo;
extern AX_MIPI_RX_ATTR_S gImx334MipiAttr;
extern AX_SNS_ATTR_T gImx334SnsAttr;
extern axsns_clk_attr_t gImx334SnsClkAttr;
extern AX_DEV_ATTR_T gImx334DevAttr;
extern AX_PIPE_ATTR_T gImx334PipeAttr;
extern AX_VIN_CHN_ATTR_T gImx334ChnAttr;

extern AX_MIPI_TX_ATTR_S gGc4653MipiTxIspBypassAttr;
extern AX_MIPI_TX_ATTR_S gGc4653MipiTxAttr;
extern AX_TX_IMG_INFO_T gGc4653TxImgInfo;
extern AX_MIPI_RX_ATTR_S gGc4653MipiAttr;
extern AX_SNS_ATTR_T gGc4653SnsAttr;
extern axsns_clk_attr_t gGc4653SnsClkAttr;
extern AX_DEV_ATTR_T gGc4653DevAttr;
extern AX_PIPE_ATTR_T gGc4653PipeAttr;
extern AX_VIN_CHN_ATTR_T gGc4653ChnAttr;

extern AX_MIPI_TX_ATTR_S gOs08a20MipiTxIspBypassAttr;
extern AX_MIPI_TX_ATTR_S gOs08a20MipiTxAttr;
extern AX_TX_IMG_INFO_T gOs08a20TxImgInfo;
extern AX_MIPI_RX_ATTR_S gOs08a20MipiAttr;
extern AX_SNS_ATTR_T gOs08a20SnsAttr;
extern axsns_clk_attr_t gOs08a20SnsClkAttr;
extern AX_DEV_ATTR_T gOs08a20DevAttr;
extern AX_PIPE_ATTR_T gOs08a20PipeAttr;
extern AX_VIN_CHN_ATTR_T gOs08a20ChnAttr;

extern AX_SNS_ATTR_T gDVPSnsAttr;
extern axsns_clk_attr_t gDVPSnsClkAttr;
extern AX_DEV_ATTR_T gDVPDevAttr;
extern AX_PIPE_ATTR_T gDVPPipeAttr;
extern AX_VIN_CHN_ATTR_T gDVPChnAttr;

extern AX_DEV_ATTR_T gBT601DevAttr;
extern AX_PIPE_ATTR_T gBT601PipeAttr;
extern AX_VIN_CHN_ATTR_T gBT601ChnAttr;

extern AX_DEV_ATTR_T gBT656DevAttr;
extern AX_PIPE_ATTR_T gBT656PipeAttr;
extern AX_VIN_CHN_ATTR_T gBT656ChnAttr;
extern AX_DEV_ATTR_T gBT1120DevAttr;

extern AX_PIPE_ATTR_T gBT1120PipeAttr;
extern AX_VIN_CHN_ATTR_T gBT1120ChnAttr;

extern AX_MIPI_RX_ATTR_S gMIPI_YUVMipiAttr;
extern AX_DEV_ATTR_T gMIPI_YUVDevAttr;
extern AX_PIPE_ATTR_T gMIPI_YUVPipeAttr;
extern AX_VIN_CHN_ATTR_T gMIPI_YUVChnAttr;

#ifdef __cplusplus
}
#endif

#endif
