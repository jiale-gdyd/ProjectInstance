#include <cstring>
#include <cstdlib>

#include "../axnpu.h"
#include "engine.hpp"

API_BEGIN_NAMESPACE(Ai)

int AiEngine::allocJointBuffer(const AX_JOINT_IOMETA_T *iometa, AX_JOINT_IO_BUFFER_T *iobuff)
{
    int ret = AX_JOINT_AllocBuffer(iometa, iobuff, AX_JOINT_ABST_CACHED);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axnpu_error("alloc joint buffer failed, return:[%d]", ret);
        return -1;
    }

    return 0;
}

int AiEngine::prepareJointIo(AX_NPU_CV_Image *inputFrame, AX_JOINT_IO_T &io, const AX_JOINT_IO_INFO_T *ioInfo, const uint32_t batchSize)
{
    memset(&io, 0, sizeof(io));
    io.nInputSize = ioInfo->nInputSize;
    if (io.nInputSize != 1) {
        axnpu_error("Only single input was supported, but got:[%d]", io.nInputSize);
        return -1;
    }

    io.pInputs = new AX_JOINT_IO_BUFFER_T[io.nInputSize];

    /* 填充输入 */
    {
        AX_JOINT_IO_BUFFER_T *pBuffer = io.pInputs;
        const AX_JOINT_IOMETA_T *pMeta = ioInfo->pInputs;

        if (pMeta->nShapeSize <= 0) {
            axnpu_error("Dimension:[%u] of shape is not supported", (uint32_t)pMeta->nShapeSize);
            return -2;
        }

        auto actualDataSize = pMeta->nSize / pMeta->pShape[0] * batchSize;
        if (inputFrame->nSize != actualDataSize) {
            axnpu_error("The buffer size is not equal to model input:[%s] size:[%u vs %u]", ioInfo->pInputs[0].pName, (uint32_t)inputFrame->nSize, actualDataSize);
            return -3;
        }

        pBuffer->nSize = (AX_U32)inputFrame->nSize;
        pBuffer->phyAddr = (AX_ADDR)inputFrame->pPhy;
        pBuffer->pVirAddr = (AX_VOID *)inputFrame->pVir;
    }

    /* 处理输出 */
    {
        io.nOutputSize = ioInfo->nOutputSize;
        io.pOutputs = new AX_JOINT_IO_BUFFER_T[io.nOutputSize];

        for (size_t i = 0; i < io.nOutputSize; ++i) {
            AX_JOINT_IO_BUFFER_T *pBuffer = io.pOutputs + i;
            const AX_JOINT_IOMETA_T *pMeta = ioInfo->pOutputs + i;

            allocJointBuffer(pMeta, pBuffer);
        }
    }

    return 0;
}

int AiEngine::npuCropResize(const AX_NPU_CV_Image *inputImage, AX_NPU_CV_Image *outputImage, AX_NPU_CV_Box *box, AX_NPU_SDK_EX_MODEL_TYPE_T modelType, AX_NPU_CV_ImageResizeAlignParam horizontal, AX_NPU_CV_ImageResizeAlignParam vertical)
{
    AX_NPU_CV_Color color;
    color.nYUVColorValue[0] = 128;
    color.nYUVColorValue[1] = 128;
    AX_NPU_SDK_EX_MODEL_TYPE_T virtual_npu_mode_type = modelType;

    if (box) {
        box->fX = std::max((int)box->fX, 0);
        box->fY = std::max((int)box->fY, 0);

        box->fW = std::min((int)box->fW, (int)inputImage->nWidth - (int)box->fX);
        box->fH = std::min((int)box->fH, (int)inputImage->nHeight - (int)box->fY);
        box->fW = int(box->fW) - int(box->fW) % 2;
        box->fH = int(box->fH) - int(box->fH) % 2;
    }

    AX_NPU_CV_Box *ppBox[1];
    ppBox[0] = box;

    int ret = AX_NPU_CV_CropResizeImage(virtual_npu_mode_type, inputImage, 1, &outputImage, ppBox, horizontal, vertical, color);
    if (ret != AX_NPU_DEV_STATUS_SUCCESS) {
        axnpu_error("npu cv crop resize image failed, return:[%d]", ret);
        return -1;
    }

    return 0;
}

bool AiEngine::readFile(const std::string &filename, std::vector<char> &data)
{
    std::fstream fs(filename, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        return false;
    }

    fs.seekg(std::ios::end);
    auto fs_end = fs.tellg();
    fs.seekg(std::ios::beg);
    auto fs_beg = fs.tellg();

    auto file_size = static_cast<size_t>(fs_end - fs_beg);
    auto vector_size = data.size();

    data.reserve(vector_size + file_size);
    data.insert(data.end(), std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());

    fs.close();
    return true;
}

int AiEngine::parseNpuModeFromJoint(const char *data, const uint32_t dataSize, AX_NPU_SDK_EX_HARD_MODE_T *mode)
{
    AX_NPU_SDK_EX_MODEL_TYPE_T npu_type;

    auto ret = AX_JOINT_GetJointModelType(data, dataSize, &npu_type);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        axnpu_error("Get joint model type failed, return:[%d]", ret);
        return -1;
    }

    if (AX_NPU_MODEL_TYPE_DEFUALT == npu_type) {
        *mode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_DISABLE;
    }
#if defined(CHIP_AX620)
    else if ((npu_type == AX_NPU_MODEL_TYPE_1_1_1) || (npu_type == AX_NPU_MODEL_TYPE_1_1_2)) {
        *pMode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_1_1;
    }
#else
    else if ((npu_type == AX_NPU_MODEL_TYPE_3_1_1) || (npu_type == AX_NPU_MODEL_TYPE_3_1_2)) {
        *mode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_3_1;
    } else if ((npu_type == AX_NPU_MODEL_TYPE_2_2_1) || (npu_type == AX_NPU_MODEL_TYPE_2_2_2)) {
        *mode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_2_2;
    }
#endif
    else {
        axnpu_error("Unknown npu mode:[%d]", (int)npu_type);
        return -2;
    }

    return 0;
}

void AiEngine::convert(axdl_frame_t *src, AX_NPU_CV_Image *dst)
{
    memset(dst, 0, sizeof(AX_NPU_CV_Image));

    dst->pPhy = src->phy;
    dst->pVir = (AX_U8 *)src->vir;
    dst->nHeight = src->height;
    dst->nWidth = src->width;
    dst->nSize = src->size;
    dst->tStride.nW = src->strideC;

    switch (src->dtype) {
        case AXDL_COLOR_SPACE_NV12:
            dst->eDtype = AX_NPU_CV_FDT_NV12;
            break;

        case AXDL_COLOR_SPACE_NV21:
            dst->eDtype = AX_NPU_CV_FDT_NV21;
            break;

        case AXDL_COLOR_SPACE_BGR:
            dst->eDtype = AX_NPU_CV_FDT_BGR;
            break;

        case AXDL_COLOR_SPACE_RGB:
            dst->eDtype = AX_NPU_CV_FDT_RGB;
            break;

        default:
            dst->eDtype = AX_NPU_CV_FDT_UNKNOWN;
            break;
    }
}

API_END_NAMESPACE(Ai)
