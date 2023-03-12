#pragma once

#include <stdint.h>
#include <ax_base_type.h>

namespace axpi {
#define MAX_POOLS               5

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t widthStride;
    int      format;
    uint32_t blockCount;
} axsys_pool_cfg_t;

typedef struct {
    uint8_t          camCount;
    uint32_t         poolCfgCount;
    axsys_pool_cfg_t *poolCfg;
} axsys_args_t;

int axsys_init(axsys_args_t *pArgs);
int axsys_deinit();
}
