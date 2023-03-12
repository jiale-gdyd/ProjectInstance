#ifndef CHIP_AX620
#define CHIP_AX620
#endif

#include <vector>

#include <joint.h>
#include <joint_adv.h>
#include <ax_sys_api.h>
#include <ax_npu_imgproc.h>
#include <ax_interpreter_external_api.h>

#include "axjoint.hpp"
#include "../utilities/io.hpp"
#include "../utilities/log.hpp"
#include "../utilities/file.hpp"

namespace axpi {
#ifndef MIN
#define MIN(a, b)       ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b)       ((a) < (b) ? (b) : (a))
#endif

typedef struct {
    AX_JOINT_HANDLE                      joint_handle;
    AX_JOINT_SDK_ATTR_T                  joint_attr;
    AX_JOINT_EXECUTION_CONTEXT           joint_ctx;
    AX_JOINT_EXECUTION_CONTEXT_SETTING_T joint_ctx_settings;
    AX_JOINT_IO_T                        joint_io_arr;
    AX_JOINT_IO_SETTING_T                joint_io_setting;
    AX_NPU_CV_Image                      algo_input_nv12;
    AX_NPU_CV_Image                      algo_input_rgb;
    AX_NPU_CV_Image                      algo_input_bgr;
    AX_JOINT_COLOR_SPACE_T               algo_format;
    int                                  algo_width = 0;
    int                                  algo_height = 0;
} handle_t;

static int prepare_io(AX_NPU_CV_Image *algo_input, AX_JOINT_IO_T &io, const AX_JOINT_IO_INFO_T *io_info, const uint32_t &batch)
{
    memset(&io, 0, sizeof(io));

    io.nInputSize = io_info->nInputSize;
    if (1 != io.nInputSize) {
        axmpi_error("Only single input was accepted, but got:[%u]", io.nInputSize);
        return -1;
    }
    io.pInputs = new AX_JOINT_IO_BUFFER_T[io.nInputSize];

    {
        AX_JOINT_IO_BUFFER_T *pBuf = io.pInputs;
        const AX_JOINT_IOMETA_T *pMeta = io_info->pInputs;

        if (pMeta->nShapeSize <= 0) {
            axmpi_error("Dimension:[%u] of shape is not allowed", (uint32_t)pMeta->nShapeSize);
            return -1;
        }

        auto actual_data_size = pMeta->nSize / pMeta->pShape[0] * batch;
        if (algo_input->nSize != actual_data_size) {
            axmpi_error("The buffer size is not equal to model input:[%s] size:[%u vs %u]", io_info->pInputs[0].pName, (uint32_t)algo_input->nSize, actual_data_size);
            return -1;
        }

        pBuf->phyAddr = (AX_ADDR)algo_input->pPhy;
        pBuf->pVirAddr = (AX_VOID *)algo_input->pVir;
        pBuf->nSize = (AX_U32)algo_input->nSize;
    }

    {
        io.nOutputSize = io_info->nOutputSize;
        io.pOutputs = new AX_JOINT_IO_BUFFER_T[io.nOutputSize];

        for (size_t i = 0; i < io.nOutputSize; ++i) {
            AX_JOINT_IO_BUFFER_T *pBuf = io.pOutputs + i;
            const AX_JOINT_IOMETA_T *pMeta = io_info->pOutputs + i;

            axio::alloc_joint_buffer(pMeta, pBuf);
        }
    }

    return AX_ERR_NPU_JOINT_SUCCESS;
}

static int npu_crop_resize(const AX_NPU_CV_Image *input_image, AX_NPU_CV_Image *output_image, AX_NPU_CV_Box *box, AX_NPU_SDK_EX_MODEL_TYPE_T model_type, AX_NPU_CV_ImageResizeAlignParam horizontal, AX_NPU_CV_ImageResizeAlignParam vertical)
{
    AX_NPU_CV_Color color;
    color.nYUVColorValue[0] = 128;
    color.nYUVColorValue[1] = 128;
    AX_NPU_SDK_EX_MODEL_TYPE_T virtual_npu_mode_type = model_type;

    if (box) {
        box->fX = MAX((int)box->fX, 0);
        box->fY = MAX((int)box->fY, 0);

        box->fW = MIN((int)box->fW, (int)input_image->nWidth - (int)box->fX);
        box->fH = MIN((int)box->fH, (int)input_image->nHeight - (int)box->fY);
        box->fW = int(box->fW) - int(box->fW) % 2;
        box->fH = int(box->fH) - int(box->fH) % 2;
    }

    AX_NPU_CV_Box *ppBox[1];
    ppBox[0] = box;

    int ret = AX_NPU_CV_CropResizeImage(virtual_npu_mode_type, input_image, 1, &output_image, ppBox, horizontal, vertical, color);
    if (ret != AX_NPU_DEV_STATUS_SUCCESS)  {
        axmpi_error("AX_NPU_CV_CropResizeImage failed, return:[%d]", ret);
        return ret;
    }

    return 0;
}

int axjoint_init(std::string modelFile, void **handler, axjoint_attr_t *attr)
{
    if (modelFile.empty()) {
        axmpi_error("invalid param:model_file is null");
        return -1;
    }

    if (!attr) {
        axmpi_error("invalid param:attr is null");
        return -1;
    }

    handle_t *handle = new handle_t;

    memset(&handle->joint_handle, 0, sizeof(handle->joint_handle));
    memset(&handle->joint_attr, 0, sizeof(handle->joint_attr));

    std::vector<char> model_buffer;
    if (!read_file(modelFile.c_str(), model_buffer)) {
        axmpi_error("Read Run-Joint model:[%s] file failed", modelFile.c_str());
        return -1;
    }

    auto ret = axio::parse_npu_mode_from_joint(model_buffer.data(), model_buffer.size(), &handle->joint_attr.eNpuMode);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        axmpi_error("Load Run-Joint model:[%s] failed", modelFile.c_str());
        return -1;
    }

