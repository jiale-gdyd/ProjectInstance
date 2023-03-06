#ifndef MAIX_JOINT_CV_UTILS_HPP
#define MAIX_JOINT_CV_UTILS_HPP

#include <cstdio>
#include <string>
#include <ax_sys_api.h>
#include <npu_common.h>
#include <ax_interpreter_external_api.h>

#include "../private.hpp"

API_BEGIN_NAMESPACE(joint)

typedef AX_NPU_CV_Image axnpu_cvimg_t;
typedef AX_NPU_SDK_EX_ATTR_T axnpu_exattr_t;

static inline axnpu_exattr_t get_npu_hard_mode(const std::string &mode)
{
    axnpu_exattr_t hard_mode;
    if (mode == "1_1") {
        hard_mode.eHardMode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_1_1;
    } else {
        hard_mode.eHardMode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_DISABLE;
    }

    return hard_mode;
}

static inline int get_npu_mode_type(const std::string &mode_type)
{
    if (mode_type == "1_1_1") {
        return AX_NPU_MODEL_TYPE_1_1_1;
    } else if (mode_type == "1_1_2") {
        return AX_NPU_MODEL_TYPE_1_1_2;
    } else {
        return AX_NPU_MODEL_TYPE_DEFUALT;
    }
}

static inline int get_color_space(const std::string &color_space)
{
    if ((color_space == "NV12") || (color_space == "nv12")) {
        return AX_NPU_CV_FDT_NV12;
    } else if ((color_space == "NV21") || (color_space == "nv21")) {
        return AX_NPU_CV_FDT_NV21;
    } else if ((color_space == "RGB") || (color_space == "rgb")) {
        return AX_NPU_CV_FDT_RGB;
    } else if ((color_space == "BGR") || (color_space == "bgr")) {
        return AX_NPU_CV_FDT_BGR;
    } else if ((color_space == "RGBA") || (color_space == "rgba")) {
        return AX_NPU_CV_FDT_RGBA;
    } else if ((color_space == "GRAY") || (color_space == "gray")) {
        return AX_NPU_CV_FDT_GRAY;
    } else {
        maxix_error("color space support error");
    }

    return AX_NPU_CV_FDT_NV12;
}

static inline uint32_t get_image_stride_w(const axnpu_cvimg_t *pImg)
{
    // 内部使用接口，不再冗余校验pImg指针
    if (pImg->tStride.nW == 0) {
        return pImg->nWidth;
    } else if (pImg->tStride.nW >= pImg->nWidth) {
        if (((pImg->eDtype == AX_NPU_CV_FDT_NV12) || (pImg->eDtype == AX_NPU_CV_FDT_NV21)) && ((pImg->tStride.nW % 2) != 0)) {
            maxix_error("Invalid param: image stride_w:[%d] should be even for NV12/NV21", pImg->tStride.nW);
        }

        return pImg->tStride.nW;
    } else {
        maxix_error("Invalid param: image stride_w:[%d] not less than image width:[%d]", pImg->tStride.nW, pImg->nWidth);
    }

    return pImg->nWidth;
}

static inline int get_image_data_size(const axnpu_cvimg_t *img)
{
    int stride_w = get_image_stride_w(img);
    switch (img->eDtype) {
        case AX_NPU_CV_FDT_NV12:
        case AX_NPU_CV_FDT_NV21:
            return int(stride_w * img->nHeight * 3 / 2);

        case AX_NPU_CV_FDT_RGB:
        case AX_NPU_CV_FDT_BGR:
        case AX_NPU_CV_FDT_YUV444:
            return int(stride_w * img->nHeight * 3);

        case AX_NPU_CV_FDT_RGBA:
            return int(stride_w * img->nHeight * 4);

        case AX_NPU_CV_FDT_GRAY:
            return int(stride_w * img->nHeight * 1);

        default:
            maxix_error("Unsupported color space:[%d] to calculate image data size", (int)img->eDtype);
            return 0;
    }
}

API_END_NAMESPACE(joint)

#endif
