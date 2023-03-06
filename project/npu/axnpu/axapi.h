#ifndef NPU_AXNPU_AXAPI_H
#define NPU_AXNPU_AXAPI_H

#include <string>
#include <stdint.h>
#include <utils/export.h>

#include "axnpu.h"

API_BEGIN_NAMESPACE(Ai)

#define MAX_BBOX_COUNT                   64
#define MAX_FACE_BBOX_COUNT              64
#define MAX_YOLOV5_MASK_OBJ_COUNT        8
#define MAX_POSE_COUNT                   3
#define OBJ_NAME_MAX_LEN                 20
#define MAX_HAND_BBOX_COUNT              2
#define RINGBUFFER_CACHE_COUNT           8
#define FACE_FEAT_LEN                    512

#define PLATE_LMK_SIZE                   4
#define FACE_LMK_SIZE                    5
#define BODY_LMK_SIZE                    17
#define ANIMAL_LMK_SIZE                  20
#define HAND_LMK_SIZE                    21

enum {
    MT_UNKNOWN = -1,

    MT_DET     = 0x10000,
    MT_DET_YOLOV5,
    MT_DET_YOLOV5_FACE,
    MT_DET_YOLOV5_LICENSE_PLATE,
    MT_DET_YOLOV6,
    MT_DET_YOLOV7,
    MT_DET_YOLOV7_FACE,
    MT_DET_YOLOV7_PALM_HAND,
    MT_DET_YOLOX,
    MT_DET_NANODET,
    MT_DET_YOLOX_PPL,
    MT_DET_PALM_HAND,
    MT_DET_YOLOPV2,
    MT_DET_YOLO_FASTBODY,
    MT_DET_SCRFD,
    MT_DET_YOLOV8,
    MT_DET_YOLOV8_SEG,
    MT_DET_CROWD_COUNT,

    MT_SEG      = 0x20000,
    MT_SEG_PPHUMSEG,

    MT_INSEG    = 0x30000,
    MT_INSEG_YOLOV5_MASK,

    MT_MLM      = 0x40000,
    MT_MLM_HUMAN_POSE_AXPPL,
    MT_MLM_HUMAN_POSE_HRNET,
    MT_MLM_ANIMAL_POSE_HRNET,
    MT_MLM_HAND_POSE,
    MT_MLM_FACE_RECOGNITION,
    MT_MLM_VEHICLE_LICENSE_RECOGNITION,

    MT_BUTT,
};

enum {
    RUNNER_UNKNOWN = MT_BUTT,
    RUNNER_AX620A,
    RUNNER_BUTT
};

typedef struct {
    float x;
    float y;
    float w;
    float h;
} axbbox_t;

typedef struct {
    float x;
    float y;
} axpoint_t;

typedef struct {
    int           width;
    int           height;
    unsigned char *data;
} axmat_t;

typedef struct {
    axbbox_t  bbox;
    int       bHasBoxVertices;
    axpoint_t bbox_vertices[4];
    int       nLandmark;
    axpoint_t *landmark;
    int       bHasMask;
    axmat_t   mYolov5Mask;
    int       bHasFaceFeat;
    axmat_t   mFaceFeat;
    int       label;
    float     prob;
    char      objname[OBJ_NAME_MAX_LEN];
} axobj_t;

typedef struct {
    int       mModelType;
    int       nObjSize;
    axobj_t   mObjects[MAX_BBOX_COUNT];
    int       bPPHumSeg;
    axmat_t   mPPHumSeg;
    int       bYolopv2Mask;
    axmat_t   mYolopv2seg;
    axmat_t   mYolopv2ll;
    int       nCrowdCount;
    axpoint_t *mCrowdCountPts;
    int       niFps;
    int       noFps;
} axres_t;

typedef struct {
    unsigned char *data;
    int           width;
    int           height;
    int           channel;
} axcanvas_t;

enum {
    COLOR_SPACE_UNK = 0,
    COLOR_SPACE_NV12,
    COLOR_SPACE_NV21,
    COLOR_SPACE_BGR,
    COLOR_SPACE_RGB,
};

typedef struct {
    uint64_t phyaddr;
    void     *viraddr;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    int      dtype;
    union {
        int  strideW;
        int  strideH;
        int  strideC;
    };
} axframe_t;

int axnpu_init(void **handler, std::string model);
void axnpu_deinit(void **handler);

int axnpu_get_ivps_width_height(void *handler, int *width_ivps, int *height_ivps);

int axnpu_get_model_type(void *handler);
int axnpu_get_color_space(void *handler);

int axnpu_forward(void *handler, axframe_t *pstFrame, axres_t *pResults);
int axnpu_draw_results(void *handler, axcanvas_t *canvas, axres_t *pResults, float fontscale, int thickness, int offset_x, int offset_y);

API_END_NAMESPACE(Ai)

#endif