    ret = AX_JOINT_Adv_Init(&handle->joint_attr);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        axmpi_error("Init Run-Joint model:[%s] failed.\n", modelFile.c_str());
        return -1;
    }

    auto deinit_joint = [&]() {
        AX_JOINT_DestroyHandle(handle->joint_handle);
        AX_JOINT_Adv_Deinit();
        return -1;
    };

    ret = AX_JOINT_CreateHandle(&handle->joint_handle, model_buffer.data(), model_buffer.size());
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        axmpi_error("Create Run-Joint handler from file:[%s] failed", modelFile.c_str());
        return deinit_joint();
    }

    const AX_CHAR *version = AX_JOINT_GetModelToolsVersion(handle->joint_handle);
    axmpi_info("Toolkit version:[%s]", version);

    std::vector<char>().swap(model_buffer);

    memset(&handle->joint_ctx, 0, sizeof(handle->joint_ctx));
    memset(&handle->joint_ctx_settings, 0, sizeof(handle->joint_ctx_settings));
    ret = AX_JOINT_CreateExecutionContextV2(handle->joint_handle, &handle->joint_ctx, &handle->joint_ctx_settings);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        axmpi_error("Create Run-Joint context failed");
        return deinit_joint();
    }

    memset(&handle->joint_io_arr, 0, sizeof(handle->joint_io_arr));
    memset(&handle->joint_io_setting, 0, sizeof(handle->joint_io_setting));
    memset(&handle->algo_input_nv12, 0, sizeof(handle->algo_input_nv12));
    memset(&handle->algo_input_rgb, 0, sizeof(handle->algo_input_rgb));
    memset(&handle->algo_input_bgr, 0, sizeof(handle->algo_input_bgr));

    auto io_info = AX_JOINT_GetIOInfo(handle->joint_handle);
    handle->algo_width = io_info->pInputs->pShape[2];
    handle->algo_format = io_info->pInputs->pExtraMeta->eColorSpace;

    switch (handle->algo_format) {
        case AX_JOINT_CS_NV12:
            attr->format = (int)AX_YUV420_SEMIPLANAR;
            handle->algo_height = io_info->pInputs->pShape[1] / 1.5;
            break;

        case AX_JOINT_CS_RGB:
            attr->format = (int)AX_FORMAT_RGB888;
            handle->algo_height = io_info->pInputs->pShape[1];
            break;

        case AX_JOINT_CS_BGR:
            attr->format = (int)AX_FORMAT_BGR888;
            handle->algo_height = io_info->pInputs->pShape[1];
            break;

        default:
            axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format, you can modify by yourself");
            return deinit_joint();
    }

    handle->algo_input_nv12.nWidth = handle->algo_input_rgb.nWidth = handle->algo_input_bgr.nWidth = handle->algo_width;
    handle->algo_input_nv12.nHeight = handle->algo_input_rgb.nHeight = handle->algo_input_bgr.nHeight = handle->algo_height;
    handle->algo_input_nv12.tStride.nW = handle->algo_input_rgb.tStride.nW = handle->algo_input_bgr.tStride.nW = handle->algo_width;

    handle->algo_input_nv12.eDtype = AX_NPU_CV_FDT_NV12;
    handle->algo_input_rgb.eDtype = AX_NPU_CV_FDT_RGB;
    handle->algo_input_bgr.eDtype = AX_NPU_CV_FDT_BGR;

    handle->algo_input_nv12.nSize = handle->algo_input_nv12.nWidth * handle->algo_input_nv12.nHeight * 1.5;
    handle->algo_input_rgb.nSize = handle->algo_input_rgb.nWidth * handle->algo_input_rgb.nHeight * 3;
    handle->algo_input_bgr.nSize = handle->algo_input_bgr.nWidth * handle->algo_input_bgr.nHeight * 3;

    ret = AX_SYS_MemAlloc((AX_U64 *)&handle->algo_input_nv12.pPhy, (void **)&handle->algo_input_nv12.pVir, handle->algo_input_nv12.nSize, 0x100, (const AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axmpi_error("alloc nv12 image sys mem failed, return:[%d]", ret);
        return deinit_joint();
    }

    ret = AX_SYS_MemAlloc((AX_U64 *)&handle->algo_input_rgb.pPhy, (void **)&handle->algo_input_rgb.pVir, handle->algo_input_rgb.nSize, 0x100, (const AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axmpi_error("alloc rgb image sys mem failed, return:[%d]", ret);
        return deinit_joint();
    }

    ret = AX_SYS_MemAlloc((AX_U64 *)&handle->algo_input_bgr.pPhy, (void **)&handle->algo_input_bgr.pVir, handle->algo_input_bgr.nSize, 0x100, (const AX_S8 *)"SAMPLE-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        axmpi_error("alloc bgr image sys mem failed, return:[%d]", ret);
        return deinit_joint();
    }

    switch (handle->algo_format) {
        case AX_JOINT_CS_NV12:
            ret = prepare_io(&handle->algo_input_nv12, handle->joint_io_arr, io_info, 1);
            break;

        case AX_JOINT_CS_RGB:
            ret = prepare_io(&handle->algo_input_rgb, handle->joint_io_arr, io_info, 1);
            break;

        case AX_JOINT_CS_BGR:
            ret = prepare_io(&handle->algo_input_bgr, handle->joint_io_arr, io_info, 1);
            break;

        default:
            axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format, you can modify by yourself");
            return deinit_joint();
    }

    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        axmpi_error("Fill input failed");
        AX_JOINT_DestroyExecutionContext(handle->joint_ctx);
        return deinit_joint();
    }

    handle->joint_io_arr.pIoSetting = &handle->joint_io_setting;

    attr->width = handle->algo_width;
    attr->height = handle->algo_height;
    attr->outSize = io_info->nOutputSize;
    attr->outInfo = io_info->pOutputs;
    attr->outputs = handle->joint_io_arr.pOutputs;

    *handler = handle;
    return 0;
}

