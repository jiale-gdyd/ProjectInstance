#include <string.h>

#include "axpi_common_api.hpp"
#include "../utilities/log.hpp"
#include "../utilities/matPixelAffine.hpp"

namespace axpi {
#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b)   ((a) < (b) ? (b) : (a))
#endif

int axpi_memalloc(unsigned long long int *phyaddr, void **pviraddr, unsigned int size, unsigned int align, const char *token)
{
    return AX_SYS_MemAlloc(phyaddr, pviraddr, size, align, (const AX_S8 *)token);
}

int axpi_memfree(unsigned long long int phyaddr, void *pviraddr)
{
    return AX_SYS_MemFree(phyaddr, pviraddr);
}

void convert(axpi_image_t *src, AX_NPU_CV_Image *dst)
{
    memset(dst, 0, sizeof(AX_NPU_CV_Image));
    dst->pPhy = src->phy;
    dst->pVir = (unsigned char *)src->vir;
    dst->nHeight = src->height;
    dst->nWidth = src->width;
    dst->nSize = src->size;
    dst->tStride.nW = src->strideC;

    switch (src->dtype) {
        case AXPI_COLOR_SPACE_NV12:
            dst->eDtype = AX_NPU_CV_FDT_NV12;
            break;

        case AXPI_COLOR_SPACE_NV21:
            dst->eDtype = AX_NPU_CV_FDT_NV21;
            break;

        case AXPI_COLOR_SPACE_BGR:
            dst->eDtype = AX_NPU_CV_FDT_BGR;
            break;

        case AXPI_COLOR_SPACE_RGB:
            dst->eDtype = AX_NPU_CV_FDT_RGB;
            break;

        default:
            dst->eDtype = AX_NPU_CV_FDT_UNKNOWN;
            break;
    }
}

int axpi_imgproc_csc(axpi_image_t *src, axpi_image_t *dst)
{
    AX_NPU_CV_Image npu_src, npu_dst;
    convert(src, &npu_src);
    convert(dst, &npu_dst);

    return AX_NPU_CV_CSC(AX_NPU_MODEL_TYPE_1_1_1, &npu_src, &npu_dst);
}

int axpi_imgproc_warp(axpi_image_t *src, axpi_image_t *dst, const float *pMat33, const int const_val)
{
    AX_NPU_CV_Image npu_src, npu_dst;
    convert(src, &npu_src);
    convert(dst, &npu_dst);

    return AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, &npu_src, &npu_dst, pMat33, AX_NPU_CV_BILINEAR, const_val);
}

static int _axpi_imgproc_crop_resize(axpi_image_t *src, axpi_image_t *dst, axpi_bbox_t *box, AX_NPU_CV_ImageResizeAlignParam horizontal, AX_NPU_CV_ImageResizeAlignParam vertical)
{
    AX_NPU_CV_Image npu_src, npu_dst;
    convert(src, &npu_src);
    convert(dst, &npu_dst);

    AX_NPU_CV_Color color;
    color.nYUVColorValue[0] = 128;
    color.nYUVColorValue[1] = 128;
    AX_NPU_SDK_EX_MODEL_TYPE_T virtual_npu_mode_type = AX_NPU_MODEL_TYPE_1_1_1;

    if (box) {
        box->x = MAX((int)box->x, 0);
        box->y = MAX((int)box->y, 0);

        box->w = MIN((int)box->w, (int)src->width - (int)box->x);
        box->h = MIN((int)box->h, (int)src->height - (int)box->y);

        box->w = int(box->w) - int(box->w) % 2;
        box->h = int(box->h) - int(box->h) % 2;
    }

    AX_NPU_CV_Box *ppBox[1];
    ppBox[0] = (AX_NPU_CV_Box *)box;

    AX_NPU_CV_Image *p_npu_dst = &npu_dst;

    return AX_NPU_CV_CropResizeImage(virtual_npu_mode_type, &npu_src, 1, &p_npu_dst, ppBox, horizontal, vertical, color);
}

int axpi_imgproc_crop_resize(axpi_image_t *src, axpi_image_t *dst, axpi_bbox_t *box)
{
    return _axpi_imgproc_crop_resize(src, dst, box, AX_NPU_CV_IMAGE_FORCE_RESIZE, AX_NPU_CV_IMAGE_FORCE_RESIZE);
}

int ax_imgproc_crop_resize_keep_ratio(axpi_image_t *src, axpi_image_t *dst, axpi_bbox_t *box)
{
    return _axpi_imgproc_crop_resize(src, dst, box, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
}

int axpi_imgproc_align_face(axpi_object_t *obj, axpi_image_t *src, axpi_image_t *dst)
{
    static float target[10] = {
        38.2946, 51.6963,
        73.5318, 51.5014,
        56.0252, 71.7366,
        41.5493, 92.3655,
        70.7299, 92.2041
    };

    float _tmp[10] = {
        obj->landmark[0].x, obj->landmark[0].y,
        obj->landmark[1].x, obj->landmark[1].y,
        obj->landmark[2].x, obj->landmark[2].y,
        obj->landmark[3].x, obj->landmark[3].y,
        obj->landmark[4].x, obj->landmark[4].y
    };

    float _m[6], _m_inv[6];
    get_affine_transform(_tmp, target, 5, _m);
    invert_affine_transform(_m, _m_inv);

    float mat3x3[3][3] = {
        {_m_inv[0], _m_inv[1], _m_inv[2]},
        {_m_inv[3], _m_inv[4], _m_inv[5]},
        {0, 0, 1}
    };

    dst->dtype = src->dtype;
    if ((dst->dtype == AXPI_COLOR_SPACE_RGB) || (dst->dtype == AXPI_COLOR_SPACE_BGR)) {
        dst->size = 112 * 112 * 3;
    } else if (dst->dtype == (AXPI_COLOR_SPACE_NV12) || (dst->dtype == AXPI_COLOR_SPACE_NV21)) {
        dst->size = 112 * 112 * 1.5;
    } else {
        axmpi_error("just only support BGR/RGB/NV12 format");
    }

    return axpi_imgproc_warp(src, dst, &mat3x3[0][0], 128);
}
}
