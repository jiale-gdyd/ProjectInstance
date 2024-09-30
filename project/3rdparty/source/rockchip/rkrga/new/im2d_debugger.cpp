#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <rockchip/rkrgax/im2d_log.h>
#include <rockchip/rkrgax/RockchipRga.h>
#include <rockchip/rkrgax/im2d_debugger.h>

const char *string_rd_mode(uint32_t mode)
{
    switch (mode) {
        case IM_RASTER_MODE:
            return "raster";

        case IM_FBC_MODE:
            return "afbc16x16";

        case IM_TILE_MODE:
            return "tile8x8";

        default:
            return "unknown";
    }
}

const char *string_color_space(uint32_t mode)
{
    switch (mode) {
        case IM_YUV_TO_RGB_BT601_LIMIT:
            return "yuv2rgb-bt.601-limit";

        case IM_YUV_TO_RGB_BT601_FULL:
            return "yuv2rgb-bt.601-full";

        case IM_YUV_TO_RGB_BT709_LIMIT:
            return "yuv2rgb-bt.709-limit";

        case IM_RGB_TO_YUV_BT601_FULL:
            return "rgb2yuv-bt.601-full";

        case IM_RGB_TO_YUV_BT601_LIMIT:
            return "rgb2yuv-bt.601-limit";

        case IM_RGB_TO_YUV_BT709_LIMIT:
            return "rgb2yuv-bt.709-limit";

        case IM_RGB_TO_Y4:
            return "rgb-to-y4";

        case IM_RGB_TO_Y4_DITHER:
            return "rgb-to-y4-dither";

        case IM_RGB_TO_Y1_DITHER:
            return "rgb-to-y1-dither";

        case IM_COLOR_SPACE_DEFAULT:
            return "default";

        case IM_RGB_FULL:
            return "rgb_full";

        case IM_RGB_CLIP:
            return "rga_clip";

        case IM_YUV_BT601_LIMIT_RANGE:
            return "yuv_bt.601-limit";

        case IM_YUV_BT601_FULL_RANGE:
            return "yuv_bt.601-full";

        case IM_YUV_BT709_LIMIT_RANGE:
            return "yuv_bt.709-limit";

        case IM_YUV_BT709_FULL_RANGE:
            return "yuv_bt.709-full";

        default:
            return "unknown";
    }
}

const char *string_blend_mode(uint32_t mode)
{
    switch (mode) {
        case IM_ALPHA_BLEND_SRC:
            return "src";

        case IM_ALPHA_BLEND_DST:
            return "dst";

        case IM_ALPHA_BLEND_SRC_OVER:
            return "src-over";

        case IM_ALPHA_BLEND_DST_OVER:
            return "dst-over";

        case IM_ALPHA_BLEND_SRC_IN:
            return "src-in";

        case IM_ALPHA_BLEND_DST_IN:
            return "dst-in";

        case IM_ALPHA_BLEND_SRC_OUT:
            return "src-out";

        case IM_ALPHA_BLEND_DST_OUT:
            return "dst-our";

        case IM_ALPHA_BLEND_SRC_ATOP:
            return "src-atop";

        case IM_ALPHA_BLEND_DST_ATOP:
            return "dst-atop";

        case IM_ALPHA_BLEND_XOR:
            return "xor";

        default:
            return "unknown";
    }
}

const char *string_rotate_mode(uint32_t rotate)
{
    switch (rotate) {
        case IM_HAL_TRANSFORM_ROT_90:
            return "90";

        case IM_HAL_TRANSFORM_ROT_180:
            return "180";

        case IM_HAL_TRANSFORM_ROT_270:
            return "270";

        default:
            return "unknown";
    }
}

const char *string_flip_mode(uint32_t flip)
{
    switch (flip) {
        case IM_HAL_TRANSFORM_FLIP_H:
            return "horiz";

        case IM_HAL_TRANSFORM_FLIP_V:
            return "verti";

        case IM_HAL_TRANSFORM_FLIP_H_V:
            return "horiz & verti";

        default:
            return "unknown";
    }
}

const char *string_mosaic_mode(uint32_t mode)
{
    switch (mode) {
        case IM_MOSAIC_8:
            return "mosaic 8x8";

        case IM_MOSAIC_16:
            return "mosaic 16x16";

        case IM_MOSAIC_32:
            return "mosaic 32x32";

        case IM_MOSAIC_64:
            return "mosaic 64x64";

        case IM_MOSAIC_128:
            return "mosaic 128x128";

        default:
            return "unknown";
    }
}

const char *string_rop_mode(uint32_t mode)
{
    switch (mode) {
        case IM_ROP_AND:
            return "and";

        case IM_ROP_OR:
            return "or";

        case IM_ROP_NOT_DST:
            return "not-dst";

        case IM_ROP_NOT_SRC:
            return "not-src";

        case IM_ROP_XOR:
            return "xor";

        case IM_ROP_NOT_XOR:
            return "not-xor";

        default:
            return "unknown";
    }
}

const char *string_colorkey_mode(uint32_t mode)
{
    switch (mode) {
        case IM_ALPHA_COLORKEY_NORMAL:
            return "normal";

        case IM_ALPHA_COLORKEY_INVERTED:
            return "inverted";

        default:
            return "unknown";
    }
}