int axjoint_exit(void *handler)
{
    handle_t *handle = (handle_t *)handler;
    if (handle) {
        auto DestroyJoint = [&]() {
            if (handle->joint_io_arr.pInputs) {
                delete[] handle->joint_io_arr.pInputs;
            }

            if (handle->joint_io_arr.pOutputs) {
                for (size_t i = 0; i < handle->joint_io_arr.nOutputSize; ++i) {
                    AX_JOINT_IO_BUFFER_T *pBuf = handle->joint_io_arr.pOutputs + i;
                    AX_JOINT_FreeBuffer(pBuf);
                }

                delete[] handle->joint_io_arr.pOutputs;
            }

            AX_JOINT_DestroyExecutionContext(handle->joint_ctx);
            AX_JOINT_DestroyHandle(handle->joint_handle);
            AX_JOINT_Adv_Deinit();
        };

        DestroyJoint();

        AX_SYS_MemFree((AX_U64)handle->algo_input_nv12.pPhy, (void *)handle->algo_input_nv12.pVir);
        AX_SYS_MemFree((AX_U64)handle->algo_input_rgb.pPhy, (void *)handle->algo_input_rgb.pVir);
        AX_SYS_MemFree((AX_U64)handle->algo_input_bgr.pPhy, (void *)handle->algo_input_bgr.pVir);

        delete handle;
    }

    return 0;
}

