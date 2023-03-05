#ifndef AXERA_MPI_AXMPI_AXAPI_H
#define AXERA_MPI_AXMPI_AXAPI_H

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "axmpi.h"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

#define AXMPI_UPALIGNTO(value, align)           ((value + align - 1) & (~(align - 1)))

#define AXMPI_UPALIGNTO2(value)                 AXMPI_UPALIGNTO(value, 2)
#define AXMPI_DOWNALIGNTO2(value)               (AXMPI_UPALIGNTO(value, 2) - 2)

#define AXMPI_UPALIGNTO4(value)                 AXMPI_UPALIGNTO(value, 4)
#define AXMPI_DOWNALIGNTO4(value)               (AXMPI_UPALIGNTO(value, 4) - 4)

#define AXMPI_UPALIGNTO8(value)                 AXMPI_UPALIGNTO(value, 8)
#define AXMPI_DOWNALIGNTO8(value)               (AXMPI_UPALIGNTO(value, 8) - 8)

#define AXMPI_UPALIGNTO16(value)                AXMPI_UPALIGNTO(value, 16)
#define AXMPI_DOWNALIGNTO16(value)              (AXMPI_UPALIGNTO(value, 16) - 16)

#define AXMPI_UPALIGNTO32(value)                AXMPI_UPALIGNTO(value, 32)
#define AXMPI_DOWNALIGNTO32(value)              (AXMPI_UPALIGNTO(value, 32) - 32)

/* VIN设备号 */
enum {
    VIN_DEV_00 = 0,
    VIN_DEV_01,
    VIN_DEV_02,
    VIN_DEV_03,
    VIN_DEV_BUTT
};

/* VIN管道号 */
enum {
    VIN_PIPE_00 = 0,
    VIN_PIPE_01,
    VIN_PIPE_02,
    VIN_PIPE_03,
    VIN_PIPE_04,
    VIN_PIPE_05,
    VIN_PIPE_06,
    VIN_PIPE_07,
    VIN_PIPE_BUTT
};

/* VIN设备通道号 */
enum {
    VIN_DEV_CHN_00 = 0,
    VIN_DEV_CHN_01,
    VIN_DEV_CHN_02,
    VIN_DEV_CHN_BUTT
};

/* IVPS组ID号 */
enum {
    IVPS_GRP_00 = 0,
    IVPS_GRP_01,
    IVPS_GRP_02,
    IVPS_GRP_03,
    IVPS_GRP_04,
    IVPS_GRP_05,
    IVPS_GRP_06,
    IVPS_GRP_07,
    IVPS_GRP_08,
    IVPS_GRP_09,
    IVPS_GRP_10,
    IVPS_GRP_11,
    IVPS_GRP_12,
    IVPS_GRP_13,
    IVPS_GRP_14,
    IVPS_GRP_15,
    IVPS_GRP_16,
    IVPS_GRP_17,
    IVPS_GRP_18,
    IVPS_GRP_19,
    IVPS_GRP_BUTT
};

/* IPVS组通道号 */
enum {
    IPVS_GRP_CHN_00 = 0,
    IPVS_GRP_CHN_01,
    IPVS_GRP_CHN_02,
    IPVS_GRP_CHN_03,
    IPVS_GRP_CHN_04,
    IPVS_GRP_CHN_BUTT
};

/* IPVS的Filter号 */
enum {
    IPVS_FILTER_00 = 0,
    IPVS_FILTER_01,
    IPVS_FILTER_BUTT
};

