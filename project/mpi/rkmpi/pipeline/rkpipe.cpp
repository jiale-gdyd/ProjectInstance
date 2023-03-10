#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <rtsp/rtspServerWrapper.hpp>

#include "rkpipe.hpp"
#include "../rkmpi.h"

API_BEGIN_NAMESPACE(mpi)

#undef DEFAULT_RTSP_SERVER_PORT
#define DEFAULT_RTSP_SERVER_PORT        8554

typedef struct {
    bool                                     sysInited;
    std::map<uint32_t, rkpipe_t *>           pipeIdMap;
    rtsp::rtsp_server_t                      rtspServerHandler;
    std::map<uint32_t, rtsp::rtsp_session_t> rtspServerSessions;
    std::vector<std::string>                 rtspEndPoint;
    std::vector<uint32_t>                    voChannel;
    std::vector<uint32_t>                    vinChannel;
    std::vector<uint32_t>                    rgaChannel;
    std::vector<uint32_t>                    vencChannel;
    std::vector<uint32_t>                    vdecChannel;
} rkpipe_private_t;

static rkpipe_private_t g_pipelineHandler;

extern int rkpipe_create_vo(rkpipe_t *pipe);
extern int rkpipe_release_vo(rkpipe_t *pipe);

extern int rkpipe_create_vin(rkpipe_t *pipe);
extern int rkpipe_release_vin(rkpipe_t *pipe);

extern int rkpipe_create_rga(rkpipe_t *pipe);
extern int rkpipe_release_rga(rkpipe_t *pipe);

extern int rkpipe_create_venc(rkpipe_t *pipe);
extern int rkpipe_release_venc(rkpipe_t *pipe);

extern int rkpipe_create_vdec(rkpipe_t *pipe);
extern int rkpipe_release_vdec(rkpipe_t *pipe);

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

bool checkRtspSessionPipeId(uint32_t pipeId)
{
    return contain(g_pipelineHandler.rtspServerSessions, pipeId);
}

rtsp::rtsp_server_t getRtspServerHandler()
{
    return g_pipelineHandler.rtspServerHandler;
}

rtsp::rtsp_session_t getRtspServerSession(uint32_t pipeId)
{
    return g_pipelineHandler.rtspServerSessions[pipeId];
}

