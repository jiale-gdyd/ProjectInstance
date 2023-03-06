#include <cmath>
#include <algorithm>

#include <string.h>
#include <ax_sys_api.h>
#include <npu_common.h>
#include <ax_npu_imgproc.h>

#include "axCommApi.hpp"

API_BEGIN_NAMESPACE(Ai)

int axsys_memfree(uint64_t phyaddr, void *pviraddr)
{
    return AX_SYS_MemFree(phyaddr, pviraddr);
}

int axsys_memalloc(uint64_t *phyaddr, void **pviraddr, uint32_t size, uint32_t align, const char *token)
{
    return AX_SYS_MemAlloc((AX_U64 *)phyaddr, pviraddr, size, align, (const AX_S8 *)token);
}

void cvt(axframe_t *src, void *dst)
{
    AX_NPU_CV_Image *pDst = (AX_NPU_CV_Image *)dst;

    memset(pDst, 0, sizeof(AX_NPU_CV_Image));
    pDst->pPhy = src->phyaddr;
    pDst->pVir = (unsigned char *)src->viraddr;
    pDst->nHeight = src->height;
    pDst->nWidth = src->width;
    pDst->nSize = src->size;
    pDst->tStride.nW = src->strideC;

    switch (src->dtype) {
        case COLOR_SPACE_NV12:
            pDst->eDtype = AX_NPU_CV_FDT_NV12;
            break;

        case COLOR_SPACE_NV21:
            pDst->eDtype = AX_NPU_CV_FDT_NV21;
            break;

        case COLOR_SPACE_BGR:
            pDst->eDtype = AX_NPU_CV_FDT_BGR;
            break;

        case COLOR_SPACE_RGB:
            pDst->eDtype = AX_NPU_CV_FDT_RGB;
            break;

        default:
            pDst->eDtype = AX_NPU_CV_FDT_UNKNOWN;
            break;
    }
}

int aximg_csc(axframe_t *src, axframe_t *dst)
{
    AX_NPU_CV_Image npu_src, npu_dst;

    cvt(src, &npu_src);
    cvt(dst, &npu_dst);
    return AX_NPU_CV_CSC(AX_NPU_MODEL_TYPE_1_1_1, &npu_src, &npu_dst);
}

int aximg_warp(axframe_t *src, axframe_t *dst, const float *pMat33, const int const_val)
{
    AX_NPU_CV_Image npu_src, npu_dst;
    cvt(src, &npu_src);
    cvt(dst, &npu_dst);
    return AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, &npu_src, &npu_dst, pMat33, AX_NPU_CV_BILINEAR, const_val);
}

static int ax_imgproc_crop_resize(axframe_t *src, axframe_t *dst, axbbox_t *box, AX_NPU_CV_ImageResizeAlignParam horizontal, AX_NPU_CV_ImageResizeAlignParam vertical)
{
    AX_NPU_CV_Image npu_src, npu_dst;
    cvt(src, &npu_src);
    cvt(dst, &npu_dst);

    AX_NPU_CV_Color color;
    color.nYUVColorValue[0] = 128;
    color.nYUVColorValue[1] = 128;
    AX_NPU_SDK_EX_MODEL_TYPE_T virtual_npu_mode_type = AX_NPU_MODEL_TYPE_1_1_1;

    if (box) {
        box->x = std::max((int)box->x, 0);
        box->y = std::max((int)box->y, 0);
        box->w = std::min((int)box->w, (int)src->width - (int)box->x);
        box->h = std::min((int)box->h, (int)src->height - (int)box->y);
        box->w = int(box->w) - int(box->w) % 2;
        box->h = int(box->h) - int(box->h) % 2;
    }

    AX_NPU_CV_Box *ppBox[1];
    ppBox[0] = (AX_NPU_CV_Box *)box;

    AX_NPU_CV_Image *p_npu_dst = &npu_dst;

    return AX_NPU_CV_CropResizeImage(virtual_npu_mode_type, &npu_src, 1, &p_npu_dst, ppBox, horizontal, vertical, color);
}

