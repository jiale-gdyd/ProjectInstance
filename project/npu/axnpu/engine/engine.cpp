#include <cstring>
#include <cstdlib>

#include "../axnpu.h"
#include "engine.hpp"

API_BEGIN_NAMESPACE(Ai)

typedef struct {
    AX_JOINT_HANDLE                      jointHandler;
    AX_JOINT_SDK_ATTR_T                  jointAttr;
    AX_JOINT_EXECUTION_CONTEXT           jointContext;
    AX_JOINT_EXECUTION_CONTEXT_SETTING_T jointCtxSetting;
    AX_JOINT_IO_T                        jointIoAttr;
    AX_JOINT_IO_SETTING_T                jointIoSetting;
    AX_NPU_CV_Image                      modelInputNV12;
    AX_NPU_CV_Image                      modelInputRGB;
    AX_NPU_CV_Image                      modelInputBGR;
    AX_JOINT_COLOR_SPACE_T               modelColorSpace;
    int32_t                              modelInputWidth;
    int32_t                              modelInputHeight;
} HandlerPrivateInfo;

AiEngine::AiEngine() : mInitFin(false), mHandler(nullptr)
{

}

AiEngine::~AiEngine()
{
    mInitFin = false;
    HandlerPrivateInfo *handler = (HandlerPrivateInfo *)mHandler;
    if (handler) {
        auto destroyJoint = [&]() {
            if (handler->jointIoAttr.pInputs) {
                delete[] handler->jointIoAttr.pInputs;
            }

            if (handler->jointIoAttr.pOutputs) {
                for (size_t i = 0; i < handler->jointIoAttr.nOutputSize; ++i) {
                    AX_JOINT_IO_BUFFER_T *pBuffer = handler->jointIoAttr.pOutputs +i;
                    AX_JOINT_FreeBuffer(pBuffer);
                }

                delete[] handler->jointIoAttr.pOutputs;
            }

            AX_JOINT_DestroyExecutionContext(handler->jointContext);
            AX_JOINT_DestroyHandle(handler->jointHandler);
            AX_JOINT_Adv_Deinit();
        };

        destroyJoint();

        AX_SYS_MemFree((AX_U64)handler->modelInputRGB.pPhy, (AX_VOID *)handler->modelInputRGB.pVir);
        AX_SYS_MemFree((AX_U64)handler->modelInputBGR.pPhy, (AX_VOID *)handler->modelInputBGR.pVir);
        AX_SYS_MemFree((AX_U64)handler->modelInputNV12.pPhy, (AX_VOID *)handler->modelInputNV12.pVir);

        delete handler;
    }
}

