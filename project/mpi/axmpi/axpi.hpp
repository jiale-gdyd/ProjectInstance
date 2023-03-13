#pragma once

#include <vector>
#include <string>
#include <cstdbool>

namespace axpi {
#define MAX_BBOX_COUNT                   64
#define MAX_FACE_BBOX_COUNT              64
#define MAX_YOLOV5_MASK_OBJ_COUNT        8
#define MAX_POSE_COUNT                   3
#define OBJ_NAME_MAX_LEN                 20
#define MAX_HAND_BBOX_COUNT              2
#define RINGBUFFER_CACHE_COUNT           8
#define FACE_FEAT_LEN                    512

#define PLATE_LMK_SIZE                  4
#define FACE_LMK_SIZE                   5
#define BODY_LMK_SIZE                   17
#define ANIMAL_LMK_SIZE                 20
#define HAND_LMK_SIZE                   21

#define MAX_LMK_SIZE                    HAND_LMK_SIZE
#define CLASS_ID_COUNT                  5

enum {
    MT_UNKNOWN = -1,

    // 检测类
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

    // 分割类
    MT_SEG = 0x20000,
    MT_SEG_PPHUMSEG,

    // 实例分割类
    MT_INSEG = 0x30000,
    MT_INSEG_YOLOV5_MASK,

    // 多级模型类
    MT_MLM = 0x40000,
    MT_MLM_HUMAN_POSE_AXPPL,
    MT_MLM_HUMAN_POSE_HRNET,
    MT_MLM_ANIMAL_POSE_HRNET,
    MT_MLM_HAND_POSE,
    MT_MLM_FACE_RECOGNITION,
    MT_MLM_VEHICLE_LICENSE_RECOGNITION,

    MT_END,
};

enum {
    RUNNER_UNKNOWN = MT_END,
    RUNNER_AX620,
    RUNNER_END
};

typedef struct {
    float x;
    float y;
    float w;
    float h;
} axpi_bbox_t;

typedef struct {
    float x;
    float y;
} axpi_point_t;

typedef struct {
    size_t        w;
    size_t        h;
    unsigned char *data;
} axpi_mat_t;

typedef struct {
    axpi_bbox_t               bbox;
    bool                      bHasBoxVertices;
    std::vector<axpi_point_t> bbox_vertices;

    std::vector<axpi_point_t> landmark;

    bool                      bHasMask;
    axpi_mat_t                yolov5Mask;

    bool                      bHasFaceFeat;
    axpi_mat_t                faceFeat;

    int                       label;
    float                     prob;
    std::string               objname;
} axpi_object_t;

typedef struct {
    int                        modelType;
    std::vector<axpi_object_t> objects;

    bool                       bPPHumSeg;
    axpi_mat_t                 PPHumSeg;

    bool                       bYolopv2Mask;
    axpi_mat_t                 yolopv2seg;
    axpi_mat_t                 yolopv2ll;

    std::vector<axpi_point_t>  crowdCountPts;

    size_t                     inFps;
    size_t                     outFps;
} axpi_results_t;

typedef struct {
    unsigned char *data;
    size_t        width;
    size_t        height;
    size_t        channel;
} axpi_canvas_t;

enum {
    AXPI_COLOR_SPACE_UNK,
    AXPI_COLOR_SPACE_NV12,
    AXPI_COLOR_SPACE_NV21,
    AXPI_COLOR_SPACE_BGR,
    AXPI_COLOR_SPACE_RGB,
};

typedef struct {
    unsigned long long int phy;
    void                   *vir;
    unsigned int           size;
    unsigned int           width;
    unsigned int           height;
    int                    dtype;
    union {
        unsigned int       strideH;
        unsigned int       strideW;
        unsigned int       strideC;
    };
} axpi_image_t;

int axpi_exit(void **handler);
int axpi_init(std::string confJsonFile, void **handler);

int axpi_get_model_type(void *handler);
int axpi_get_color_space(void *handler);
int axpi_get_ivps_width_height(void *handler, std::string confJsonFile, size_t &width, size_t &height);

int axpi_inference(void *handler, axpi_image_t *pstFrame, axpi_results_t *lastResults);
int axpi_draw_results(void *handler, axpi_canvas_t *canvas, axpi_results_t *lastResults, float fontscale, int thickness, int offsetX, int offsetY);
}