int aximg_crop_resize(axframe_t *src, axframe_t *dst, axbbox_t *box)
{
    return ax_imgproc_crop_resize(src, dst, box, AX_NPU_CV_IMAGE_FORCE_RESIZE, AX_NPU_CV_IMAGE_FORCE_RESIZE);
}

int aximg_crop_resize_keep_ratio(axframe_t *src, axframe_t *dst, axbbox_t *box)
{
    return ax_imgproc_crop_resize(src, dst, box, AX_NPU_CV_IMAGE_HORIZONTAL_CENTER, AX_NPU_CV_IMAGE_VERTICAL_CENTER);
}

static void get_affine_transform(const float *points_from, const float *points_to, int num_point, float *tm)
{
    float mm[4];
    float mb[4] = {0.f};
    float ma[4][4] = {{0.f}};

    for (int i = 0; i < num_point; i++) {
        ma[0][0] += points_from[0] * points_from[0] + points_from[1] * points_from[1];
        ma[0][2] += points_from[0];
        ma[0][3] += points_from[1];

        mb[0] += points_from[0] * points_to[0] + points_from[1] * points_to[1];
        mb[1] += points_from[0] * points_to[1] - points_from[1] * points_to[0];
        mb[2] += points_to[0];
        mb[3] += points_to[1];

        points_from += 2;
        points_to += 2;
    }

    ma[1][1] = ma[0][0];
    ma[2][1] = ma[1][2] = -ma[0][3];
    ma[3][1] = ma[1][3] = ma[2][0] = ma[0][2];
    ma[2][2] = ma[3][3] = (float)num_point;
    ma[3][0] = ma[0][3];

    float det;
    float mai[4][4];

    {
        float A2323 = ma[2][2] * ma[3][3] - ma[2][3] * ma[3][2];
        float A1323 = ma[2][1] * ma[3][3] - ma[2][3] * ma[3][1];
        float A1223 = ma[2][1] * ma[3][2] - ma[2][2] * ma[3][1];
        float A0323 = ma[2][0] * ma[3][3] - ma[2][3] * ma[3][0];
        float A0223 = ma[2][0] * ma[3][2] - ma[2][2] * ma[3][0];
        float A0123 = ma[2][0] * ma[3][1] - ma[2][1] * ma[3][0];
        float A2313 = ma[1][2] * ma[3][3] - ma[1][3] * ma[3][2];
        float A1313 = ma[1][1] * ma[3][3] - ma[1][3] * ma[3][1];
        float A1213 = ma[1][1] * ma[3][2] - ma[1][2] * ma[3][1];
        float A2312 = ma[1][2] * ma[2][3] - ma[1][3] * ma[2][2];
        float A1312 = ma[1][1] * ma[2][3] - ma[1][3] * ma[2][1];
        float A1212 = ma[1][1] * ma[2][2] - ma[1][2] * ma[2][1];
        float A0313 = ma[1][0] * ma[3][3] - ma[1][3] * ma[3][0];
        float A0213 = ma[1][0] * ma[3][2] - ma[1][2] * ma[3][0];
        float A0312 = ma[1][0] * ma[2][3] - ma[1][3] * ma[2][0];
        float A0212 = ma[1][0] * ma[2][2] - ma[1][2] * ma[2][0];
        float A0113 = ma[1][0] * ma[3][1] - ma[1][1] * ma[3][0];
        float A0112 = ma[1][0] * ma[2][1] - ma[1][1] * ma[2][0];

        det = ma[0][0] * (ma[1][1] * A2323 - ma[1][2] * A1323 + ma[1][3] * A1223)
            - ma[0][1] * (ma[1][0] * A2323 - ma[1][2] * A0323 + ma[1][3] * A0223)
            + ma[0][2] * (ma[1][0] * A1323 - ma[1][1] * A0323 + ma[1][3] * A0123)
            - ma[0][3] * (ma[1][0] * A1223 - ma[1][1] * A0223 + ma[1][2] * A0123);

        det = 1.f / det;

        mai[0][0] =   (ma[1][1] * A2323 - ma[1][2] * A1323 + ma[1][3] * A1223);
        mai[0][1] = - (ma[0][1] * A2323 - ma[0][2] * A1323 + ma[0][3] * A1223);
        mai[0][2] =   (ma[0][1] * A2313 - ma[0][2] * A1313 + ma[0][3] * A1213);
        mai[0][3] = - (ma[0][1] * A2312 - ma[0][2] * A1312 + ma[0][3] * A1212);
        mai[1][0] = - (ma[1][0] * A2323 - ma[1][2] * A0323 + ma[1][3] * A0223);
        mai[1][1] =   (ma[0][0] * A2323 - ma[0][2] * A0323 + ma[0][3] * A0223);
        mai[1][2] = - (ma[0][0] * A2313 - ma[0][2] * A0313 + ma[0][3] * A0213);
        mai[1][3] =   (ma[0][0] * A2312 - ma[0][2] * A0312 + ma[0][3] * A0212);
        mai[2][0] =   (ma[1][0] * A1323 - ma[1][1] * A0323 + ma[1][3] * A0123);
        mai[2][1] = - (ma[0][0] * A1323 - ma[0][1] * A0323 + ma[0][3] * A0123);
        mai[2][2] =   (ma[0][0] * A1313 - ma[0][1] * A0313 + ma[0][3] * A0113);
        mai[2][3] = - (ma[0][0] * A1312 - ma[0][1] * A0312 + ma[0][3] * A0112);
        mai[3][0] = - (ma[1][0] * A1223 - ma[1][1] * A0223 + ma[1][2] * A0123);
        mai[3][1] =   (ma[0][0] * A1223 - ma[0][1] * A0223 + ma[0][2] * A0123);
        mai[3][2] = - (ma[0][0] * A1213 - ma[0][1] * A0213 + ma[0][2] * A0113);
        mai[3][3] =   (ma[0][0] * A1212 - ma[0][1] * A0212 + ma[0][2] * A0112);
    }

    mm[0] = det * (mai[0][0] * mb[0] + mai[0][1] * mb[1] + mai[0][2] * mb[2] + mai[0][3] * mb[3]);
    mm[1] = det * (mai[1][0] * mb[0] + mai[1][1] * mb[1] + mai[1][2] * mb[2] + mai[1][3] * mb[3]);
    mm[2] = det * (mai[2][0] * mb[0] + mai[2][1] * mb[1] + mai[2][2] * mb[2] + mai[2][3] * mb[3]);
    mm[3] = det * (mai[3][0] * mb[0] + mai[3][1] * mb[1] + mai[3][2] * mb[2] + mai[3][3] * mb[3]);

    tm[0] = tm[4] = mm[0];
    tm[1] = -mm[1];
    tm[3] = mm[1];
    tm[2] = mm[2];
    tm[5] = mm[3];
}

