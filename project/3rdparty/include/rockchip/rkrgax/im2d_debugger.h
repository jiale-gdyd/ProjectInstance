#ifndef RKRGA_IM2D_DEBUGGER_H
#define RKRGA_IM2D_DEBUGGER_H

#include "im2d_type.h"

const char *string_rd_mode(uint32_t mode);
const char *string_color_space(uint32_t mode);
const char *string_blend_mode(uint32_t mode);
const char *string_rotate_mode(uint32_t rotate);
const char *string_flip_mode(uint32_t flip);
const char *string_mosaic_mode(uint32_t mode);
const char *string_rop_mode(uint32_t mode);
const char *string_colorkey_mode(uint32_t mode);

#endif