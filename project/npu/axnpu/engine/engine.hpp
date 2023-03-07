#ifndef NPU_AXNPU_ENGINE_ENGINE_HPP
#define NPU_AXNPU_ENGINE_ENGINE_HPP

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstdbool>

#include <joint.h>
#include <npu_common.h>
#include <ax_sys_api.h>
#include <ax_npu_imgproc.h>
#include <npu/AiStruct.hpp>

API_BEGIN_NAMESPACE(Ai)

typedef struct {
    int32_t              modelInputWidth;       // 模型输入宽度
    int32_t              modelInputHeight;      // 模型输入高度
    int32_t              modelInputFormat;      // 模型输入颜色空间(例如，AX_FORMAT_RGB888)
    uint32_t             modelOutputCount;      // 模型输出节点个数
    AX_JOINT_IOMETA_T    *modelOutputInfo;      // 模型输出的节点信息数组指针，数组的长度对应modelOutputCount
    AX_JOINT_IO_BUFFER_T *modelOutputBuff;      // 模型输出的节点张量数组指针，数组的长度对应modelOutputCount
} AiJointAttribute;

typedef struct {
    std::string           name;
    uint32_t              index;
    std::vector<uint32_t> shape;
    int32_t               size;
    uint64_t              phyAddr;
    void                  *virAddr;
} AiRunTensor;

class API_HIDDEN AiEngine {
public:
    AiEngine();
    ~AiEngine();

    int init(std::string modelFile);
    int inference(axdl_frame_t *mediaFrame, const axdl_bbox_t *cropResizeBbox);

public:
    int getModelWidth();
    int getModelHeight();
    int getModelColorSpace();

    const int getOutputSize();
    const AiRunTensor &getOutput(int idx);
    const AiRunTensor *getOutputPtr(void);

private:
    int allocJointBuffer(const AX_JOINT_IOMETA_T *iometa, AX_JOINT_IO_BUFFER_T *iobuff);
    int prepareJointIo(AX_NPU_CV_Image *inputFrame, AX_JOINT_IO_T &io, const AX_JOINT_IO_INFO_T *ioInfo, const uint32_t batchSize = 1);
    int npuCropResize(const AX_NPU_CV_Image *inputImage, AX_NPU_CV_Image *outputImage, AX_NPU_CV_Box *box, AX_NPU_SDK_EX_MODEL_TYPE_T modelType, AX_NPU_CV_ImageResizeAlignParam horizontal, AX_NPU_CV_ImageResizeAlignParam vertical);

private:
    bool readFile(const std::string &filename, std::vector<char> &data);
    int parseNpuModeFromJoint(const char *data, const uint32_t dataSize, AX_NPU_SDK_EX_HARD_MODE_T *mode);

private:
    void convert(axdl_frame_t *src, AX_NPU_CV_Image *dst);

private:
    bool                     mInitFin;      // AI引擎是否初始化完成
    void                     *mHandler;     // AI引擎上下文控制句柄
    AiJointAttribute         mJointAttr;    // AI引擎模型信息属性信息
    std::vector<AiRunTensor> mRunTensor;    // AI引擎运行时张量信息
};

API_END_NAMESPACE(Ai)

#endif