int rkpipe_create(rkpipe_t *pipe)
{
    if (!pipe) {
        rkmpi_error("Invalid pipeline handler, shold not be null");
        return -1;
    }

    if (!pipe->enable) {
        rkmpi_warn("current pipeline is not enabled");
        return 0;
    }

    if (mpi::contain(g_pipelineHandler.pipeIdMap, pipe->pipelineId)) {
        rkmpi_error("The current pipeline id:[0x%X] is already in use and should not be reused", pipe->pipelineId);
        return -2;
    }

    auto systemInit = [&]() {
        if (g_pipelineHandler.voChannel.size() == 0
            && g_pipelineHandler.vinChannel.size() == 0
            && g_pipelineHandler.rgaChannel.size() == 0
            && g_pipelineHandler.vencChannel.size() == 0
            && g_pipelineHandler.vdecChannel.size() == 0)
        {
            int ret = drm_mpi_system_init();
            if (ret == 0) {
                g_pipelineHandler.sysInited = true;
            }

            return ret;
        }
    };

    g_pipelineHandler.pipeIdMap[pipe->pipelineId] = pipe;
    switch (pipe->inputType) {
        case RKPIPE_INPUT_VIN: {
            if (systemInit() != 0) {
                return -3;
            }

            if (pipe->vin.vinChannel >= DRM_VI_CHANNEL_BUTT) {
                rkmpi_error("vin channel must lower than:[%d], current:[%u]", DRM_VI_CHANNEL_BUTT - 1, pipe->vin.vinChannel);
                return -4;
            }

            if (mpi::contain(g_pipelineHandler.vinChannel, pipe->vin.vinChannel)) {
                rkmpi_error("vin channel:[%d] has been created", pipe->vin.vinChannel);
                return -5;
            }

            if (rkpipe_create_vin(pipe)) {
                return -6;
            }

            g_pipelineHandler.vinChannel.push_back(pipe->vin.vinChannel);
            break;
        }

        case RKPIPE_INPUT_VDEC_H264:
        case RKPIPE_INPUT_VDEC_H265:
        case RKPIPE_INPUT_VDEC_JPEG: {
            if (systemInit() != 0) {
                return -8;
            }

            if (pipe->vdec.vdecChannel >= DRM_VDEC_CHANNEL_BUTT) {
                rkmpi_error("vdec channel must lower than:[%d], current:[%u]", DRM_VDEC_CHANNEL_BUTT - 1, pipe->vdec.vdecChannel);
                return -9;
            }

            if (mpi::contain(g_pipelineHandler.vdecChannel, pipe->vdec.vdecChannel)) {
                rkmpi_error("vdec channel:[%d] has been created", pipe->vdec.vdecChannel);
                return -10;
            }

            if (rkpipe_create_vdec(pipe)) {
                return -11;
            }

            g_pipelineHandler.vdecChannel.push_back(pipe->vdec.vdecChannel);
            break;
        }

        default:
            rkmpi_error("Unsupport input type:[%d]", pipe->inputType);
            return -12;
    }

    switch (pipe->outputType) {
        case RKPIPE_OUTPUT_VENC_MJPG:
        case RKPIPE_OUTPUT_VENC_H264:
        case RKPIPE_OUTPUT_VENC_H265:
        case RKPIPE_OUTPUT_RTSP_H264:
        case RKPIPE_OUTPUT_RTSP_H265: {
            if (systemInit() != 0) {
                return -13;
            }

            if (pipe->venc.vencChannel >= DRM_VENC_CHANNEL_BUTT) {
                rkmpi_error("venc channel must lower than:[%d], current:[%u]", DRM_VENC_CHANNEL_BUTT - 1, pipe->venc.vencChannel);
                return -14;
            }

            if (mpi::contain(g_pipelineHandler.vencChannel, pipe->venc.vencChannel)) {
                rkmpi_error("venc channel:[%d] has been created", pipe->venc.vencChannel);
                return -15;
            }

            if (rkpipe_create_venc(pipe)) {
                return -16;
            }

            g_pipelineHandler.vencChannel.push_back(pipe->venc.vencChannel);
            if ((pipe->outputType == RKPIPE_OUTPUT_RTSP_H264) || (pipe->outputType == RKPIPE_OUTPUT_RTSP_H265)) {
                if (!g_pipelineHandler.rtspServerHandler) {
                    g_pipelineHandler.rtspServerHandler = rtsp::rtsp_new_server(pipe->venc.rtspServerPort ? pipe->venc.rtspServerPort : DEFAULT_RTSP_SERVER_PORT);
                }

                std::string url = pipe->venc.rtspServerEndPoint;
                if (url.length()) {
                    if (url[0] != '/') {
                        url = "/" + url;
                    }
                }

                if (!mpi::contain(g_pipelineHandler.rtspEndPoint, url) && !mpi::contain(g_pipelineHandler.rtspServerSessions, pipe->pipelineId)) {
                    auto rtspSession = rtsp::rtsp_new_session(g_pipelineHandler.rtspServerHandler, url.c_str(), pipe->outputType == RKPIPE_OUTPUT_RTSP_H264 ? 0 : 1);
                    g_pipelineHandler.rtspServerSessions[pipe->pipelineId] = rtspSession;
                    g_pipelineHandler.rtspEndPoint.push_back(url);
                }
            }

            break;
        }

        case RKPIPE_OUTPUT_VO_USER_SCREEN: {
            if (systemInit() != 0) {
                return -16;
            }

            if (pipe->vo.voChannel >= DRM_VO_CHANNEL_BUTT) {
                rkmpi_error("vo channel must lower than:[%d], current:[%u]", DRM_VO_CHANNEL_BUTT - 1, pipe->vo.voChannel);
                return -14;
            }

            if (mpi::contain(g_pipelineHandler.voChannel, pipe->vo.voChannel)) {
                rkmpi_error("vo channel:[%d] has been created", pipe->vo.voChannel);
                return -15;
            }

            if (rkpipe_create_vo(pipe)) {
                return -16;
            }

            g_pipelineHandler.voChannel.push_back(pipe->vo.voChannel);
            break;
        }

        default:
            rkmpi_error("Unsupport output type:[%d]", pipe->outputType);
            return -13;
    }

    // switch (pipe->bindRelation) {

    // }

    return 0;
}

