#ifndef MAIX_JOINT_CV_CV_HPP
#define MAIX_JOINT_CV_CV_HPP

#include <cstdio>
#include <vector>
#include <memory>

#include <joint.h>
#include <ax_sys_api.h>
#include <npu_common.h>
#include <ax_npu_imgproc.h>
#include <ax_interpreter_external_api.h>

#include "io.hpp"
#include "utils.hpp"

API_BEGIN_NAMESPACE(joint)

typedef AX_NPU_CV_Box axnpu_cvbox_t;

typedef struct {
    int h;
    int w;
    int stride_w = w;
    int color_space;
} aximg_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} axbox_t;

axnpu_cvbox_t *filter_box(const aximg_t &input, const axbox_t &box)
{
    if ((box.x < 0) || (box.y < 0) || (box.w < 0) || (box.h < 0)) {
        maxix_error("box err x or y or h or w < 0");
        return nullptr;
    }

    if (((box.w % 2) == 1) || ((box.h % 2) == 1)) {
        maxix_error("box err w or h not even");
        return nullptr;
    }

    if (((box.w + box.x) > input.w) || ((box.h + box.y) > input.h)) {
        maxix_error("boxs bottom right overflow input");
        return nullptr;
    }

    auto ax_cv_box = new axnpu_cvbox_t;
    ax_cv_box->fX = box.x;
    ax_cv_box->fY = box.y;
    ax_cv_box->fW = box.w;
    ax_cv_box->fH = box.h;

    return ax_cv_box;
}

axnpu_cvimg_t *alloc_cv_image(const aximg_t &input)
{
    int ret = 0;
    auto dst_image = new axnpu_cvimg_t;

    dst_image->nWidth = input.w;
    dst_image->nHeight = input.h;
    dst_image->tStride.nW = input.stride_w;
    dst_image->eDtype = (AX_NPU_CV_FrameDataType)input.color_space;

    ret = AX_SYS_MemAlloc((AX_U64 *)&dst_image->pPhy, (void **)&dst_image->pVir, joint::get_image_data_size(dst_image), 128, (const AX_S8 *)"NPU-CV");
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        maxix_error("error alloc image sys mem, return:[%d]", ret);
        return nullptr;
    }

    return dst_image;
}

int free_cv_image(axnpu_cvimg_t *image)
{
    int ret = AX_SYS_MemFree((AX_U64)image->pPhy, (void*)image->pVir);
    if (ret != AX_ERR_NPU_JOINT_SUCCESS) {
        maxix_error("error AX_SYS_MemFree, return:[%d]", ret);
        return -1;
    }

    return 0;
}

int npu_crop_resize(axnpu_cvimg_t *input_image, const char* input_data, axnpu_cvimg_t *output_image, axnpu_cvbox_t *box, int model_type)
{
    joint::copy_to_device(input_data, (void **)&input_image->pVir, joint::get_image_data_size(input_image));

    AX_NPU_CV_Color color;
    color.nYUVColorValue[0] = 0;
    color.nYUVColorValue[1] = 128;
    AX_NPU_SDK_EX_MODEL_TYPE_T virtual_npu_mode_type = (AX_NPU_SDK_EX_MODEL_TYPE_T)model_type;
    AX_NPU_CV_ImageResizeAlignParam horizontal = (AX_NPU_CV_ImageResizeAlignParam)0;
    AX_NPU_CV_ImageResizeAlignParam vertical = (AX_NPU_CV_ImageResizeAlignParam)0;

    int ret = AX_NPU_CV_CropResizeImage(virtual_npu_mode_type, input_image, 1, &output_image, &box, horizontal, vertical, color);
    if (ret != AX_NPU_DEV_STATUS_SUCCESS) {
        maxix_error("AX_NPU_CV_CropResizeImage failed, return:[%d]", ret);
        return -1;
    }

    return 0;
}

API_END_NAMESPACE(joint)

#endif
