#pragma once

#include <vector>
#include <string>

#include "axpi_common_api.hpp"

namespace axpi {
typedef struct {
    std::string               name;
    unsigned int              index;
    std::vector<unsigned int> shape;
    unsigned int              size;
    unsigned long             phyAddr;
    void                      *pVirAddr;
} axpi_runner_tensor_t;

class  AxPiRunnerBase {
public:
    virtual int exit() = 0;
    virtual int init(std::string modelFile) = 0;

    int getOutputCount() {
        return mTensors.size();
    };

    const axpi_runner_tensor_t &getSpecOutput(int idx) {
        return mTensors[idx];
    }

    const axpi_runner_tensor_t *getOutputsPtr() {
        return mTensors.data();
    }

    virtual int getAlgoWidth() = 0;
    virtual int getAlgoHeight() = 0;
    virtual int getColorSpace() = 0;

    virtual int inference(axpi_image_t *pstFrame, const axpi_bbox_t *crop_resize_box) = 0;

protected:
    std::vector<axpi_runner_tensor_t> mTensors;
};
}