int rkpipe_release(rkpipe_t *pipe)
{
    if (!pipe) {
        rkmpi_error("Invalid pipeline, maybe nullptr");
        return -1;
    }

    if (!pipe->enable) {
        rkmpi_error("current pipeline id:[%d] not enable", pipe->pipelineId);
        return -2;
    }

    if (!mpi::contain(g_pipelineHandler.pipeIdMap, pipe->pipelineId)) {
        rkmpi_error("not find pipe:[%d], maybe not initialized", pipe->pipelineId);
        return -3;
    }

    pipe->quitThread = true;
    mpi::erase(g_pipelineHandler.pipeIdMap, pipe->pipelineId);

    switch (pipe->outputType) {
        case RKPIPE_OUTPUT_VENC_MJPG:
        case RKPIPE_OUTPUT_VENC_H264:
        case RKPIPE_OUTPUT_VENC_H265:
        case RKPIPE_OUTPUT_RTSP_H264:
        case RKPIPE_OUTPUT_RTSP_H265: {
            /* 如果有绑定，则先取消绑定 */

            if (mpi::contain(g_pipelineHandler.vencChannel, pipe->venc.vencChannel)) {
                rkpipe_release_venc(pipe);
                mpi::erase(g_pipelineHandler.vencChannel, pipe->venc.vencChannel);
            }

            if ((pipe->outputType == RKPIPE_OUTPUT_RTSP_H264) || (pipe->outputType == RKPIPE_OUTPUT_RTSP_H265)) {
                std::string url = pipe->venc.rtspServerEndPoint;
                if (url.length()) {
                    if (url[0] != '/') {
                        url = "/" + url;
                    }
                }

                if (mpi::contain(g_pipelineHandler.rtspServerSessions, pipe->pipelineId)) {
                    rtsp::rtsp_release_session(g_pipelineHandler.rtspServerHandler, g_pipelineHandler.rtspServerSessions[pipe->pipelineId]);
                    mpi::erase(g_pipelineHandler.rtspServerSessions, pipe->pipelineId);
                }

                if (mpi::contain(g_pipelineHandler.rtspEndPoint, url)) {
                    mpi::erase(g_pipelineHandler.rtspEndPoint, url);
                }

                if (g_pipelineHandler.rtspServerSessions.size() == 0) {
                    rtsp::rtsp_release_server(&(g_pipelineHandler.rtspServerHandler));
                    g_pipelineHandler.rtspServerHandler = NULL;
                }
            }

            break;
        }

        case RKPIPE_OUTPUT_VO: {
            /* 如果有绑定，则先取消绑定 */

            if (mpi::contain(g_pipelineHandler.voChannel, pipe->vo.voChannel)) {
                rkpipe_release_vo(pipe);
                mpi::erase(g_pipelineHandler.voChannel, pipe->vo.voChannel);
            }
            break;
        }

        default:
            break;
    }

    auto systemDeinit = [&]() {
        if (g_pipelineHandler.voChannel.size() == 0
            && g_pipelineHandler.vinChannel.size() == 0
            && g_pipelineHandler.rgaChannel.size() == 0
            && g_pipelineHandler.vencChannel.size() == 0
            && g_pipelineHandler.vdecChannel.size() == 0)
        {
            g_pipelineHandler.sysInited = false;
        }
    };

    switch (pipe->inputType) {
        case RKPIPE_INPUT_VIN: {
            /* 如果有绑定，则先取消绑定 */

            if (mpi::contain(g_pipelineHandler.vinChannel, pipe->vin.vinChannel)) {
                rkpipe_release_vin(pipe);
                mpi::erase(g_pipelineHandler.vinChannel, pipe->vin.vinChannel);
            }

            systemDeinit();
            break;
        }

        case RKPIPE_INPUT_VDEC_H264:
        case RKPIPE_INPUT_VDEC_H265:
        case RKPIPE_INPUT_VDEC_JPEG: {
            /* 如果有绑定，则先取消绑定 */

            if (mpi::contain(g_pipelineHandler.vdecChannel, pipe->vdec.vdecChannel)) {
                rkpipe_release_vdec(pipe);
                mpi::erase(g_pipelineHandler.vdecChannel, pipe->vdec.vdecChannel);
            }

            systemDeinit();
            break;
        }

        default:
            break;
    }

    return 0;
}

API_END_NAMESPACE(mpi)
