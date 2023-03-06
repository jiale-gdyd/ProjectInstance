#ifndef NPU_AXNPU_RUNNERBASE_HPP
#define NPU_AXNPU_RUNNERBASE_HPP

#include "axapi.h"

#include <string>
#include <vector>
#include <cstdint>

API_BEGIN_NAMESPACE(Ai)

/* 模型输出信息 */
typedef struct {
    std::string           name;
    uint32_t              index;
    std::vector<uint32_t> shape;
    int                   size;
    uint64_t              phyaddr;
    void                  *viraddr;
} ax_tensor_t;

class AxRunnerBase {
public:
    virtual int init(std::string model) = 0;
    virtual void deinit() = 0;

    virtual int getColorSpace() = 0;
    virtual int getModelWidth() = 0;
    virtual int getModelHeight() = 0;

    virtual int forward(axframe_t *mediaFrame, const axbbox_t *cropReizeBBox) = 0;

    int getCount() {
        return mTensors.size();
    }

    const ax_tensor_t &getOutput(int index) {
        return mTensors[index];
    }

    const ax_tensor_t *getOutputsPtr() {
        return mTensors.data();
    }

protected:
    std::vector<ax_tensor_t> mTensors;
};

API_END_NAMESPACE(Ai)

#endif
