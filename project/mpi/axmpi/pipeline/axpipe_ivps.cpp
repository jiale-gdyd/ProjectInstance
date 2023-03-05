#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define __AXERA_PIPELINE_HPP_INSIDE__
#include "private.hpp"
#undef __AXERA_PIPELINE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

#ifndef ALIGN_UP
#define ALIGN_UP(x, align)          ((((x) + ((align)-1)) / (align)) * (align))
#endif

static void *ivpsGetFrameThread(void *arg)
{
    int ret = 0;
    int milliSec = 200;
    axpipe_t *pipe = reinterpret_cast<axpipe_t *>(arg);

    while (!pipe->bThreadQuit) {
        AX_VIDEO_FRAME_S stVideoFrame;
        ret = AX_IVPS_GetChnFrame(pipe->ivpsConfig.ivpsGroup, 0, &stVideoFrame, milliSec);
        if (ret != 0) {
            if (ret == AX_ERR_IVPS_BUF_EMPTY) {
                usleep(1000);
                continue;
            }

            usleep(1000);
            continue;
        }

        stVideoFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(stVideoFrame.u32BlkId[0]);
        stVideoFrame.u64VirAddr[0] = (AX_U32)AX_POOL_GetBlockVirAddr(stVideoFrame.u32BlkId[0]);

        if (pipe->frameCallback) {
            axpipe_buffer_t buff;

            buff.pipeId = pipe->pipeId;
            buff.outType = pipe->outType;
            buff.width = stVideoFrame.u32Width;
            buff.height = stVideoFrame.u32Height;
            buff.stride = (0 == stVideoFrame.u32PicStride[0]) ? stVideoFrame.u32Width : stVideoFrame.u32PicStride[0];

            switch ((int)stVideoFrame.enImgFormat) {
                case AX_YUV420_SEMIPLANAR:
                    buff.size = stVideoFrame.u32PicStride[0] * stVideoFrame.u32Height * 3 / 2;
                    buff.dataType = AXPIPE_OUTPUT_BUFF_NV12;
                    break;

                case AX_FORMAT_RGB888:
                    buff.size = stVideoFrame.u32PicStride[0] * stVideoFrame.u32Height * 3;
                    buff.dataType = AXPIPE_OUTPUT_BUFF_RGB;
                    break;

                case AX_FORMAT_BGR888:
                    buff.size = stVideoFrame.u32PicStride[0] * stVideoFrame.u32Height * 3;
                    buff.dataType = AXPIPE_OUTPUT_BUFF_BGR;
                    break;

                default:
                    buff.dataType = AXPIPE_OUTPUT_NONE;
                    break;
            }

            buff.virAddr = (unsigned char *)stVideoFrame.u64VirAddr[0];
            buff.phyAddr = stVideoFrame.u64PhyAddr[0];
            buff.userData = pipe;
            pipe->frameCallback(&buff);
        }

        AX_IVPS_ReleaseChnFrame(pipe->ivpsConfig.ivpsGroup, 0, &stVideoFrame);
    }

    return NULL;
}