int axjoint_inference(void *handler, const void *frame, const void *crop_resize_box)
{
    handle_t *handle = (handle_t *)handler;
    if (!handle) {
        axmpi_error("invalid param:yhandle is null");
        return -1;
    }

    AX_NPU_CV_Image *pstFrame = (AX_NPU_CV_Image *)frame;

    AX_NPU_SDK_EX_MODEL_TYPE_T ModelType;
    AX_JOINT_GetVNPUMode(handle->joint_handle, &ModelType);

    switch (pstFrame->eDtype) {
        case AX_NPU_CV_FDT_NV12:
            npu_crop_resize(pstFrame, &handle->algo_input_nv12, (AX_NPU_CV_Box *)crop_resize_box, ModelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
            break;

        case AX_NPU_CV_FDT_RGB:
            npu_crop_resize(pstFrame, &handle->algo_input_rgb, (AX_NPU_CV_Box *)crop_resize_box, ModelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
            break;

        case AX_NPU_CV_FDT_BGR:
            npu_crop_resize(pstFrame, &handle->algo_input_bgr, (AX_NPU_CV_Box *)crop_resize_box, ModelType, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
            break;

        default:
            axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format, you can modify by yourself");
            return -1;
    }

    switch (handle->algo_format) {
        case AX_JOINT_CS_NV12: {
            switch (pstFrame->eDtype) {
                case AX_NPU_CV_FDT_NV12:
                    break;

                case AX_NPU_CV_FDT_RGB:
                    AX_NPU_CV_CSC(ModelType, &handle->algo_input_rgb, &handle->algo_input_nv12);
                    break;

                case AX_NPU_CV_FDT_BGR:
                    AX_NPU_CV_CSC(ModelType, &handle->algo_input_bgr, &handle->algo_input_nv12);
                    break;

                default:
                    axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format, you can modify by yourself");
                    return -1;
            }
        }
        break;

        case AX_JOINT_CS_RGB: {
            switch (pstFrame->eDtype) {
                case AX_NPU_CV_FDT_NV12:
                    AX_NPU_CV_CSC(ModelType, &handle->algo_input_nv12, &handle->algo_input_rgb);
                    break;

                case AX_NPU_CV_FDT_RGB:
                    break;

                case AX_NPU_CV_FDT_BGR:
                    AX_NPU_CV_CSC(ModelType, &handle->algo_input_bgr, &handle->algo_input_rgb);
                    break;

                default:
                    axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format,you can modify by yourself");
                    return -1;
            }
        }
        break;

        case AX_JOINT_CS_BGR: {
            switch (pstFrame->eDtype) {
                case AX_NPU_CV_FDT_NV12:
                    AX_NPU_CV_CSC(ModelType, &handle->algo_input_nv12, &handle->algo_input_bgr);
                    break;

                case AX_NPU_CV_FDT_RGB:
                    AX_NPU_CV_CSC(ModelType, &handle->algo_input_rgb, &handle->algo_input_bgr);
                    break;

                case AX_NPU_CV_FDT_BGR:
                    break;

                default:
                    axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format, you can modify by yourself");
                    return -1;
            }
        }
        break;

        default:
            axmpi_warn("now ax-pipeline just only support NV12/RGB/BGR input format, you can modify by yourself");
            return -1;
    }

    auto ret = AX_JOINT_RunSync(handle->joint_handle, handle->joint_ctx, &handle->joint_io_arr);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        return -1;
    }

    return 0;
}
}