/* IVPS区域号 */
enum {
    IPVS_RGN_00 = 0,
    IPVS_RGN_01,
    IPVS_RGN_02,
    IPVS_RGN_03,
    IPVS_RGN_04,
    IPVS_RGN_05,
    IPVS_RGN_06,
    IPVS_RGN_07,
    IPVS_RGN_08,
    IPVS_RGN_09,
    IPVS_RGN_10,
    IPVS_RGN_11,
    IPVS_RGN_12,
    IPVS_RGN_13,
    IPVS_RGN_14,
    IPVS_RGN_15,
    IPVS_RGN_16,
    IPVS_RGN_17,
    IPVS_RGN_18,
    IPVS_RGN_19,
    IPVS_RGN_20,
    IPVS_RGN_21,
    IPVS_RGN_22,
    IPVS_RGN_23,
    IPVS_RGN_24,
    IPVS_RGN_25,
    IPVS_RGN_26,
    IPVS_RGN_27,
    IPVS_RGN_28,
    IPVS_RGN_29,
    IPVS_RGN_30,
    IPVS_RGN_31,
    IPVS_RGN_BUTT
};

/* OSD区域数 */
enum {
    OSD_RGN_00 = 0,
    OSD_RGN_01,
    OSD_RGN_02,
    OSD_RGN_03,
    OSD_RGN_04,
    OSD_RGN_BUTT
};

/* VDEC组号 */
enum {
    VDEC_GRP_00 = 0,
    VDEC_GRP_01,
    VDEC_GRP_02,
    VDEC_GRP_03,
    VDEC_GRP_04,
    VDEC_GRP_05,
    VDEC_GRP_06,
    VDEC_GRP_07,
    VDEC_GRP_08,
    VDEC_GRP_09,
    VDEC_GRP_10,
    VDEC_GRP_11,
    VDEC_GRP_12,
    VDEC_GRP_13,
    VDEC_GRP_14,
    VDEC_GRP_15,
    VDEC_GRP_BUTT
};

/* VENC通道数 */
enum {
    VENC_CHN_00 = 0,
    VENC_CHN_01,
    VENC_CHN_02,
    VENC_CHN_03,
    VENC_CHN_04,
    VENC_CHN_05,
    VENC_CHN_06,
    VENC_CHN_07,
    VENC_CHN_08,
    VENC_CHN_09,
    VENC_CHN_10,
    VENC_CHN_11,
    VENC_CHN_12,
    VENC_CHN_13,
    VENC_CHN_14,
    VENC_CHN_15,
    VENC_CHN_16,
    VENC_CHN_17,
    VENC_CHN_18,
    VENC_CHN_19,
    VENC_CHN_20,
    VENC_CHN_21,
    VENC_CHN_22,
    VENC_CHN_23,
    VENC_CHN_24,
    VENC_CHN_25,
    VENC_CHN_26,
    VENC_CHN_27,
    VENC_CHN_28,
    VENC_CHN_29,
    VENC_CHN_30,
    VENC_CHN_31,
    VENC_CHN_32,
    VENC_CHN_33,
    VENC_CHN_34,
    VENC_CHN_35,
    VENC_CHN_36,
    VENC_CHN_37,
    VENC_CHN_38,
    VENC_CHN_39,
    VENC_CHN_40,
    VENC_CHN_41,
    VENC_CHN_42,
    VENC_CHN_43,
    VENC_CHN_44,
    VENC_CHN_45,
    VENC_CHN_46,
    VENC_CHN_47,
    VENC_CHN_48,
    VENC_CHN_49,
    VENC_CHN_50,
    VENC_CHN_51,
    VENC_CHN_52,
    VENC_CHN_53,
    VENC_CHN_54,
    VENC_CHN_55,
    VENC_CHN_56,
    VENC_CHN_57,
    VENC_CHN_58,
    VENC_CHN_59,
    VENC_CHN_60,
    VENC_CHN_61,
    VENC_CHN_62,
    VENC_CHN_63,
    VENC_CHN_BUTT
};

typedef AX_VIDEO_FRAME_S video_frame_t;
typedef AX_IVPS_PIPELINE_ATTR_S ipvs_pipeline_attr_t;
typedef AX_IVPS_RGN_DISP_GROUP_S ivps_region_disp_group_t;

#endif
