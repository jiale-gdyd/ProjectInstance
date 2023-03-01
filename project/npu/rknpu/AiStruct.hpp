#ifndef AARCH32_ROCKCHIP_RKNPU_AISTRUCT_HPP
#define AARCH32_ROCKCHIP_RKNPU_AISTRUCT_HPP

#include <string>
#include <vector>
#include <utils/export.h>

API_BEGIN_NAMESPACE(Ai)

/* 算法结果结构 */
typedef struct {
    float       left;               // 检测框左上角坐标X
    float       top;                // 检测框左上角坐标Y
    float       right;              // 检测框右下角坐标X
    float       bottom;             // 检测框右下角坐标Y
    float       score;              // 检测框置信度
    int         classify;           // 检测目标分类号
    int         trackerId;          // 检测目标跟踪ID
    float       prob;               // 检测框置信度
} bbox;

/* 算法类型 */
enum {
    YOLOXS,                         // yoloxs
    YOLOV5S,                        // yolov5s
    YOLOX_TINY_FACE,                // yolox-tiny face
    YOLOX_NANO_FACE,                // yolox-nano face
    YOLOX_NANO_FACE_TINY,           // yolox-nano face tiny
};

/* 算法作者 */
enum {
    AUTHOR_JIALELU,                 // jialelu
    AUTHOR_ROCKCHIP,                // 瑞星微提供
};

typedef struct {
    int width;                      // 矩形宽
    int height;                     // 矩形高
} rect_t;

typedef struct {
    std::string         name;       // 模型输出层名称
    int                 stride;     // 当前模型输出层跨度
    std::vector<rect_t> anchors;    // 当前模型输出anchor
} yolo_layer_t;

API_END_NAMESPACE(Ai)

#endif
