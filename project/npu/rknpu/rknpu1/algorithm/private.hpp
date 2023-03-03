#ifndef AARCH32_ROCKCHIP_ALGORITHM_PRIVATE_HPP
#define AARCH32_ROCKCHIP_ALGORITHM_PRIVATE_HPP

#include <vector>
#include <sys/types.h>
#include <utils/export.h>
#include <npu/AiStruct.hpp>
#include <rknn/rknn_runtime.h>

API_BEGIN_NAMESPACE(Ai)

#define OBJ_NAME_MAX_SIZE       16
#define OBJ_NUMB_MAX_SIZE       64

typedef struct {
    float                         nms_threshold;         // nms阈值
    float                         conf_threshold;        // 目标框过滤阈值
    size_t                        classCount;            // 类被总数
    size_t                        realWidth;             // 实际图像宽度
    size_t                        realHeight;            // 实际图像高度
    size_t                        imageWidth;            // 图像原始宽度
    size_t                        imageHeight;           // 图像原始高度
    size_t                        modelInputWidth;       // 模型输入图像宽
    size_t                        modelInputHeight;      // 模型输入图像高
    size_t                        hstride;               // 水平跨度
    size_t                        vstride;               // 垂直跨度
    int                           algo_author;           // 算法作者
    std::vector<int>              filterClass;           // 过滤类别
    std::vector<yolo_layer_t>     yoloAnchors;           // YOLO输出层
    std::vector<rknn_tensor_attr> outputsAttr;           // rknn张量输出属性
} algo_args_t;

API_END_NAMESPACE(Ai)

#endif
