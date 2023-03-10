#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

#include <ax_ivps_api.h>
#include <ax_vdec_api.h>
#include <ax_venc_api.h>
#include <rtsp/rtspServerWrapper.hpp>

#include "private.hpp"
#include "../utilities/netutils.hpp"

namespace axpi {
#undef DEFAULT_RTSP_SERVER_PORT
#define DEFAULT_RTSP_SERVER_PORT        8554

typedef struct {
    std::map<int, axpipe_t *>           pipeIdPipe;
    bool                                bMaxi3Init = false;
    bool                                bUserVoInit = false;
    rtsp::rtsp_server_t                 rtspServerHandler;
    std::map<int, rtsp::rtsp_session_t> rtspServerSessions;
    std::vector<std::string>            rtspEndPoint;
    std::vector<int>                    ivps_group;
    std::vector<int>                    vdec_channel;
    std::vector<int>                    venc_channel;
} axpipe_internal_t;

static axpipe_internal_t sg_axPipeHandler;

static void print_rtsp(std::string url, uint16_t port)
{
    char ipaddr[64] = {0};
    int ret = get_ip("eth0", ipaddr);
    if (ret == 0) {
        axmpi_info("[eth0]  rtsp url rtsp://%s:%d/%s", ipaddr, port, url.c_str());
    }

    ret = get_ip("usb0", ipaddr);
    if (ret == 0) {
        axmpi_info("[usb0]  rtsp url rtsp://%s:%d/%s", ipaddr, port, url.c_str());
    }

    ret = get_ip("wlan0", ipaddr);
    if (ret == 0) {
        axmpi_info("[wlan0] rtsp url rtsp://%s:%d/%s", ipaddr, port, url.c_str());
    }
}

template <typename T>
bool contain(std::vector<T> &v, T &t)
{
    auto item = std::find(v.begin(), v.end(), t);
    if (item != v.end()) {
        return true;
    }

    return false;
}

template <typename KT, typename VT>
bool contain(std::map<KT, VT> &v, KT &t)
{
    auto item = v.find(t);
    if (item != v.end()) {
        return true;
    }

    return false;
}

template <typename T>
bool erase(std::vector<T> &v, T &t)
{
    auto item = std::find(v.begin(), v.end(), t);
    if (item != v.end()) {
        v.erase(item);
        return true;
    }

    return false;
}

template <typename KT, typename VT>
bool erase(std::map<KT, VT> &v, KT &t)
{
    auto item = v.find(t);
    if (item != v.end()) {
        v.erase(item);
        return true;
    }

    return false;
}

bool checkRtspSessionPipeId(int pipeId)
{
    return contain(sg_axPipeHandler.rtspServerSessions, pipeId);
}

rtsp::rtsp_server_t getRtspServerHandler()
{
    return sg_axPipeHandler.rtspServerHandler;
}

rtsp::rtsp_session_t getRtspServerSession(int pipeId)
{
    return sg_axPipeHandler.rtspServerSessions[pipeId];
}

int axpipe_create(axpipe_t *pipe)
{
    if (!pipe) {
        axmpi_error("Invalid pipeline, maybe nullptr");
        return -1;
    }

    if (!pipe->enable) {
        axmpi_warn("current pipeline id:[%d] not enable", pipe->pipeId);
        return -2;
    }

    if (axpi::contain(sg_axPipeHandler.pipeIdPipe, pipe->pipeId)) {
        axmpi_warn("current pipeline id:[%d] has been created", pipe->pipeId);
        return -3;
    }

    sg_axPipeHandler.pipeIdPipe[pipe->pipeId] = pipe;

    switch (pipe->inType) {
        case AXPIPE_INPUT_USER: {
            if (sg_axPipeHandler.ivps_group.size() == 0) {
                int ret = AX_IVPS_Init();
                if (ret != 0) {
                    axmpi_error("AX_IVPS_Init failed, return:[%d]", ret);
                    return -4;
                }
            }

            if (axpi::contain(sg_axPipeHandler.ivps_group, pipe->ivps.group)) {
                axmpi_warn("ivps group:[%d] has been created", pipe->ivps.group);
                return -5;
            }

            sg_axPipeHandler.ivps_group.push_back(pipe->ivps.group);
            int ret = axpipe_create_ivps(pipe);
            if (ret != 0) {
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                axmpi_error("axpipe_create_ivps failed, return:[%d]", ret);
                return -6;
            }

            break;
        }

        case AXPIPE_INPUT_VIN: {
            if (pipe->vinPipe >= VIN_PIPE_COUNT) {
                axmpi_error("vin pipe must lower than %d, current:[%d]", VIN_PIPE_COUNT, pipe->vinPipe);
                return -7;
            }

            if (pipe->vinChn >= VIN_CHN_COUNT) {
                axmpi_error("vin chn must lower than %d, current:[%d]", VIN_CHN_COUNT, pipe->vinChn);
                return -8;
            }

            if (sg_axPipeHandler.ivps_group.size() == 0) {
                int ret = AX_IVPS_Init();
                if (ret != 0) {
                    axmpi_error("AX_IVPS_Init failed, return:[%d]", ret);
                    return -9;
                }
            }

            if (axpi::contain(sg_axPipeHandler.ivps_group, pipe->ivps.group)) {
                axmpi_warn("ivps group:[%d] has been created", pipe->ivps.group);
                return -10;
            }

            sg_axPipeHandler.ivps_group.push_back(pipe->ivps.group);
            int ret = axpipe_create_ivps(pipe);
            if (ret != 0) {
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                axmpi_error("axpipe_create_ivps failed, return:[%d]", ret);
                return -11;
            }

            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_VIN;
            srcMod.s32GrpId = pipe->vinPipe;
            srcMod.s32ChnId = pipe->vinChn;
            dstMod.enModId = AX_ID_IVPS;
            dstMod.s32GrpId = pipe->ivps.group;
            dstMod.s32ChnId = 0;
            ret = AX_SYS_Link((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
            if (ret != 0) {
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                axmpi_error("bind vin:[%d:%d] to ivps:[%d:0] failed, return:[%d]", pipe->vinPipe, pipe->vinChn, pipe->ivps.group, 0, ret);
                return -12;
            }

            break;
        }

        case AXPIPE_INPUT_VDEC_H264:
        case AXPIPE_INPUT_VDEC_JPEG: {
            if (sg_axPipeHandler.ivps_group.size() == 0) {
                int ret = AX_IVPS_Init();
                if (ret != 0) {
                    axmpi_error("AX_IVPS_Init failed, return:[%d]", ret);
                    return -13;
                }
            }

            if (axpi::contain(sg_axPipeHandler.ivps_group, pipe->ivps.group)) {
                axmpi_warn("ivps group:[%d] has been created", pipe->ivps.group);
                return -14;
            }

            sg_axPipeHandler.ivps_group.push_back(pipe->ivps.group);
            int ret = axpipe_create_ivps(pipe);
            if (ret != 0) {
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                axmpi_error("axpipe_create_ivps failed, return:[%d]", ret);
                return -15;
            }

            if (sg_axPipeHandler.vdec_channel.size() == 0) {
                ret = AX_VDEC_Init();
                if (ret != 0) {
                    axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                    axmpi_error("AX_VDEC_Init failed, return:[%d]", ret);
                    return -16;
                }
            }

            if (!axpi::contain(sg_axPipeHandler.vdec_channel, pipe->vdec.channel)) {
                ret = axpipe_create_vdec(pipe);
                if (ret != 0) {
                    axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                    axmpi_error("axpipe_create_vdec failed, return:[%d]", ret);
                    return -17;
                }

                sg_axPipeHandler.vdec_channel.push_back(pipe->vdec.channel);
            }

            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_VDEC;
            srcMod.s32GrpId = pipe->vdec.channel;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_IVPS;
            dstMod.s32GrpId = pipe->ivps.group;
            dstMod.s32ChnId = 0;
            ret = AX_SYS_Link((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
            if (ret != 0) {
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
                axpi::erase(sg_axPipeHandler.vdec_channel, pipe->vdec.channel);
                axmpi_error("bind vdec:[%d:%d] to ivps:[%d:%d] failed, return:[%d]", pipe->vdec.channel, 0, pipe->ivps.group, 0, ret);
                return -18;
            }

            break;
        }

        default:
            axmpi_warn("Unsupported input type:[%d]", pipe->inType);
            break;
    }

    switch (pipe->outType) {
        case AXPIPE_OUTPUT_VENC_MJPG:
        case AXPIPE_OUTPUT_VENC_H264:
        case AXPIPE_OUTPUT_VENC_H265:
        case AXPIPE_OUTPUT_RTSP_H264:
        case AXPIPE_OUTPUT_RTSP_H265: {
            if (sg_axPipeHandler.venc_channel.size() == 0) {
                AX_VENC_MOD_ATTR_S stVencModAttr;
                memset(&stVencModAttr, 0, sizeof(stVencModAttr));
                stVencModAttr.enVencType = VENC_MULTI_ENCODER;

                int ret = AX_VENC_Init((const AX_VENC_MOD_ATTR_S *)&stVencModAttr);
                if (ret != 0) {
                    axmpi_error("AX_VENC_Init failed, return:[%d]", ret);
                    return -19;
                }
            }

            if (axpi::contain(sg_axPipeHandler.venc_channel, pipe->venc.channel)) {
                axmpi_warn("venc chn:[%d] has been created", pipe->venc.channel);
                return -20;
            }

            sg_axPipeHandler.venc_channel.push_back(pipe->venc.channel);

            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = pipe->ivps.group;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_VENC;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = pipe->venc.channel;
            int ret = AX_SYS_Link((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
            if (ret != 0) {
                axpi::erase(sg_axPipeHandler.venc_channel, pipe->venc.channel);
                axmpi_error("bind ivps:[%d:%d] to venc:[%d:%d] failed, return:[%d]", pipe->ivps.group, 0, 0, pipe->venc.channel, ret);
                return -21;
            }

            ret = axpipe_create_venc(pipe);
            if (ret != 0) {
                AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
                axpi::erase(sg_axPipeHandler.venc_channel, pipe->venc.channel);
                axmpi_error("axpipe_create_venc failed, return:[%d]", ret);
                return -22;
            }

            if ((pipe->outType == AXPIPE_OUTPUT_RTSP_H264) || (pipe->outType == AXPIPE_OUTPUT_RTSP_H265)) {
                uint16_t rtspPort = pipe->venc.rtspPort ? pipe->venc.rtspPort : DEFAULT_RTSP_SERVER_PORT;
                if (!sg_axPipeHandler.rtspServerHandler) {
                    sg_axPipeHandler.rtspServerHandler = rtsp::rtsp_new_server(rtspPort);
                }

                std::string url = pipe->venc.endPoint;
                if (!axpi::contain(sg_axPipeHandler.rtspEndPoint, url) && !axpi::contain(sg_axPipeHandler.rtspServerSessions, pipe->pipeId)) {
                    auto rtspSession = rtsp::rtsp_new_session(sg_axPipeHandler.rtspServerHandler, url.c_str(), pipe->outType == AXPIPE_OUTPUT_RTSP_H264 ? 0 : 1);
                    sg_axPipeHandler.rtspServerSessions[pipe->pipeId] = rtspSession;
                    sg_axPipeHandler.rtspEndPoint.push_back(url);

                    print_rtsp(url, rtspPort);
                } else {
                    axmpi_warn("rtsp url:[%s] has been created", url.c_str());
                }
            }

            break;
        }

        case AXPIPE_OUTPUT_VO_SIPEED_SCREEN: {
            if (!sg_axPipeHandler.bMaxi3Init) {
                AX_MOD_INFO_S srcMod, dstMod;
                srcMod.enModId = AX_ID_IVPS;
                srcMod.s32GrpId = pipe->ivps.group;
                srcMod.s32ChnId = 0;
                dstMod.enModId = AX_ID_VO;
                dstMod.s32GrpId = 0;
                dstMod.s32ChnId = 0;

                int ret = AX_SYS_Link((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
                if (ret != 0) {
                    axmpi_error("bind ivps:[%d:%d] to vo:[%d:%d] failed, return:[%d]", pipe->ivps.group, 0, 0, 0, ret);
                    return -23;
                }

                ret = axpipe_create_vo("dsi0@480x854@60", pipe);
                if (ret != 0) {
                    AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
                    axmpi_error("axpipe_create_vo failed, return:[%d]", ret);
                    return -24;
                }

                sg_axPipeHandler.bMaxi3Init = true;
            }

            break;
        }

        case AXPIPE_OUTPUT_VO_USER_SCREEN: {
            if (!sg_axPipeHandler.bUserVoInit) {
                AX_MOD_INFO_S srcMod, dstMod;
                srcMod.enModId = AX_ID_IVPS;
                srcMod.s32GrpId = pipe->ivps.group;
                srcMod.s32ChnId = 0;
                dstMod.enModId = AX_ID_VO;
                dstMod.s32GrpId = 0;
                dstMod.s32ChnId = 0;

                int ret = AX_SYS_Link((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
                if (ret != 0) {
                    axmpi_error("bind ivps:[%d:%d] to vo:[%d:%d] failed, return:[%d]", pipe->ivps.group, 0, 0, 0, ret);
                    return -23;
                }

                ret = axpipe_create_vo(pipe->vo.dispDevStr, pipe);
                if (ret != 0) {
                    AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);
                    axmpi_error("axpipe_create_vo failed, return:[%d]", ret);
                    return -24;
                }

                sg_axPipeHandler.bUserVoInit = true;
            }

            break;
            break;
        }

        default:
            axmpi_warn("Unsupported output type:[%d]", pipe->outType);
            break;
    }

    return 0;
}

int axpipe_release(axpipe_t *pipe)
{
    if (!pipe) {
        axmpi_error("Invalid pipeline, maybe nullptr");
        return -1;
    }

    if (!pipe->enable) {
        axmpi_warn("current pipeline id:[%d] not enable", pipe->pipeId);
        return -2;
    }

    if (!axpi::contain(sg_axPipeHandler.pipeIdPipe, pipe->pipeId)) {
        axmpi_error("not find pipe:[%d], maybe not initialized", pipe->pipeId);
        return -3;
    }

    pipe->bThreadQuit = true;
    axpi::erase(sg_axPipeHandler.pipeIdPipe, pipe->pipeId);

    switch (pipe->outType) {
        case AXPIPE_OUTPUT_VENC_MJPG:
        case AXPIPE_OUTPUT_VENC_H264:
        case AXPIPE_OUTPUT_VENC_H265:
        case AXPIPE_OUTPUT_RTSP_H264:
        case AXPIPE_OUTPUT_RTSP_H265: {
            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = pipe->ivps.group;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_VENC;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = pipe->venc.channel;
            AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);

            if (axpi::contain(sg_axPipeHandler.venc_channel, pipe->venc.channel)) {
                axpipe_release_venc(pipe);
                axpi::erase(sg_axPipeHandler.venc_channel, pipe->venc.channel);
            }

            if (sg_axPipeHandler.venc_channel.size() == 0) {
                AX_VENC_Deinit();
            }

            if ((pipe->outType == AXPIPE_OUTPUT_RTSP_H264) || (pipe->outType == AXPIPE_OUTPUT_RTSP_H265)) {
                std::string url = pipe->venc.endPoint;
                if (url.length()) {
                    if (url[0] != '/') {
                        url = "/" + url;
                    }
                }

                if (axpi::contain(sg_axPipeHandler.rtspServerSessions, pipe->pipeId)) {
                    rtsp::rtsp_release_session(sg_axPipeHandler.rtspServerHandler, sg_axPipeHandler.rtspServerSessions[pipe->pipeId]);
                    axpi::erase(sg_axPipeHandler.rtspServerSessions, pipe->pipeId);
                }

                if (axpi::contain(sg_axPipeHandler.rtspEndPoint, url)) {
                    axpi::erase(sg_axPipeHandler.rtspEndPoint, url);
                }

                if (sg_axPipeHandler.rtspServerSessions.size() == 0) {
                    rtsp::rtsp_release_server(&(sg_axPipeHandler.rtspServerHandler));
                    sg_axPipeHandler.rtspServerHandler = NULL;
                }
            }

            break;
        }

        case AXPIPE_OUTPUT_VO_SIPEED_SCREEN: {
            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = pipe->ivps.group;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_VO;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 0;
            AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);

            if (sg_axPipeHandler.bMaxi3Init) {
                axpipe_release_vo();
                sg_axPipeHandler.bMaxi3Init = false;
            }

            break;
        }

        case AXPIPE_OUTPUT_VO_USER_SCREEN: {
            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = pipe->ivps.group;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_VO;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 0;
            AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);

            if (sg_axPipeHandler.bUserVoInit) {
                axpipe_release_vo();
                sg_axPipeHandler.bUserVoInit = false;
            }

            break;
        }

        default:
            break;
    }

    switch (pipe->inType) {
        case AXPIPE_INPUT_USER: {
            if (axpi::contain(sg_axPipeHandler.ivps_group, pipe->ivps.group)) {
                axpipe_release_ivps(pipe);
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
            }

            if (sg_axPipeHandler.ivps_group.size() == 0) {
                AX_IVPS_Deinit();
            }

            break;
        }

        case AXPIPE_INPUT_VIN: {
            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_VIN;
            srcMod.s32GrpId = pipe->vinPipe;
            srcMod.s32ChnId = pipe->vinChn;
            dstMod.enModId = AX_ID_IVPS;
            dstMod.s32GrpId = pipe->ivps.group;
            dstMod.s32ChnId = 0;
            AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);

            if (axpi::contain(sg_axPipeHandler.ivps_group, pipe->ivps.group)) {
                axpipe_release_ivps(pipe);
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
            }

            if (sg_axPipeHandler.ivps_group.size() == 0) {
                AX_IVPS_Deinit();
            }
            break;
        }

        case AXPIPE_INPUT_VDEC_H264:
        case AXPIPE_INPUT_VDEC_JPEG: {
            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_VDEC;
            srcMod.s32GrpId = pipe->vdec.channel;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_IVPS;
            dstMod.s32GrpId = pipe->ivps.group;
            dstMod.s32ChnId = 0;
            AX_SYS_UnLink((const AX_MOD_INFO_S *)&srcMod, (const AX_MOD_INFO_S *)&dstMod);

            if (axpi::contain(sg_axPipeHandler.vdec_channel, pipe->vdec.channel)) {
                if (pipe->inType == AXPIPE_INPUT_VDEC_H264) {
                    axpipe_release_vdec(pipe);
                }

                axpi::erase(sg_axPipeHandler.vdec_channel, pipe->vdec.channel);
            }

            if (sg_axPipeHandler.vdec_channel.size() == 0) {
                AX_VDEC_DeInit();
            }

            if (axpi::contain(sg_axPipeHandler.ivps_group, pipe->ivps.group)) {
                axpipe_release_ivps(pipe);
                axpi::erase(sg_axPipeHandler.ivps_group, pipe->ivps.group);
            }

            if (sg_axPipeHandler.ivps_group.size() == 0) {
                AX_IVPS_Deinit();
            }

            break;
        }

        default:
            break;
    }

    return 0;
}

int axpipe_user_input(axpipe_t *pipe, int pipes, axpipe_buffer_t *buff)
{
    if (!pipe) {
        axmpi_error("Invalid pipeline, maybe nullptr");
        return -1;
    }

    if (!buff) {
        axmpi_warn("Invalid buffer, maybe nullptr");
        return -2;
    }

    if (!axpi::contain(sg_axPipeHandler.pipeIdPipe, pipe->pipeId)) {
        axmpi_error("not find pipe:[%d], maybe not initialized", pipe->pipeId);
        return -3;
    }

    switch (pipe->inType) {
        case AXPIPE_INPUT_USER: {
            AX_BLK blk_id;
            uint64_t picSize = 0;
            AX_VIDEO_FRAME_INFO_S frameInfo;

            picSize = (buff->width * buff->height) * 3 / 2;
            blk_id = AX_POOL_GetBlock(frameInfo.u32PoolId, picSize, NULL);
            if (blk_id == AX_INVALID_BLOCKID) {
                axmpi_error("AX_POOL_GetBlock failed");
                return -4;
            }

            frameInfo.bEof = AX_TRUE;
            frameInfo.enModId = AX_ID_IVPS;
            frameInfo.stVFrame.u32BlkId[0] = blk_id;
            frameInfo.stVFrame.u32Width = buff->width;
            frameInfo.stVFrame.u32Height = buff->height;
            frameInfo.stVFrame.enImgFormat = AX_YUV420_SEMIPLANAR;
            frameInfo.stVFrame.enVscanFormat = AX_VSCAN_FORMAT_RASTER;
            frameInfo.stVFrame.enCompressMode = AX_COMPRESS_MODE_NONE;
            frameInfo.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blk_id);
            frameInfo.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blk_id);
            frameInfo.stVFrame.u32PicStride[0] = buff->width;
            frameInfo.stVFrame.u64PhyAddr[1] = frameInfo.stVFrame.u64PhyAddr[0] + frameInfo.stVFrame.u32PicStride[0] * frameInfo.stVFrame.u32Height;
            frameInfo.stVFrame.u64PhyAddr[2] = 0;
            frameInfo.stVFrame.u64VirAddr[1] = frameInfo.stVFrame.u64VirAddr[0] + frameInfo.stVFrame.u32PicStride[0] * frameInfo.stVFrame.u32Height;
            frameInfo.stVFrame.u64VirAddr[2] = 0;
            frameInfo.u32PoolId = AX_POOL_Handle2PoolId(blk_id);
            memcpy((void *)frameInfo.stVFrame.u64VirAddr[0], buff->virAddr, picSize);

            int ret;
            std::vector<int> tmpVec;

            for (int i = 0; i < pipes; ++i) {
                if (!axpi::contain(tmpVec, pipe[i].ivps.group)) {
                    ret = AX_IVPS_SendFrame(pipe[i].ivps.group, &frameInfo.stVFrame, 200);
                    if (ret != 0) {
                        // axmpi_error("ivps group:[%d] send frame failed, retuen:[%d]", pipe[i].ivps.group, ret);
                    }

                    tmpVec.push_back(pipe[i].ivps.group);
                }
            }

            AX_BLK blkId = frameInfo.stVFrame.u32BlkId[0];
            ret = AX_POOL_ReleaseBlock(blkId);
            if (ret != 0) {
                axmpi_error("AX_POOL_ReleaseBlock failed, return:[%d]", ret);
                return -5;
            }

            break;
        }

        case AXPIPE_INPUT_VDEC_H264: {
            int ret;
            uint64_t pts = 0;
            std::vector<int> tmpVec;
            AX_VDEC_STREAM_S frameInfo = {0};

            frameInfo.u64PTS = pts++;
            frameInfo.u32Len = buff->size;
            frameInfo.pu8Addr = (unsigned char *)buff->virAddr;
            frameInfo.bEndOfFrame = buff->virAddr == NULL ? AX_TRUE : AX_FALSE;
            frameInfo.bEndOfStream = buff->virAddr == NULL ? AX_TRUE : AX_FALSE;

            for (int i = 0; i < pipes; ++i) {
                if (!axpi::contain(tmpVec, pipe[i].vdec.channel)) {
                    ret = AX_VDEC_SendStream(pipe[i].vdec.channel, &frameInfo, 200);
                    if (ret != 0) {
                        axmpi_error("vdec group:[%d] send frame failed, return:[%d]", pipe[i].vdec.channel, ret);
                    }

                    tmpVec.push_back(pipe[i].vdec.channel);
                }
            }

            break;
        }

        case AXPIPE_INPUT_VDEC_JPEG: {
            axpipe_create_jdec(pipe);

            int ret;
            uint64_t pts = 0;
            std::vector<int> tmpVec;
            AX_VDEC_STREAM_S frameInfo = {0};

            frameInfo.u64PTS = pts++;
            frameInfo.u32Len = buff->size;
            frameInfo.pu8Addr = (unsigned char *)buff->virAddr;
            frameInfo.bEndOfFrame = buff->virAddr == NULL ? AX_TRUE : AX_FALSE;
            frameInfo.bEndOfStream = buff->virAddr == NULL ? AX_TRUE : AX_FALSE;

            for (int i = 0; i < pipes; ++i) {
                if (!axpi::contain(tmpVec, pipe[i].vdec.channel)) {
                    ret = AX_VDEC_SendStream(pipe[i].vdec.channel, &frameInfo, -1);
                    if (ret != 0) {
                        axmpi_error("jdec group:[%d] send frame failed, return:[%d]", pipe[i].vdec.channel, ret);
                    }

                    tmpVec.push_back(pipe[i].vdec.channel);
                }
            }

            axpipe_release_jdec(pipe);
            break;
        }

        default:
            axmpi_warn("Unsupported input type:[%d]", pipe->inType);
            break;
    }

    return 0;
}
}
