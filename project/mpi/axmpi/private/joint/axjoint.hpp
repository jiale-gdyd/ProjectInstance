#pragma once

#include <string>
#include <joint.h>
#include <stdint.h>

namespace axpi {
typedef struct {
    uint32_t             width;
    uint32_t             height;
    uint32_t             format;
    uint32_t             outSize;
    AX_JOINT_IOMETA_T    *outInfo;
    AX_JOINT_IO_BUFFER_T *outputs;
} axjoint_attr_t;

int axjoint_exit(void *handler);
int axjoint_init(std::string modelFile, void **handler, axjoint_attr_t *attr);

int axjoint_inference(void *handler, const void *frame, const void *crop_resize_box);
}