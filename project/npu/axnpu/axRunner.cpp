#include <npu_common.h>

#include "axJoint.hpp"
#include "axRunner.hpp"
#include "axCommApi.hpp"

API_BEGIN_NAMESPACE(Ai)

struct AxJointRunnerHandler {
    void           *handler = nullptr;
    axjoint_attr_t attribute = {0};
};

AxRunner::~AxRunner()
{
    deinit();
}

int AxRunner::init(std::string model)
{
    if (mHandler) {
        return -1;
    }

    mHandler = new AxJointRunnerHandler;
    int ret = axjoint_create(model, &mHandler->handler, &mHandler->attribute);
    if (ret) {
        axnpu_error("axjoint_create failed, return:[%d]", ret);
        return ret;
    }

    for (size_t i = 0; i < mHandler->attribute.outputs; i++) {
        ax_tensor_t tensor;

        tensor.index = i;
        tensor.name = std::string(mHandler->attribute.iometa[i].pName);
        tensor.size = mHandler->attribute.iometa[i].nSize;
        for (size_t j = 0; j < mHandler->attribute.iometa[i].nShapeSize; j++) {
            tensor.shape.push_back(mHandler->attribute.iometa[i].pShape[j]);
        }

        tensor.phyaddr = mHandler->attribute.iobuff[i].phyAddr;
        tensor.viraddr = mHandler->attribute.iobuff[i].pVirAddr;
        mTensors.push_back(tensor);
    }

    return 0;
}

void AxRunner::deinit()
{
    if (mHandler && mHandler->handler) {
        axjoint_release(mHandler->handler);
    }

    delete mHandler;
    mHandler = nullptr;
}

int AxRunner::getColorSpace()
{
    switch (mHandler->attribute.format) {
        case AX_FORMAT_RGB888:
            return COLOR_SPACE_RGB;

        case AX_FORMAT_BGR888:
            return COLOR_SPACE_BGR;

        case AX_YUV420_SEMIPLANAR:
            return COLOR_SPACE_NV12;

        default:
            return COLOR_SPACE_UNK;
    }
}

int AxRunner::getModelWidth()
{
    return mHandler->attribute.width;
}

int AxRunner::getModelHeight()
{
    return mHandler->attribute.height;
}

int AxRunner::forward(axframe_t *mediaFrame, const axbbox_t *cropReizeBBox)
{
    AX_NPU_CV_Image npu_image;
    cvt(mediaFrame, &npu_image);
    return axjoint_forward(mHandler->handler, &npu_image, (void *)cropReizeBBox);
}

API_END_NAMESPACE(Ai)
