#include <string.h>
#include <unistd.h>

#include <ax_venc_api.h>
#include <rtsp/rtspServerWrapper.hpp>

#include "private.hpp"
#include "codec/common_venc.hpp"

namespace axpi {
extern bool checkRtspSessionPipeId(int pipeId);
extern rtsp::rtsp_server_t getRtspServerHandler();
extern rtsp::rtsp_session_t getRtspServerSession(int pipeId);

static void *vencGetFrameThread(void *arg)
{
    int ret = 0;
    short milliSec = 200;
    AX_VENC_STREAM_S stStream;
    AX_VENC_RECV_PIC_PARAM_S stRecvParam;
    axpipe_t *pipe = reinterpret_cast<axpipe_t *>(arg);

    ret = AX_VENC_StartRecvFrame(pipe->venc.channel, &stRecvParam);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] start recv frame failed, return:[%d]", pipe->venc.channel, ret);
        return NULL;
    }

    while (!pipe->bThreadQuit) {
        ret = AX_VENC_GetStream(pipe->venc.channel, &stStream, milliSec);
        if (ret == 0) {
            switch (pipe->outType) {
                case AXPIPE_OUTPUT_RTSP_H264:
                case AXPIPE_OUTPUT_RTSP_H265: {
                    if (checkRtspSessionPipeId(pipe->pipeId)) {
                        rtsp::rtsp_buff_t buff = {0};
                        buff.vsize = stStream.stPack.u32Len;
                        buff.vbuff = stStream.stPack.pu8Addr;
                        buff.vtimstamp = stStream.stPack.u64PTS;

                        rtsp::rtsp_push(getRtspServerHandler(), getRtspServerSession(pipe->pipeId), &buff);
                    }

                    break;
                }

                default:
                    break;
            }

            if (pipe->frameCallback) {
                axpipe_buffer_t buff;
                buff.pipeId = pipe->pipeId;
                buff.outType = pipe->outType;
                buff.width = 0;
                buff.height = 0;
                buff.size = stStream.stPack.u32Len;
                buff.stride = 0;
                buff.dataType = AXPIPE_OUTPUT_NONE;
                buff.virAddr = stStream.stPack.pu8Addr;
                buff.phyAddr = stStream.stPack.ulPhyAddr;
                buff.userData = pipe;
                pipe->frameCallback(&buff, pipe->userData, pipe->userData2);
            }

            ret = AX_VENC_ReleaseStream(pipe->venc.channel, &stStream);
            if (ret != 0) {
                usleep(30000);
            }
        } else {
            usleep(30000);
        }
    }

    return NULL;
}

static bool setJpegParam(axpipe_t *pipe)
{
    int ret = 0;
    AX_VENC_JPEG_PARAM_S stJpegParam;

    memset(&stJpegParam, 0, sizeof(stJpegParam));
    ret = AX_VENC_GetJpegParam(pipe->venc.channel, &stJpegParam);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] get jpeg param failed, return:[%d]", pipe->venc.channel, ret);
        return false;
    }

    stJpegParam.u32Qfactor = 90;
    ret = AX_VENC_SetJpegParam(pipe->venc.channel, &stJpegParam);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] set jpeg param failed, return:[%d]", pipe->venc.channel, ret);
        return false;
    }

    return true;
}

static bool setRcParam(axpipe_t *pipe, int enRcMode)
{
    int ret = 0;
    AX_VENC_RC_PARAM_S stRcParam;

    ret = AX_VENC_GetRcParam(pipe->venc.channel, &stRcParam);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] get rc param failed, return:[%d]", pipe->venc.channel, ret);
        return false;
    }

    if (enRcMode == VENC_RC_MODE_MJPEGCBR) {
        stRcParam.stMjpegCbr.u32BitRate = 4000;
        stRcParam.stMjpegCbr.u32MinQp = 20;
        stRcParam.stMjpegCbr.u32MaxQp = 30;
    } else if (enRcMode == VENC_RC_MODE_MJPEGVBR) {
        stRcParam.stMjpegVbr.u32MaxBitRate = 4000;
        stRcParam.stMjpegVbr.u32MinQp = 20;
        stRcParam.stMjpegVbr.u32MaxQp = 30;
    } else if (enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
        stRcParam.stMjpegFixQp.s32FixedQp = 22;
    }

    ret = AX_VENC_SetRcParam(pipe->venc.channel, &stRcParam);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] set rc param failed, return:[%d]", pipe->venc.channel, ret);
        return false;
    }

    return true;
}

