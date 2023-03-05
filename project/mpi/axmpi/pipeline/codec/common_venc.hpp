#ifndef MPI_AXMPI_PIPELINE_CODEC_COMMON_VENC_HPP
#define MPI_AXMPI_PIPELINE_CODEC_COMMON_VENC_HPP

#if !defined(__AXERA_PIPELINE_HPP_INSIDE__)
#error "can't be included directly."
#endif

#include <stdint.h>
#include <utils/export.h>

#include <ax_sys_api.h>
#include <ax_comm_venc.h>
#include <ax_base_type.h>
#include <ax_global_type.h>

API_BEGIN_NAMESPACE(media)

#define MAX_MOSAIC_NUM          12

#define VENC_CHN_NUM_MAX        (64)
#define VENC_CHN_NUM            (2)
#define CHN_NUM                 VENC_CHN_NUM
#define INVALID_DEFAULT         (-255)
#define MAX_CU_SIZE             (64)
#define CLIP3(x, y, z)          ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))
#define MIN(a, b)               ((a) < (b) ? (a) : (b))

enum {
    VENC_CODEC_NONDE = 0,
    VENC_CODEC_HEVC,
    VENC_CODEC_H264,
};

enum {
    VENC_RC_NONE = -1,
    VENC_RC_CBR  = 0,
    VENC_RC_VBR,
    VENC_RC_AVBR,
    VENC_RC_QPMAP,
    VENC_RC_FIXQP,
    VENC_RC_BUTT
};

typedef struct {
    int                 enCompressMode;
    uint32_t            UVheaderSize;
    uint32_t            UVpayloadSize;
    uint32_t            YheaderSize;
    uint32_t            YpayloadSize;
    uint32_t            CropX;
    uint32_t            CropY;

    char                *input;
    char                *output;

    int                outputFrameRate;
    int                inputFrameRate;

    int                nSrcWidth;
    int                nSrcHeight;
    int                picStride[3];

    int                inputFormat;
    int                codecFormat;

    int                picture_cnt;
    int                chnNum;
    int                syncType;

    int                enableCabac;

    int                intraAreaEnable;
    int                intraAreaTop;
    int                intraAreaLeft;
    int                intraAreaBottom;
    int                intraAreaRight;

    int                ipcm1AreaTop;
    int                ipcm1AreaLeft;
    int                ipcm1AreaBottom;
    int                ipcm1AreaRight;

    int                ipcm2AreaTop;
    int                ipcm2AreaLeft;
    int                ipcm2AreaBottom;
    int                ipcm2AreaRight;

    int                ipcm3AreaTop;
    int                ipcm3AreaLeft;
    int                ipcm3AreaBottom;
    int                ipcm3AreaRight;

    int                ipcm4AreaTop;
    int                ipcm4AreaLeft;
    int                ipcm4AreaBottom;
    int                ipcm4AreaRight;

    int                ipcm5AreaTop;
    int                ipcm5AreaLeft;
    int                ipcm5AreaBottom;
    int                ipcm5AreaRight;

    int                ipcm6AreaTop;
    int                ipcm6AreaLeft;
    int                ipcm6AreaBottom;
    int                ipcm6AreaRight;

    int                ipcm7AreaTop;
    int                ipcm7AreaLeft;
    int                ipcm7AreaBottom;
    int                ipcm7AreaRight;

    int                ipcm8AreaTop;
    int                ipcm8AreaLeft;
    int                ipcm8AreaBottom;
    int                ipcm8AreaRight;
    int                ipcmMapEnable;
    char               *ipcmMapFile;

    char               *skipMapFile;
    int                skipMapEnable;
    int                skipMapBlockUnit;

    AX_VENC_ROI_ATTR_S roiAttr[MAX_ROI_NUM];

    int                hrdConformance;
    int                cpbSize;
    int                gopLength;

    int                rcMode;
    int                qpHdr;
    int                qpMin;
    int                qpMax;
    int                qpMinI;
    int                qpMaxI;
    int                bitRate;

    bool               loopEncode;

    int                picRc;
    int                ctbRc;
    int                blockRCSize;
    uint32_t           rcQpDeltaRange;
    uint32_t           rcBaseMBComplexity;
    int                picSkip;
    int                picQpDeltaMin;
    int                picQpDeltaMax;
    int                ctbRcRowQpStep;

    float              tolCtbRcInter;
    float              tolCtbRcIntra;

    int                bitrateWindow;
    int                intraQpDelta;

    int                disableDeblocking;
    int                enableSao;

    bool               enablePerfTest;
    int                frameNum;

    int                profile;
    int                tier;
    int                level;

    int                sliceSize;
    int                rotation;

    bool               enableCrop;
    int                cropWidth;
    int                cropHeight;
    int                horOffsetSrc;
    int                verOffsetSrc;

    int                enableDeblockOverride;
    int                deblockOverride;

    int                fieldOrder;
    int                videoRange;
    int                sei;
    char               *userData;
    int                gopType;

    int                gdrDuration;
    uint32_t           roiMapDeltaQpBlockUnit;
    uint32_t           roiMapDeltaQpEnable;
    char               *roiMapDeltaQpFile;
    char               *roiMapDeltaQpBinFile;
    char               *roiMapInfoBinFile;
    char               *RoimapCuCtrlInfoBinFile;
    char               *RoimapCuCtrlIndexBinFile;
    uint32_t           RoiCuCtrlVer;
    uint32_t           RoiQpDeltaVer;

    int                argc;
    char               **argv;

    int                constChromaEn;
    uint32_t           constCb;
    uint32_t           constCr;

    int                skip_frame_enabled_flag;
    int                skip_frame_poc;

    uint32_t           mosaicEnables;
    uint32_t           mosXoffset[MAX_MOSAIC_NUM];
    uint32_t           mosYoffset[MAX_MOSAIC_NUM];
    uint32_t           mosWidth[MAX_MOSAIC_NUM];
    uint32_t           mosHeight[MAX_MOSAIC_NUM];
} axvenc_cmd_param_t;

typedef struct {
    uint32_t           nQpmapBlockUnit;
    int                enQpmapType;
    int                enCtbRcType;
    int                eImgFormat;
    int                enLinkMode;
    int                enRcType;
    axvenc_cmd_param_t *pCmdl;
} axvenc_chn_param_t;

typedef struct {
    axvenc_chn_param_t tDevAttr[VENC_CHN_NUM_MAX];
    uint32_t           nChnNum;
} axvenc_attr_t;

typedef struct {
    bool               bThreadStart;
    int                vencChn;
    int                width;
    int                height;
    int                stride;
    uint32_t           maxCuSize;
    uint32_t           frameSize;
    uint32_t           userPoolId;
    axvenc_chn_param_t chnParam;
} axvenc_encode_param_t;

typedef struct {
    bool               bThreadStart;
    int                vencChn;
    axvenc_chn_param_t chnParam;
} axvenc_getstream_param_t;

API_END_NAMESPACE(media)

#endif
