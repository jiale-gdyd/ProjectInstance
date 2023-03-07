#ifndef NPU_AXNPU_AXSTRUCT_HPP
#define NPU_AXNPU_AXSTRUCT_HPP

#include <stdint.h>
#include <stdbool.h>
#include <utils/export.h>

#define AXDL_BBOX_MAX       (64)

API_BEGIN_NAMESPACE(Ai)

enum {
    AXDL_COLOR_SPACE_UNK,
    AXDL_COLOR_SPACE_NV12,
    AXDL_COLOR_SPACE_NV21,
    AXDL_COLOR_SPACE_BGR,
    AXDL_COLOR_SPACE_RGB
};

enum {
    MT_UNKNOWN = -1,

    // 检测
    MT_DET = 0x10000,
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

    // 分割
    MT_SEG = 0x20000,
    MT_SEG_PPHUMSEG,

    // 实例分割
    MT_INSEG = 0x30000,
    MT_INSEG_YOLOV5_MASK,

    // 多级模型
    MT_MLM = 0x40000,
    MT_MLM_HUMAN_POSE_AXPPL,
    MT_MLM_HUMAN_POSE_HRNET,
    MT_MLM_ANIMAL_POSE_HRNET,
    MT_MLM_HAND_POSE,
    MT_MLM_FACE_RECOGNITION,
    MT_MLM_VEHICLE_LICENSE_RECOGNITION,
};

typedef struct {
    uint64_t    phy;
    void        *vir;
    uint32_t    size;
    uint32_t    width;
    uint32_t    height;
    int32_t     dtype;
    union {
        int32_t strideH;
        int32_t strideW;
        int32_t strideC;
    };
} axdl_frame_t;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} axdl_bbox_t;

typedef struct {
    float x;
    float y;
} axdl_point_t;

typedef struct {
    int width;
    int height;
} axdl_mat_t;

typedef struct {
    axdl_bbox_t  bbox;
    bool         bHasBoxVertices;
    axdl_point_t bboxVertices[4];

    int          landmarks;
    axdl_point_t *landmark;

    bool         bHasMark;
    axdl_mat_t   yoloMask;

    bool         bHasFaceFeat;
    axdl_mat_t   faceFeat;

    int          label;
    float        prob;
} axdl_object_t;

typedef struct {
    int           modelType;
    int           objectSize;
    axdl_object_t objects[AXDL_BBOX_MAX];

    bool          bPPHumSeg;
    axdl_mat_t    pphumSeg;

    bool          bYolopv2Mask;
    axdl_mat_t    yolopv2ll;
    axdl_mat_t    yolopv2Seg;

    int           crowdCount;
    axdl_point_t  *crowdPoints;

    int           inFps;
    int           outFps;
} axdl_result_t;

typedef struct {
    uint8_t *data;
    int     width;
    int     height;
    int     channels;
} axdl_canvas_t;

API_END_NAMESPACE(Ai)

#endif
