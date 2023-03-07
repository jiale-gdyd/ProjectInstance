#ifndef AXNPU_ALGORITHM_DETECTOR_ALGODETECTOR_HPP
#define AXNPU_ALGORITHM_DETECTOR_ALGODETECTOR_HPP

#include <string>
#include <memory>
#include <sys/types.h>
#include <utils/export.h>
#include <npu/axnpu/axnpu.h>
#include <npu/axnpu/engine/engine.hpp>

API_BEGIN_NAMESPACE(Ai)

class API_HIDDEN AlgoDetector {
public:
    AlgoDetector(std::string modelFile, size_t imageWidth, size_t imageHeight, size_t classCount, float confThreshold, float nmsThreshold, int algoType, int algoAuthor);
    ~AlgoDetector();

    virtual int init();

    int forward(axdl_frame_t *mediaFrame, axdl_bbox_t *cropResizeBbox, axdl_result_t *lastResult);

public:
    int getModelInputWidthHeight(int &width, int &height, int &colorSpace) const;

private:
    int extract(axdl_result_t *lastResult);

private:
    int axera_extract(axdl_result_t *lastResult);
    int axera_extract_yolov5(axdl_result_t *lastResult);

private:
    int                mAnchorSizePerStride = 6;
    std::vector<int >  mStrides = { 8, 16, 32 };
    std::vector<float> mAnchors = { 12, 16, 19, 36, 40, 28, 36, 75, 76, 55, 72, 146, 142, 110, 192, 243, 459, 401 };

private:
    bool                          mInitFin;               // 初始化完成
    std::string                   mModelFile;             // 算法模型文件
    size_t                        mImageWidth;            // 输入图像宽度(原图)
    size_t                        mImageHeight;           // 输入图像高度(原图)
    size_t                        mClassCount;            // 分类检测的分类数(例如80个类)
    float                         mConfThreshold;         // 置信度过滤阈值
    float                         mNmsThreshold;          // 非极大抑制过滤阈值
    int                           mAlgoType;              // 算法类型(例如Yolov5, Yolov3等)
    int                           mAlgoAuthor;            // 算法提供作者(例如rockchip，不同作者可能在处理上有区别)

    std::shared_ptr<Ai::AiEngine> pAiEngine;
};

API_END_NAMESPACE(Ai)

#endif