int AiEngine::init(std::string modelFile)
{
    if (modelFile.empty()) {
        axnpu_error("Input model file is empty");
        return -1;
    }

    HandlerPrivateInfo *handler = new HandlerPrivateInfo;
    if (handler == nullptr) {
        axnpu_error("new internal handler failed");
        return -2;
    }

    /* 1. 创建一个运行时句柄并加载模型 */
    memset(&handler->jointAttr, 0, sizeof(handler->jointAttr));
    memset(&handler->jointHandler, 0, sizeof(handler->jointHandler));

    /* 1.1 读取模型文件到缓冲区 */
    std::vector<char> modelBuffer;
    if (!readFile(modelFile, modelBuffer)) {
        axnpu_error("Read model file:[%s] failed", modelFile.c_str());
        return -3;
    }

    /* 1.2 加载模型 */
    int ret = parseNpuModeFromJoint(modelBuffer.data(), modelBuffer.size(), &handler->jointAttr.eNpuMode);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("Load model failed, return:[%d]", ret);
        return -4;
    }

    /* 1.3 初始化模型 */
    ret = AX_JOINT_Adv_Init(&handler->jointAttr);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("Init model failed, return:[%d]", ret);
        return -5;
    }

    auto deinitJoint = [&]() {
        AX_JOINT_DestroyHandle(handler->jointHandler);
        AX_JOINT_Adv_Deinit();
        return -100;
    };

    /* 1.4 真正初始化处理 */
    ret = AX_JOINT_CreateHandle(&handler->jointHandler, modelBuffer.data(), modelBuffer.size());
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("Create joint handler failed, return:[%d]", ret);
        return deinitJoint();
    }

    /* 1.5 获取工具包版本 */
    const char *toolkitVersion = AX_JOINT_GetModelToolsVersion(handler->jointHandler);
    if (toolkitVersion) {
        axnpu_info("Joint toolkit version:[%s]", toolkitVersion);
    }

    /* 1.6 删除模型缓冲区 */
    std::vector<char>().swap(modelBuffer);

    /* 1.7 创建上下文 */
    memset(&handler->jointContext, 0, sizeof(handler->jointContext));
    memset(&handler->jointCtxSetting, 0, sizeof(handler->jointCtxSetting));
    ret = AX_JOINT_CreateExecutionContextV2(handler->jointHandler, &handler->jointContext, &handler->jointCtxSetting);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("Create joint context failed, return:[%d]", ret);
        return deinitJoint();
    }

    memset(&handler->jointIoAttr, 0, sizeof(handler->jointIoAttr));
    memset(&handler->jointIoSetting, 0, sizeof(handler->jointIoSetting));
    memset(&handler->modelInputNV12, 0, sizeof(handler->modelInputNV12));
    memset(&handler->modelInputRGB, 0, sizeof(handler->modelInputRGB));
    memset(&handler->modelInputBGR, 0, sizeof(handler->modelInputBGR));

    auto ioInfo = AX_JOINT_GetIOInfo(handler->jointHandler);
    handler->modelInputWidth = ioInfo->pInputs->pShape[2];
    handler->modelColorSpace = ioInfo->pInputs->pExtraMeta->eColorSpace;

    switch (handler->modelColorSpace) {
        case AX_JOINT_CS_NV12:
            mJointAttr.modelInputFormat = AX_YUV420_SEMIPLANAR;
            handler->modelInputHeight = ioInfo->pInputs->pShape[1] / 1.5;
            break;

        case AX_JOINT_CS_RGB:
            mJointAttr.modelInputFormat = AX_FORMAT_RGB888;
            handler->modelInputHeight = ioInfo->pInputs->pShape[1];
            break;

        case AX_JOINT_CS_BGR:
            mJointAttr.modelInputFormat = AX_FORMAT_BGR888;
            handler->modelInputHeight = ioInfo->pInputs->pShape[1];
            break;

        default:
            axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
            return deinitJoint();
    }

    handler->modelInputNV12.nWidth = handler->modelInputRGB.nWidth = handler->modelInputBGR.nWidth = handler->modelInputWidth;
    handler->modelInputNV12.nHeight = handler->modelInputRGB.nHeight = handler->modelInputBGR.nHeight = handler->modelInputHeight;
    handler->modelInputNV12.tStride.nW = handler->modelInputRGB.tStride.nW = handler->modelInputBGR.tStride.nW = handler->modelInputWidth;

    handler->modelInputRGB.eDtype = AX_NPU_CV_FDT_RGB;
    handler->modelInputBGR.eDtype = AX_NPU_CV_FDT_BGR;
    handler->modelInputNV12.eDtype = AX_NPU_CV_FDT_NV12;

    handler->modelInputRGB.nSize = handler->modelInputRGB.nWidth * handler->modelInputRGB.nHeight * 3;
    handler->modelInputBGR.nSize = handler->modelInputBGR.nWidth * handler->modelInputBGR.nHeight * 3;
    handler->modelInputNV12.nSize = handler->modelInputNV12.nWidth * handler->modelInputNV12.nHeight * 1.5;

    ret = AX_SYS_MemAlloc((AX_U64 *)&handler->modelInputNV12.pPhy, (AX_VOID **)&handler->modelInputNV12.pVir, handler->modelInputNV12.nSize, 0x100, (const AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("alloc nv12 image system memory failed, return:[%d]", ret);
        return deinitJoint();;
    }

    ret = AX_SYS_MemAlloc((AX_U64 *)&handler->modelInputRGB.pPhy, (AX_VOID **)&handler->modelInputRGB.pVir, handler->modelInputRGB.nSize, 0x100, (const AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("alloc rgb image system memory failed, return:[%d]", ret);
        return deinitJoint();;
    }

    ret = AX_SYS_MemAlloc((AX_U64 *)&handler->modelInputBGR.pPhy, (AX_VOID **)&handler->modelInputBGR.pVir, handler->modelInputBGR.nSize, 0x100, (const AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("alloc bgr image system memory failed, return:[%d]", ret);
        return deinitJoint();;
    }

    switch (handler->modelColorSpace) {
        case AX_JOINT_CS_NV12:
            ret = prepareJointIo(&handler->modelInputNV12, handler->jointIoAttr, ioInfo, 1);
            break;

        case AX_JOINT_CS_RGB:
            ret = prepareJointIo(&handler->modelInputRGB, handler->jointIoAttr, ioInfo, 1);
            break;

        case AX_JOINT_CS_BGR:
            ret = prepareJointIo(&handler->modelInputBGR, handler->jointIoAttr, ioInfo, 1);
            break;

        default:
            axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
            return deinitJoint();
    }

    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("fill input failed, return:[%d]", ret);
        AX_JOINT_DestroyExecutionContext(handler->jointContext);
        return deinitJoint();
    }

    handler->jointIoAttr.pIoSetting = &handler->jointIoSetting;
    mJointAttr.modelInputWidth = handler->modelInputWidth;
    mJointAttr.modelInputHeight = handler->modelInputHeight;
    mJointAttr.modelOutputCount = ioInfo->nOutputSize;
    mJointAttr.modelOutputInfo = ioInfo->pOutputs;
    mJointAttr.modelOutputBuff = handler->jointIoAttr.pOutputs;

    mInitFin = true;
    mHandler = handler;

    for (size_t i = 0; i < mJointAttr.modelOutputCount; ++i) {
        AiRunTensor tensor;

        tensor.index = i;
        tensor.name = std::string(mJointAttr.modelOutputInfo[i].pName);
        tensor.size = mJointAttr.modelOutputInfo[i].nSize;
        for (size_t j = 0; j < mJointAttr.modelOutputInfo[i].nShapeSize; ++j) {
            tensor.shape.push_back(mJointAttr.modelOutputInfo[i].pShape[j]);
        }
        tensor.phyAddr = mJointAttr.modelOutputBuff[i].phyAddr;
        tensor.virAddr = mJointAttr.modelOutputBuff[i].pVirAddr;

        mRunTensor.push_back(tensor);
    }

    return 0;
}

int AiEngine::inference(axdl_frame_t *mediaFrame, const axdl_bbox_t *cropResizeBbox)
{
    if (!mInitFin) {
        axnpu_error("inference engine not initialized");
        return -1;
    }

    HandlerPrivateInfo *handler = (HandlerPrivateInfo *)mHandler;
    if (handler == nullptr) {
        axnpu_error("inference engine not initialized");
        return -2;
    }

    AX_NPU_CV_Image npu_image;
    convert(mediaFrame, &npu_image);

    AX_NPU_SDK_EX_MODEL_TYPE_T modelType;
    AX_JOINT_GetVNPUMode(handler->jointHandler, &modelType);

    switch (npu_image.eDtype) {
        case AX_NPU_CV_FDT_NV12:
            npuCropResize(&npu_image, &handler->modelInputNV12, (AX_NPU_CV_Box *)cropResizeBbox, modelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
            break;

        case AX_NPU_CV_FDT_RGB:
            npuCropResize(&npu_image, &handler->modelInputRGB, (AX_NPU_CV_Box *)cropResizeBbox, modelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
            break;

        case AX_NPU_CV_FDT_BGR:
            npuCropResize(&npu_image, &handler->modelInputBGR, (AX_NPU_CV_Box *)cropResizeBbox, modelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
            break;

        default:
            axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
            return -3;
    }

    switch (handler->modelColorSpace) {
        case AX_JOINT_CS_NV12: {
            switch (npu_image.eDtype) {
                case AX_NPU_CV_FDT_NV12:
                    break;

                case AX_NPU_CV_FDT_RGB:
                    AX_NPU_CV_CSC(modelType, &handler->modelInputRGB, &handler->modelInputNV12);
                    break;

                case AX_NPU_CV_FDT_BGR:
                    AX_NPU_CV_CSC(modelType, &handler->modelInputBGR, &handler->modelInputNV12);
                    break;

                default:
                    axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
                    return -4;
            }

            break;
        }

        case AX_JOINT_CS_RGB: {
            switch (npu_image.eDtype) {
                case AX_NPU_CV_FDT_NV12:
                    AX_NPU_CV_CSC(modelType, &handler->modelInputNV12, &handler->modelInputRGB);
                    break;

                case AX_NPU_CV_FDT_RGB:
                    break;

                case AX_NPU_CV_FDT_BGR:
                    AX_NPU_CV_CSC(modelType, &handler->modelInputBGR, &handler->modelInputRGB);
                    break;

                default:
                    axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
                    return -5;
            }

            break;
        }

        case AX_JOINT_CS_BGR: {
            switch (npu_image.eDtype) {
                case AX_NPU_CV_FDT_NV12:
                    AX_NPU_CV_CSC(modelType, &handler->modelInputNV12, &handler->modelInputBGR);
                    break;

                case AX_NPU_CV_FDT_RGB:
                    AX_NPU_CV_CSC(modelType, &handler->modelInputRGB, &handler->modelInputBGR);
                    break;

                case AX_NPU_CV_FDT_BGR:
                    break;

                default:
                    axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
                    return -6;
            }

            break;
        }

        default:
            axnpu_error("Now just only support NV12/RGB/BGR input format, you can modify by yourself");
            return -7;
    }

    int ret = AX_JOINT_RunSync(handler->jointHandler, handler->jointContext, &handler->jointIoAttr);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("Joint run sync failed, return:[%d]", ret);
        return -8;
    }

    return 0;
}

int AiEngine::getModelWidth()
{
    if (!mInitFin) {
        axnpu_error("inference engine not initialized");
        return -1;
    }

    return mJointAttr.modelInputWidth;
}

int AiEngine::getModelHeight()
{
    if (!mInitFin) {
        axnpu_error("inference engine not initialized");
        return -1;
    }

    return mJointAttr.modelInputHeight;
}

int AiEngine::getModelColorSpace()
{
    if (!mInitFin) {
        axnpu_error("inference engine not initialized");
        return AXDL_COLOR_SPACE_UNK;
    }

    switch (mJointAttr.modelInputFormat) {
        case AX_FORMAT_RGB888:
            return AXDL_COLOR_SPACE_RGB;

        case AX_FORMAT_BGR888:
            return AXDL_COLOR_SPACE_BGR;

        case AX_YUV420_SEMIPLANAR:
            return AXDL_COLOR_SPACE_NV12;

        default:
            return AXDL_COLOR_SPACE_UNK;
    }
}

const int AiEngine::getOutputSize()
{
    if (!mInitFin) {
        axnpu_error("inference engine not initialized");
        return 0;
    }

    return mRunTensor.size();
}

const AiRunTensor &AiEngine::getOutput(int idx)
{
    return mRunTensor[idx];
}

const AiRunTensor *AiEngine::getOutputPtr(void)
{
    return mRunTensor.data();
}

API_END_NAMESPACE(Ai)
