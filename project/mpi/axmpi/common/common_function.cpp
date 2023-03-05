#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "../axmpi.h"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

#include "common_function.hpp"

API_BEGIN_NAMESPACE(media)

static axsys_pool_cfg_t gtSysCommPoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 15},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4},
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR,      5},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      4},
    {1280, 720,  1280,  AX_YUV420_SEMIPLANAR,     4},
};

static axsys_pool_cfg_t gtSysCommPoolSingleOs04a10OnlineSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 3},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4},
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR,      3},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      2},
    {1280, 720,  1280, AX_YUV420_SEMIPLANAR,      2},
};

static axsys_pool_cfg_t gtSysCommPoolSingleOs04a10Hdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 17},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5},
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR,      6},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      6},
    {720,  576,  720,  AX_YUV420_SEMIPLANAR,      6},
};

static axsys_pool_cfg_t gtSysCommPoolSingleOs04a10OnlineHdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 6},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4},
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR,     3},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,     2},
    {720,  576,  720,  AX_YUV420_SEMIPLANAR,     2},
};

static axsys_pool_cfg_t gtSysCommPoolSingleImx334Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_12BPP, 15},
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR,      6},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      3},
    {960,  540,  960,  AX_YUV420_SEMIPLANAR,      3},
};

static axsys_pool_cfg_t gtSysCommPoolSingleImx334Hdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP, 17},
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR,      6},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      6},
    {960,  540,  960,  AX_YUV420_SEMIPLANAR,      6},
};

static axsys_pool_cfg_t gtSysCommPoolSingleGc4653[] = {
    {2560, 1440, 2560, AX_FORMAT_BAYER_RAW_10BPP, 3},
    {2560, 1440, 2560, AX_FORMAT_BAYER_RAW_16BPP, 4},
    {2560, 1440, 2560, AX_YUV420_SEMIPLANAR,      2},
    {1280, 720,  1280, AX_YUV420_SEMIPLANAR,      2},
    {640,  360,  640,  AX_YUV420_SEMIPLANAR,      2},
};

static axsys_pool_cfg_t gtSysCommPoolDoubleOs04a10[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 15 * 2},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5 * 2},
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR,      6 * 2},
    {1344, 760,  1344, AX_YUV420_SEMIPLANAR,      3 * 2},
    {1344, 760,  1344, AX_YUV420_SEMIPLANAR,      3 * 2},
};

static axsys_pool_cfg_t gtSysCommPoolSingleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_12BPP, 15},
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR,      6},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      3},
    {960,  540,  960,  AX_YUV420_SEMIPLANAR,      3},
};

static axsys_pool_cfg_t gtSysCommPoolSingleOs08a20Hdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP, 17},
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR,      6},
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR,      6},
    {960,  540,  960,  AX_YUV420_SEMIPLANAR,      6},
};

static axsys_pool_cfg_t gtSysCommPoolSingleDVP[] = {
    {1600, 300, 1600, AX_FORMAT_BAYER_RAW_8BPP,   40},
    {1600, 300, 1600, AX_FORMAT_BAYER_RAW_16BPP,  5},
    {1600, 300, 1600, AX_YUV422_INTERLEAVED_UYVY, 6},
};

static axsys_pool_cfg_t gtSysCommPoolBT601[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP,  40},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP,  5},
    {2688, 1520, 2688, AX_YUV422_INTERLEAVED_YUYV, 6},
    {1920, 1080, 1920, AX_YUV422_INTERLEAVED_YUYV, 3},
    {1280, 720,  1280, AX_YUV422_INTERLEAVED_YUYV, 3},
};

static axsys_pool_cfg_t gtSysCommPoolBT656[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP,  40},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP,  5},
    {2688, 1520, 2688, AX_YUV422_INTERLEAVED_YUYV, 6},
    {1920, 1080, 1920, AX_YUV422_INTERLEAVED_YUYV, 3},
    {1280, 720,  1280, AX_YUV422_INTERLEAVED_YUYV, 3},
};

static axsys_pool_cfg_t gtSysCommPoolBT1120[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP,  40},
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP,  5},
    {2688, 1520, 2688, AX_YUV422_INTERLEAVED_YUYV, 6},
    {1920, 1080, 1920, AX_YUV422_INTERLEAVED_YUYV, 3},
    {1280, 720,  1280, AX_YUV422_INTERLEAVED_YUYV, 3},
};

static axsys_pool_cfg_t gtSysCommPoolMIPI_YUV[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 40},
};