int axpipe_create_ivps(axpipe_t *pipe)
{
    if (pipe->ivpsConfig.ivpsGroup >= IVPS_GRP_BUTT) {
        axmpi_error("Invalid ivps group:[%d], only support:[%d:%d]", pipe->ivpsConfig.ivpsGroup, IVPS_GRP_00, IVPS_GRP_BUTT - 1);
        return -1;
    }

    int ret = 0;
    int chnId = 0;
    AX_IVPS_GRP_ATTR_S stGrpAttr = {0};
    int grpId = pipe->ivpsConfig.ivpsGroup;
    AX_IVPS_PIPELINE_ATTR_S stPipelineAttr = {0};

    stPipelineAttr.tFbInfo.PoolId = AX_INVALID_POOLID;
    stPipelineAttr.nOutChnNum = 1;

    stGrpAttr.nInFifoDepth = 1;
    stGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;
    ret = AX_IVPS_CreateGrp(grpId, &stGrpAttr);
    if (ret != 0) {
        axmpi_error("ivps group:[%d] create failed, return:[%d]", grpId, ret);
        return -2;
    }

    memset(&stPipelineAttr.tFilter, 0x00, sizeof(stPipelineAttr.tFilter));

    stPipelineAttr.tFilter[chnId][0].bEnable = AX_TRUE;
    stPipelineAttr.tFilter[chnId][0].tFRC.nSrcFrameRate = pipe->ivpsConfig.ivpsFramerate;
    stPipelineAttr.tFilter[chnId][0].tFRC.nDstFrameRate = pipe->ivpsConfig.ivpsFramerate;
    stPipelineAttr.tFilter[chnId][0].nDstPicOffsetX0 = 0;
    stPipelineAttr.tFilter[chnId][0].nDstPicOffsetY0 = 0;
    stPipelineAttr.tFilter[chnId][0].nDstPicWidth = pipe->ivpsConfig.ivpsWidth;
    stPipelineAttr.tFilter[chnId][0].nDstPicHeight = pipe->ivpsConfig.ivpsHeight;
    stPipelineAttr.tFilter[chnId][0].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[chnId][0].nDstPicWidth, 64);
    stPipelineAttr.tFilter[chnId][0].nDstFrameWidth = pipe->ivpsConfig.ivpsWidth;
    stPipelineAttr.tFilter[chnId][0].nDstFrameHeight = pipe->ivpsConfig.ivpsHeight;
    stPipelineAttr.tFilter[chnId][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
    stPipelineAttr.tFilter[chnId][0].eEngine = AX_IVPS_ENGINE_TDP;

    if (pipe->ivpsConfig.bLetterBbox) {
        AX_IVPS_ASPECT_RATIO_S tAspectRatio;
        tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_AUTO;
        tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
        tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
        tAspectRatio.nBgColor = 0x0000FF;
        stPipelineAttr.tFilter[chnId][0].tTdpCfg.tAspectRatio = tAspectRatio;
    }

    stPipelineAttr.tFilter[chnId][0].tTdpCfg.bFlip = pipe->ivpsConfig.bIvpsFlip > 0 ? AX_TRUE : AX_FALSE;
    stPipelineAttr.tFilter[chnId][0].tTdpCfg.bMirror = pipe->ivpsConfig.bIvpsMirror > 0 ? AX_TRUE : AX_FALSE;
    if (pipe->ivpsConfig.ivpsRotate == 0) {
        stPipelineAttr.tFilter[chnId][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_0;
    } else if (pipe->ivpsConfig.ivpsRotate == 90) {
        stPipelineAttr.tFilter[chnId][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_90;
    } else if (pipe->ivpsConfig.ivpsRotate == 180) {
        stPipelineAttr.tFilter[chnId][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_180;
    } else if (pipe->ivpsConfig.ivpsRotate == 270) {
        stPipelineAttr.tFilter[chnId][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_270;
    }

    switch (stPipelineAttr.tFilter[chnId][0].tTdpCfg.eRotation) {
        case AX_IVPS_ROTATION_90:
        case AX_IVPS_ROTATION_270:
            stPipelineAttr.tFilter[chnId][0].nDstPicWidth = pipe->ivpsConfig.ivpsHeight;
            stPipelineAttr.tFilter[chnId][0].nDstPicHeight = pipe->ivpsConfig.ivpsWidth;
            stPipelineAttr.tFilter[chnId][0].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[chnId][0].nDstPicWidth, 64);
            stPipelineAttr.tFilter[chnId][0].nDstFrameWidth = pipe->ivpsConfig.ivpsHeight;
            stPipelineAttr.tFilter[chnId][0].nDstFrameHeight = pipe->ivpsConfig.ivpsWidth;
            break;

        default:
            break;
    }

    switch (pipe->outType) {
        case AXPIPE_OUTPUT_BUFF_RGB:
            stPipelineAttr.tFilter[chnId][0].eDstPicFormat = AX_FORMAT_RGB888;
            break;

        case AXPIPE_OUTPUT_BUFF_BGR:
            stPipelineAttr.tFilter[chnId][0].eDstPicFormat = AX_FORMAT_BGR888;
            break;

        case AXPIPE_OUTPUT_BUFF_NV21:
            stPipelineAttr.tFilter[chnId][0].eDstPicFormat = AX_YUV420_SEMIPLANAR_VU;
            break;

        case AXPIPE_OUTPUT_BUFF_NV12:
        case AXPIPE_OUTPUT_VENC_MJPG:
        case AXPIPE_OUTPUT_VENC_H264:
        case AXPIPE_OUTPUT_VENC_H265:
        case AXPIPE_OUTPUT_RTSP_H264:
        case AXPIPE_OUTPUT_RTSP_H265:
        case AXPIPE_OUTPUT_VO_USER_SCREEN:
        case AXPIPE_OUTPUT_VO_SIPEED_SCREEN:
            stPipelineAttr.tFilter[chnId][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
            break;

        default:
            break;
    }

    stPipelineAttr.nOutFifoDepth[chnId] = pipe->ivpsConfig.fifoCount;
    if (stPipelineAttr.nOutFifoDepth[chnId] < 0) {
        stPipelineAttr.nOutFifoDepth[chnId] = 0;
    }

    if (stPipelineAttr.nOutFifoDepth[chnId] > 4) {
        stPipelineAttr.nOutFifoDepth[chnId] = 4;
    }

    ret = AX_IVPS_SetPipelineAttr(grpId, &stPipelineAttr);
    if (ret != 0) {
        axmpi_error("ivps group:[%d] set pipeline attribute failed, return:[%d]", grpId, ret);
        return -3;
    }

    ret = AX_IVPS_EnableChn(grpId, chnId);
    if (ret != 0) {
        axmpi_error("enable ivps group:[%d] chn:[%d] failed, return:[%d]", grpId, chnId, ret);
        return -4;
    }

    ret = AX_IVPS_StartGrp(grpId);
    if (ret != 0) {
        axmpi_error("start ivps group:[%d] failed, return:[%d]", grpId, ret);
        return -5;
    }

    for (int i = 0; (i < pipe->ivpsConfig.osdRegions) && (i < OSD_RGN_BUTT); ++i) {
        IVPS_RGN_HANDLE hChnRgn = AX_IVPS_RGN_Create();
        if (hChnRgn != AX_IVPS_INVALID_REGION_HANDLE) {
            int fileter = 0x00;
            ret = AX_IVPS_RGN_AttachToFilter(hChnRgn, pipe->ivpsConfig.ivpsGroup, fileter);
            if (ret != 0) {
                axmpi_error("attach region handler:[%d] to ivps group:[%d] filter:[%d] failed, return:[%d]", hChnRgn, pipe->ivpsConfig.ivpsGroup, fileter, ret);
                pipe->ivpsConfig.osdRegions = i;
                break;
            }

            pipe->ivpsConfig.osdRgnHandle[i] = hChnRgn;
            continue;
        }

        pipe->ivpsConfig.osdRegions = i;
        break;
    }

    switch (pipe->outType) {
        case AXPIPE_OUTPUT_BUFF_RGB:
        case AXPIPE_OUTPUT_BUFF_BGR:
        case AXPIPE_OUTPUT_BUFF_NV12:
        case AXPIPE_OUTPUT_BUFF_NV21: {
            if (stPipelineAttr.nOutFifoDepth[chnId] > 0) {
                ret = pthread_create(&pipe->ivpsConfig.tid, NULL, ivpsGetFrameThread, (void *)pipe);
                if (ret != 0) {
                    axmpi_error("create ivps get frame thread failed");
                    return -6;
                }
            }

            break;
        }

        default:
            break;
    }

    return 0;
}

int axpipe_release_ivps(axpipe_t *pipe)
{
    pthread_join(pipe->ivpsConfig.tid, NULL);

    int ret = AX_IVPS_StopGrp(pipe->ivpsConfig.ivpsGroup);
    if (ret != 0) {
        axmpi_error("stop ivps group:[%d] failed, return:[%d]", pipe->ivpsConfig.ivpsGroup, ret);
        return -1;
    }

    ret = AX_IVPS_DisableChn(pipe->ivpsConfig.ivpsGroup, 0);
    if (ret != 0) {
        axmpi_error("disable ivps group:[%d] chn:[0] failed, return:[%d]", pipe->ivpsConfig.ivpsGroup, ret);
        return -2;
    }

    ret = AX_IVPS_DestoryGrp(pipe->ivpsConfig.ivpsGroup);
    if (ret != 0) {
        axmpi_error("destroy ivps group:[%d] failed", pipe->ivpsConfig.ivpsGroup, ret);
        return -3;
    }

    return 0;
}

API_END_NAMESPACE(media)
