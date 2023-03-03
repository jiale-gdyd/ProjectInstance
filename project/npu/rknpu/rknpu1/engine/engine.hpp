#ifndef AARCH32_ROCKCHIP_RKNPU1_ENGINE_HPP
#define AARCH32_ROCKCHIP_RKNPU1_ENGINE_HPP

#include <vector>
#include <string>
#include <algorithm>

#include <stdbool.h>
#include <utils/export.h>
#include <npu/rknpu/rknpu.h>
#include <opencv2/opencv.hpp>
#include <rknn/rknn_runtime.h>

#include "../private.hpp"

API_BEGIN_NAMESPACE(Ai)

class API_HIDDEN AiEngine {
public:
    AiEngine();
    ~AiEngine();

    int init(std::string modelFile);

    int forward(cv::Mat image);
    int forward(std::vector<cv::Mat> imageSet);

    int forward(unsigned char *data, size_t dataSize);
    int forward(std::vector<unsigned char *> dataSet, std::vector<size_t> dataSizeSet);

    int forward(std::vector<rknn_tensor_mem> &inputs_mem, size_t index, bool &bLockInMem);

    int extract(extract_func_ptr extract_cb, nms_func_ptr nms_cb, void *args, std::map<int, std::vector<bbox>> &lastResult);

public:
    int setOutputMemory(std::vector<rknn_tensor_mem> &outputs_mem);

public:
    uint32_t getInputCount();
    uint32_t getOutputCount();
    std::vector<rknn_tensor_attr> getInputAttr();
    std::vector<rknn_tensor_attr> getOutputAttr();

public:
    static void nms(std::vector<bbox> &inputBbox, float threshold);

private:
    bool                          mInitFin;         // 是否已经初始化
    rknn_context                  mContext;         // 推理引擎上下文句柄
    rknn_input_output_num         mIOCount;         // 推理模型输入和输出数
    std::vector<rknn_tensor_attr> mInputAttr;       // 输入张量属性信息
    std::vector<rknn_tensor_attr> mOutputAttr;      // 输出张量属性信息
    rknn_output                   mOutputs[3];      // 输出张量数据
};

API_END_NAMESPACE(Ai)

#endif