int axcam_setup(axcam_t cams[MAX_CAMERAS], int sysCase, int hadMode, int *snsType, axsys_args_t *pArgs, int frameRate)
{
    if ((sysCase >= SYS_CASE_BUTT) || (sysCase <= SYS_CASE_NONE)) {
        axmpi_error("error case type:[%d], only support:[%d:%d]", sysCase, SYS_CASE_NONE + 1, SYS_CASE_BUTT - 1);
        return -1;
    }

    if (sysCase == SYS_CASE_SINGLE_OS04A10) {
        pArgs->camCount = 1;
        *snsType = OMNIVISION_OS04A10;
        axisp_get_sns_config(OMNIVISION_OS04A10, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        if (hadMode == AX_SNS_LINEAR_MODE) {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleOs04a10Sdr) / sizeof(gtSysCommPoolSingleOs04a10Sdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleOs04a10Sdr;
        } else if (hadMode == AX_SNS_HDR_2X_MODE) {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleOs04a10Hdr) / sizeof(gtSysCommPoolSingleOs04a10Hdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleOs04a10Hdr;
        }

        cams[0].stPipeAttr.ePipeDataSrc = AX_PIPE_SOURCE_DEV_ONLINE;
        cams[0].stSnsAttr.nFrameRate = frameRate;
    } else if (sysCase == SYS_CASE_SINGLE_OS04A10_ONLINE) {
        pArgs->camCount = 1;
        *snsType = OMNIVISION_OS04A10;
        axisp_get_sns_config(OMNIVISION_OS04A10, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        if (hadMode == AX_SNS_LINEAR_MODE) {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleOs04a10OnlineSdr) / sizeof(gtSysCommPoolSingleOs04a10OnlineSdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleOs04a10OnlineSdr;
        } else if (hadMode == AX_SNS_HDR_2X_MODE) {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleOs04a10OnlineHdr) / sizeof(gtSysCommPoolSingleOs04a10OnlineHdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleOs04a10OnlineHdr;
        }

        cams[0].stPipeAttr.ePipeDataSrc = AX_PIPE_SOURCE_DEV_ONLINE;
        cams[0].stChnAttr.tChnAttr[0].nDepth = 1;
        cams[0].stChnAttr.tChnAttr[1].nDepth = 1;
        cams[0].stChnAttr.tChnAttr[2].nDepth = 1;
        cams[0].stSnsAttr.nFrameRate = frameRate;
    } else if (sysCase == SYS_CASE_SINGLE_IMX334) {
        pArgs->camCount = 1;
        *snsType = SONY_IMX334;
        axisp_get_sns_config(SONY_IMX334, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        if (hadMode == AX_SNS_LINEAR_MODE) {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleImx334Sdr) / sizeof(gtSysCommPoolSingleImx334Sdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleImx334Sdr;
            cams[0].stSnsAttr.eRawType = AX_RT_RAW12;
            cams[0].stDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            cams[0].stPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        } else {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleImx334Hdr) / sizeof(gtSysCommPoolSingleImx334Hdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleImx334Hdr;
        }

        cams[0].stSnsAttr.nFrameRate = frameRate;
    } else if (sysCase == SYS_CASE_SINGLE_GC4653) {
        pArgs->camCount = 1;
        *snsType = GALAXYCORE_GC4653;
        pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleGc4653) / sizeof(gtSysCommPoolSingleGc4653[0]);
        pArgs->poolCfg = gtSysCommPoolSingleGc4653;

        axisp_get_sns_config(GALAXYCORE_GC4653, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);
        cams[0].stSnsAttr.nFrameRate = frameRate;
    } else if (sysCase == SYS_CASE_DUAL_OS04A10) {
        pArgs->camCount = 2;
        *snsType = OMNIVISION_OS04A10;
        axisp_get_sns_config(OMNIVISION_OS04A10, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);
        axisp_get_sns_config(OMNIVISION_OS04A10, &cams[1].stSnsAttr, &cams[1].stSnsClkAttr, &cams[1].stDevAttr, &cams[1].stPipeAttr, &cams[1].stChnAttr);
        pArgs->poolCfgCount = sizeof(gtSysCommPoolDoubleOs04a10) / sizeof(gtSysCommPoolDoubleOs04a10[0]);
        pArgs->poolCfg = gtSysCommPoolDoubleOs04a10;

        cams[0].stSnsClkAttr.nSnsClkIdx = 0;
        cams[1].stSnsClkAttr.nSnsClkIdx = 2;
    } else if (sysCase == SYS_CASE_SINGLE_OS08A20) {
        pArgs->camCount = 1;
        *snsType = OMNIVISION_OS08A20;
        axisp_get_sns_config(OMNIVISION_OS08A20, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        if (hadMode == AX_SNS_LINEAR_MODE) {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleOs08a20Sdr) / sizeof(gtSysCommPoolSingleOs08a20Sdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleOs08a20Sdr;
            cams[0].stSnsAttr.eRawType = AX_RT_RAW12;
            cams[0].stDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            cams[0].stPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        } else {
            pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleOs08a20Hdr) / sizeof(gtSysCommPoolSingleOs08a20Hdr[0]);
            pArgs->poolCfg = gtSysCommPoolSingleOs08a20Hdr;
        }

        cams[0].stSnsAttr.nFrameRate = frameRate;
    } else if (sysCase == SYS_CASE_SINGLE_DVP) {
        pArgs->camCount = 1;
        cams[0].eSnsType = SENSOR_DVP;
        axisp_get_sns_config(SENSOR_DVP, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        pArgs->poolCfgCount = sizeof(gtSysCommPoolSingleDVP) / sizeof(gtSysCommPoolSingleDVP[0]);
        pArgs->poolCfg = gtSysCommPoolSingleDVP;
    } else if (sysCase == SYS_CASE_SINGLE_BT601) {
        pArgs->camCount = 1;
        cams[0].eSnsType = SENSOR_BT601;
        axisp_get_sns_config(SENSOR_BT601, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        pArgs->poolCfgCount = sizeof(gtSysCommPoolBT601) / sizeof(gtSysCommPoolBT601[0]);
        pArgs->poolCfg = gtSysCommPoolBT601;
    } else if (sysCase == SYS_CASE_SINGLE_BT656) {
        pArgs->camCount = 1;
        cams[0].eSnsType = SENSOR_BT656;
        axisp_get_sns_config(SENSOR_BT656, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        pArgs->poolCfgCount = sizeof(gtSysCommPoolBT656) / sizeof(gtSysCommPoolBT656[0]);
        pArgs->poolCfg = gtSysCommPoolBT656;
    } else if (sysCase == SYS_CASE_SINGLE_BT1120) {
        pArgs->camCount = 1;
        cams[0].eSnsType = SENSOR_BT1120;
        axisp_get_sns_config(SENSOR_BT1120, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        pArgs->poolCfgCount = sizeof(gtSysCommPoolBT1120) / sizeof(gtSysCommPoolBT1120[0]);
        pArgs->poolCfg = gtSysCommPoolBT1120;
    } else if (sysCase == SYS_CASE_MIPI_YUV) {
        pArgs->camCount = 1;
        *snsType = MIPI_YUV;
        axisp_get_sns_config(MIPI_YUV, &cams[0].stSnsAttr, &cams[0].stSnsClkAttr, &cams[0].stDevAttr, &cams[0].stPipeAttr, &cams[0].stChnAttr);

        pArgs->poolCfgCount = sizeof(gtSysCommPoolMIPI_YUV) / sizeof(gtSysCommPoolMIPI_YUV[0]);
        pArgs->poolCfg = gtSysCommPoolMIPI_YUV;
    }

    for (int i = 0; i < pArgs->camCount; i++) {
        cams[i].eSnsType = *snsType;
        cams[i].stSnsAttr.eSnsMode = (AX_SNS_HDR_MODE_E)hadMode;
        cams[i].stDevAttr.eSnsMode = (AX_SNS_HDR_MODE_E)hadMode;
        cams[i].stPipeAttr.eSnsMode = (AX_SNS_HDR_MODE_E)hadMode;
        cams[i].stChnAttr.tChnAttr[0].nDepth = 0;
        cams[i].stChnAttr.tChnAttr[1].nDepth = 0;
        cams[i].stChnAttr.tChnAttr[2].nDepth = 0;

        if (i == 0) {
            cams[i].nDevId = 0;
            cams[i].nRxDev = AX_MIPI_RX_DEV_0;
            cams[i].nPipeId = 0;
        } else if (i == 1) {
            cams[i].nDevId = 2;
            cams[i].nRxDev = AX_MIPI_RX_DEV_2;
            cams[i].nPipeId = 2;
        }
    }

    return 0;
}

API_END_NAMESPACE(media)
