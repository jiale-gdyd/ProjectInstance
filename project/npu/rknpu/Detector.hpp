#ifndef AARCH32_ROCKCHIP_RKNPU_DETECTOR_HPP
#define AARCH32_ROCKCHIP_RKNPU_DETECTOR_HPP

#include <vector>
#include <stdbool.h>
#include "AiStruct.hpp"

API_BEGIN_NAMESPACE(Ai)

class AlgoDetector;
class ObjectTracker;

class API_EXPORT AiDetector {
public:
    /* 算法构造器 */
    AiDetector();

    /* 算法析构器 */
    ~AiDetector();

    /**
     * 函数名称: init
     * 功能描述: 算法初始化
     * 输入参数: modelFile     --> rknn模型文件
     *          imageWidth    --> 输入图像宽度(原图)
     *          imageHeight   --> 输入图像高度(原图)
     *          classCount    --> 分类检测目标类别数
     *          confThreshold --> 检测目标置信度过滤阈值
     *          nmsThreshold  --> 非极大值抑制过滤阈值
     *          algoType      --> 算法类型
     *          algoAuthor    --> 算法提供作者
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int init(std::string modelFile, size_t imageWidth, size_t imageHeight, size_t classCount, float confThreshold, float nmsThreshold, int algoType = YOLOV5S, int algoAuthor = AUTHOR_ROCKCHIP);

    /**
     * 函数名称: forward
     * 功能描述: 仅将图像送入算法进行推理，不获取推理输出数据
     * 输入参数: mediaFrame --> 原始多媒体数据帧(RGB888)
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int forward(void *mediaFrame);

    /**
     * 函数名称: forward
     * 功能描述: 将图像送入算法进行推理，同时获取推理处理后的检测目标框集
     * 输入参数: mediaFrame --> 原始多媒体数据帧(RGB888)
     * 输出参数: lastResult --> 推理后处理完后的检测目标框集
     * 返回说明: 成功返回0，其他则失败
     */
    int forward(void *mediaFrame, std::vector<bbox> &lastResult);

    /**
     * 函数名称: forward
     * 功能描述: 将图像送入算法进行推理，同时获取推理后处理完以及过滤设定分类后的检测目标框集
     * 输入参数: mediaFrame  --> 原始多媒体数据帧(RGB888)
     *          filterClass --> 需要过滤的分类类别号
     * 输出参数: lastResult  --> 推理后处理完以及过滤设定分类后的检测目标框集
     * 返回说明: 成功返回0，其他则失败
     */
    int forward(void *mediaFrame, std::vector<bbox> &lastResult, std::vector<int> filterClass);

    /**
     * 函数名称: extract
     * 功能描述: 仅将推理后处理完后的目标检测框集输出
     * 输入参数: 无
     * 输出参数: lastResult --> 推理后处理完后的目标检测框集
     * 返回说明: 成功返回0，其他则失败
     */
    int extract(std::vector<bbox> &lastResult);

    /**
     * 函数名称: extract
     * 功能描述: 仅将推理后处理完以及过滤设定的分类后的目标检测框集输出
     * 输入参数: filterClass --> 需要过滤的分类类别号
     * 输出参数: lastResult  --> 推理后处理完以及过滤设定的分类后的目标检测框集
     * 返回说明: 成功返回0，其他则失败
     */
    int extract(std::vector<bbox> &lastResult, std::vector<int> filterClass);

    /**
     * 函数名称: setYoloAnchor
     * 功能描述: 设置YOLO输出层的锚点信息
     * 输入参数: anchor --> YOLO输出层的锚点信息
     * 输出参数: 无
     * 返回说明: 无
     */
    void setYoloAnchor(std::vector<yolo_layer_t> anchor);

private:
    bool                                        mInitFin;           // 初始化完成
    size_t                                      mImgChns;           // 图像通道数
    size_t                                      mClsCount;          // 分类类别数量
    size_t                                      mImageWidth;        // 原始图像宽度
    size_t                                      mImageHeight;       // 原始图像高度
    size_t                                      mInputWidth;        // 模型输入宽度
    size_t                                      mInputHeight;       // 模型输出高度

    int                                         mAlgoType;          // 算法类型
    int                                         mAlgoAuthor;        // 算法作者

    std::shared_ptr<AlgoDetector>               mDetector;          // 算法检测器句柄
    std::vector<std::shared_ptr<ObjectTracker>> mClsTracker;        // 类别检测框跟踪器
};

API_END_NAMESPACE(Ai)

#endif