static void invert_affine_transform(const float *tm, float *tm_inv)
{
    float D = tm[0] * tm[4] - tm[1] * tm[3];
    D = D != 0.f ? 1.f / D : 0.f;

    float A11 = tm[4] * D;
    float A22 = tm[0] * D;
    float A12 = -tm[1] * D;
    float A21 = -tm[3] * D;
    float b1 = -A11 * tm[2] - A12 * tm[5];
    float b2 = -A21 * tm[2] - A22 * tm[5];

    tm_inv[0] = A11;
    tm_inv[1] = A12;
    tm_inv[2] = b1;
    tm_inv[3] = A21;
    tm_inv[4] = A22;
    tm_inv[5] = b2;
}

int aximg_align_face(axobj_t *obj, axframe_t *src, axframe_t *dst)
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
    if ((dst->dtype == COLOR_SPACE_RGB) || (dst->dtype == COLOR_SPACE_BGR)) {
        dst->size = 112 * 112 * 3;
    } else if ((dst->dtype == COLOR_SPACE_NV12) || (dst->dtype == COLOR_SPACE_NV21)) {
        dst->size = 112 * 112 * 1.5;
    }  else {
        axnpu_error("just only support BGR/RGB/NV12 format");
    }

    return aximg_warp(src, dst, &mat3x3[0][0], 128);
}

API_END_NAMESPACE(Ai)
