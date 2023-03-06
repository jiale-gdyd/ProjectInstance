#ifndef NPU_AXNPU_AXCOMM_API_HPP
#define NPU_AXNPU_AXCOMM_API_HPP

#include "axapi.h"

API_BEGIN_NAMESPACE(Ai)

int axsys_memfree(uint64_t phyaddr, void *pviraddr);
int axsys_memalloc(uint64_t *phyaddr, void **pviraddr, uint32_t size, uint32_t align, const char *token);

int aximg_csc(axframe_t *src, axframe_t *dst);
int aximg_warp(axframe_t *src, axframe_t *dst, const float *pMat33, const int const_val);

int aximg_crop_resize(axframe_t *src, axframe_t *dst, axbbox_t *box);
int aximg_crop_resize_keep_ratio(axframe_t *src, axframe_t *dst, axbbox_t *box);

int aximg_align_face(axobj_t *obj, axframe_t *src, axframe_t *dst);

void cvt(axframe_t *src, void *dst);

API_END_NAMESPACE(Ai)

#endif