int axpipe_create_venc(axpipe_t *pipe)
{
    if (pipe->venc.channel >= VENC_CHN_COUNT) {
        axmpi_error("Invalid venc chn:[%d], only support:[%d, %d]", pipe->venc.channel, 0, VENC_CHN_COUNT - 1);
        return -1;
    }

    typedef struct {
        int      eRCType;
        uint32_t nMinQp;
        uint32_t nMaxQp;
        uint32_t nMinIQp;
        uint32_t nMaxIQp;
        int      nIntraQpDelta;
    } rc_info_t;

    typedef struct {
        int       ePayloadType;
        uint32_t  nGOP;
        uint32_t  nSrcFrameRate;
        uint32_t  nDstFrameRate;
        uint32_t  nStride;
        int       nInWidth;
        int       nInHeight;
        int       nOutWidth;
        int       nOutHeight;
        int       nOffsetCropX;
        int       nOffsetCropY;
        int       nOffsetCropW;
        int       nOffsetCropH;
        int       eImgFormat;
        rc_info_t stRCInfo;
        int       nBitrate;
    } video_config_t;

    video_config_t config;
    AX_VENC_CHN_ATTR_S stVencChnAttr;

    memset(&config, 0, sizeof(config));
    config.stRCInfo.eRCType = VENC_RC_VBR;
    config.nGOP = 50;
    config.nBitrate = 4000;
    config.stRCInfo.nMinQp = 10;
    config.stRCInfo.nMaxQp = 51;
    config.stRCInfo.nMinIQp = 10;
    config.stRCInfo.nMaxIQp = 51;
    config.stRCInfo.nIntraQpDelta = -2;
    config.nOffsetCropX = 0;
    config.nOffsetCropY = 0;
    config.nOffsetCropW = 0;
    config.nOffsetCropH = 0;

    switch (pipe->outType) {
        case AXPIPE_OUTPUT_VENC_H264:
        case AXPIPE_OUTPUT_RTSP_H264:
            config.ePayloadType = PT_H264;
            break;

        case AXPIPE_OUTPUT_VENC_H265:
        case AXPIPE_OUTPUT_RTSP_H265:
            config.ePayloadType = PT_H265;
            break;

        case AXPIPE_OUTPUT_VENC_MJPG:
            config.ePayloadType = PT_MJPEG;
            break;

        default:
            axmpi_error("Unsupport output type:[%d]", pipe->outType);
            return -2;
    }

    config.nInWidth = pipe->ivps.width;
    config.nInHeight = pipe->ivps.height;
    config.nStride = config.nInWidth;

    switch (pipe->ivps.rotate) {
        case 90:
        case 270:
            config.nInWidth = pipe->ivps.height;
            config.nInHeight = pipe->ivps.width;
            config.nStride = config.nInWidth;
            break;

        default:
            break;
    }

    config.nSrcFrameRate = pipe->ivps.framerate;
    config.nDstFrameRate = pipe->ivps.framerate;

    memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_S));
    stVencChnAttr.stVencAttr.u32MaxPicWidth = 0;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = 0;

    stVencChnAttr.stVencAttr.u32PicWidthSrc = config.nInWidth;
    stVencChnAttr.stVencAttr.u32PicHeightSrc = config.nInHeight;

    stVencChnAttr.stVencAttr.u32CropOffsetX = config.nOffsetCropX;
    stVencChnAttr.stVencAttr.u32CropOffsetY = config.nOffsetCropY;
    stVencChnAttr.stVencAttr.u32CropWidth = config.nOffsetCropW;
    stVencChnAttr.stVencAttr.u32CropHeight = config.nOffsetCropH;
    stVencChnAttr.stVencAttr.u32VideoRange = 1;

    stVencChnAttr.stVencAttr.u32BufSize = config.nStride * config.nInHeight * 3 / 2;
    stVencChnAttr.stVencAttr.u32MbLinesPerSlice = 0;
    stVencChnAttr.stVencAttr.enLinkMode = AX_LINK_MODE;
    stVencChnAttr.stVencAttr.u32GdrDuration = 0;

    stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stVencChnAttr.stVencAttr.enType = (AX_PAYLOAD_TYPE_E)config.ePayloadType;

    switch (stVencChnAttr.stVencAttr.enType) {
        case PT_H265: {
            stVencChnAttr.stVencAttr.enProfile = VENC_HEVC_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = VENC_HEVC_LEVEL_6;
            stVencChnAttr.stVencAttr.enTier = VENC_HEVC_MAIN_TIER;

            if (config.stRCInfo.eRCType == VENC_RC_CBR) {
                AX_VENC_H265_CBR_S stH265Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH265Cbr.u32Gop = config.nGOP;
                stH265Cbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH265Cbr.fr32DstFrameRate = config.nDstFrameRate;
                stH265Cbr.u32BitRate = config.nBitrate;
                stH265Cbr.u32MinQp = config.stRCInfo.nMinQp;
                stH265Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH265Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH265Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH265Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_S));
            } else if (config.stRCInfo.eRCType == VENC_RC_VBR) {
                AX_VENC_H265_VBR_S stH265Vbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH265Vbr.u32Gop = config.nGOP;
                stH265Vbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH265Vbr.fr32DstFrameRate = config.nDstFrameRate;
                stH265Vbr.u32MaxBitRate = config.nBitrate;
                stH265Vbr.u32MinQp = config.stRCInfo.nMinQp;
                stH265Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH265Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH265Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH265Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_S));
            } else if (config.stRCInfo.eRCType == VENC_RC_FIXQP) {
                AX_VENC_H265_FIXQP_S stH265FixQp;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265FIXQP;
                stH265FixQp.u32Gop = config.nGOP;
                stH265FixQp.u32SrcFrameRate = config.nSrcFrameRate;
                stH265FixQp.fr32DstFrameRate = config.nDstFrameRate;
                stH265FixQp.u32IQp = 25;
                stH265FixQp.u32PQp = 30;
                stH265FixQp.u32BQp = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_S));
            }

            break;
        }

        case PT_H264: {
            stVencChnAttr.stVencAttr.enProfile = VENC_H264_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = VENC_H264_LEVEL_5_2;

            if (config.stRCInfo.eRCType == VENC_RC_CBR) {
                AX_VENC_H264_CBR_S stH264Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH264Cbr.u32Gop = config.nGOP;
                stH264Cbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH264Cbr.fr32DstFrameRate = config.nDstFrameRate;
                stH264Cbr.u32BitRate = config.nBitrate;
                stH264Cbr.u32MinQp = config.stRCInfo.nMinQp;
                stH264Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH264Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH264Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH264Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_S));
            } else if (config.stRCInfo.eRCType == VENC_RC_VBR) {
                AX_VENC_H264_VBR_S stH264Vbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH264Vbr.u32Gop = config.nGOP;
                stH264Vbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH264Vbr.fr32DstFrameRate = config.nDstFrameRate;
                stH264Vbr.u32MaxBitRate = config.nBitrate;
                stH264Vbr.u32MinQp = config.stRCInfo.nMinQp;
                stH264Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH264Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH264Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH264Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_S));
            } else if (config.stRCInfo.eRCType == VENC_RC_FIXQP) {
                AX_VENC_H264_FIXQP_S stH264FixQp;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
                stH264FixQp.u32Gop = config.nGOP;
                stH264FixQp.u32SrcFrameRate = config.nSrcFrameRate;
                stH264FixQp.fr32DstFrameRate = config.nDstFrameRate;
                stH264FixQp.u32IQp = 25;
                stH264FixQp.u32PQp = 30;
                stH264FixQp.u32BQp = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_S));
            }

            break;
        }

        case PT_MJPEG: {
            if (config.stRCInfo.eRCType == VENC_RC_CBR) {
                AX_VENC_MJPEG_CBR_S stMjpegCbrAttr;
                memset(&stMjpegCbrAttr, 0, sizeof(stMjpegCbrAttr));
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
                stMjpegCbrAttr.u32StatTime = 1;
                stMjpegCbrAttr.u32SrcFrameRate = config.nSrcFrameRate;
                stMjpegCbrAttr.fr32DstFrameRate = config.nDstFrameRate;
                stMjpegCbrAttr.u32BitRate = 4000;
                stMjpegCbrAttr.u32MinQp = 20;
                stMjpegCbrAttr.u32MaxQp = 30;
                memcpy(&stVencChnAttr.stRcAttr.stMjpegCbr, &stMjpegCbrAttr, sizeof(AX_VENC_MJPEG_CBR_S));
            } else if (config.stRCInfo.eRCType == VENC_RC_VBR) {
                AX_VENC_MJPEG_VBR_S stMjpegVbrAttr;
                memset(&stMjpegVbrAttr, 0, sizeof(stMjpegVbrAttr));
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGVBR;
                stMjpegVbrAttr.u32StatTime = 1;
                stMjpegVbrAttr.u32SrcFrameRate = config.nSrcFrameRate;
                stMjpegVbrAttr.fr32DstFrameRate = config.nDstFrameRate;
                stMjpegVbrAttr.u32MaxBitRate = 4000;
                stMjpegVbrAttr.u32MinQp = 20;
                stMjpegVbrAttr.u32MaxQp = 30;
                memcpy(&stVencChnAttr.stRcAttr.stMjpegVbr, &stMjpegVbrAttr, sizeof(AX_VENC_MJPEG_VBR_S));
            } else if (config.stRCInfo.eRCType == VENC_RC_FIXQP) {
                AX_VENC_MJPEG_FIXQP_S stMjpegFixQpAttr;
                memset(&stMjpegFixQpAttr, 0, sizeof(stMjpegFixQpAttr));
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;
                stMjpegFixQpAttr.u32SrcFrameRate = config.nSrcFrameRate;
                stMjpegFixQpAttr.fr32DstFrameRate = config.nDstFrameRate;
                stMjpegFixQpAttr.s32FixedQp = 22;
                memcpy(&stVencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegFixQpAttr, sizeof(AX_VENC_MJPEG_FIXQP_S));
            }

            break;
        }

        default:
            axmpi_error("Unsupport venc type:[%d]", stVencChnAttr.stVencAttr.enType);
            return -3;
    }

    int ret = AX_VENC_CreateChn(pipe->venc.channel, &stVencChnAttr);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] create failed, return:[%d]", pipe->venc.channel, ret);
        return -4;
    }

    if (pipe->outType == AXPIPE_OUTPUT_VENC_MJPG) {
        setRcParam(pipe, stVencChnAttr.stRcAttr.enRcMode);
        setJpegParam(pipe);
    }

    if (pthread_create(&pipe->venc.threadId, NULL, vencGetFrameThread, pipe) != 0) {
        axmpi_error("create venc get frame thread failed");
        return -5;
    }

    return 0;
}

int axpipe_release_venc(axpipe_t *pipe)
{
    if (pipe->venc.threadId) {
        pthread_join(pipe->venc.threadId, NULL);
    }

    int ret = AX_VENC_StopRecvFrame(pipe->venc.channel);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] stop recv frame failed, return:[%d]", pipe->venc.channel, ret);
        return -1;
    }

    ret = AX_VENC_DestroyChn(pipe->venc.channel);
    if (ret != 0) {
        axmpi_error("venc chn:[%d] destroy failed, return:[%d]", pipe->venc.channel, ret);
        return -2;
    }

    return 0;
}
}
