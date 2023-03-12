#pragma once

#include <ax_sys_api.h>
#include <ax_npu_imgproc.h>

#include "../../axpi.hpp"

namespace axpi {
int axpi_memfree(unsigned long long int phyaddr, void *pviraddr);
int axpi_memalloc(unsigned long long int *phyaddr, void **pviraddr, unsigned int size, unsigned int align, const char *token);

int axpi_imgproc_csc(axpi_image_t *src, axpi_image_t *dst);
int axpi_imgproc_warp(axpi_image_t *src, axpi_image_t *dst, const float *pMat33, const int const_val);

int axpi_imgproc_crop_resize(axpi_image_t *src, axpi_image_t *dst, axpi_bbox_t *box);
int axpi_imgproc_crop_resize_keep_ratio(axpi_image_t *src, axpi_image_t *dst, axpi_bbox_t *box);

int axpi_imgproc_align_face(axpi_object_t *obj, axpi_image_t *src, axpi_image_t *dst);

void convert(axpi_image_t *src, AX_NPU_CV_Image *dst);
}
