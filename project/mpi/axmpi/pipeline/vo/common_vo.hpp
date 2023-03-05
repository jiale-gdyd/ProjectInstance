#ifndef MPI_AXMPI_PIPELINE_VO_COMMON_VO_HPP
#define MPI_AXMPI_PIPELINE_VO_COMMON_VO_HPP

#if !defined(__AXERA_PIPELINE_HPP_INSIDE__)
#error "can't be included directly."
#endif

#include <stdint.h>
#include <utils/export.h>

#include <ax_vo_api.h>
#include <ax_sys_api.h>
#include <ax_base_type.h>

API_BEGIN_NAMESPACE(media)

#define ALIGN_UP(x, a)              ((((x) + ((a) - 1)) / a) * a)
#define ALIGN_DOWN(x, a)            (((x) / (a)) * (a))

#define AXVO_VLAYER_MAX             2

enum {
    VO_MODE_1MUX,
    VO_MODE_2MUX,
    VO_MODE_4MUX,
    VO_MODE_8MUX,
    VO_MODE_9MUX,
    VO_MODE_16MUX,
    VO_MODE_25MUX,
    VO_MODE_36MUX,
    VO_MODE_49MUX,
    VO_MODE_64MUX,
    VO_MODE_2X4,
    VO_MODE_BUTT
};

typedef VO_RESO_S axvo_reso_t;
typedef AX_MOD_INFO_S ax_mod_info_t;
typedef VO_PUB_ATTR_S axvo_pub_attr_t;
typedef AX_POOL_FLOORPLAN_T ax_pool_floorplan_t;
typedef VO_VIDEO_LAYER_ATTR_S axvo_video_layer_attr_t;

typedef struct {
    uint32_t                voLayer;
    axvo_video_layer_attr_t stVoLayerAttr;
    int                     enVoMode;
    int                     enChnPixFmt;
    uint32_t                u32FifoDepth;
    ax_mod_info_t           stSrcMod;
    ax_mod_info_t           stDstMod;
} axvo_layer_config_t;

typedef struct axSAMPLE_VO_CONFIG_S {
    uint32_t            voDev;
    int                 enVoIntfType;
    int                 enIntfSync;
    axvo_reso_t         stReso;
    uint32_t            u32FifoDepth;
    uint32_t            u32LayerNr;
    axvo_layer_config_t stVoLayer[AXVO_VLAYER_MAX];
    int                 s32EnableGLayer;
    uint32_t            graphicLayer;
    uint32_t            u32CommonPoolCnt;
    ax_pool_floorplan_t stPoolFloorPlan;
} axvo_config_t;

int common_axvo_stop_device(uint32_t voDev);
int common_axvo_start_device(uint32_t voDev, axvo_pub_attr_t *pstPubAttr);

int common_axvo_stop_layer(uint32_t voLayer);
int common_axvo_start_layer(uint32_t voLayer, const axvo_video_layer_attr_t *pstLayerAttr);

int common_axvo_stop_channel(uint32_t voLayer, int enMode);
int common_axvo_start_channel(uint32_t voLayer, int enMode, uint32_t u32FifoDepth);

int common_axvo_stop(axvo_config_t *pstVoConfig);
int common_axvo_start(axvo_config_t *pstVoConfig);

API_END_NAMESPACE(media)

#endif
