#include <ax_npu_imgproc.h>

#include "../joint/axjoint.hpp"
#include "../utilities/log.hpp"
#include "axpi_model_runner_ax620.hpp"

namespace axpi {
struct axjoint_runner_ax620_handle_t {
    void          *m_handle = nullptr;
    axjoint_attr_t m_attr = {0};
};

int AxPiRunnerAx620::init(std::string modelFile)
{
    if (m_handle) {
        return -1;
    }

    m_handle = new axjoint_runner_ax620_handle_t;
    int ret = axjoint_init(modelFile, &m_handle->m_handle, &m_handle->m_attr);
    if (ret) {
        axmpi_error("axjoint_init failed, return:[%d]", ret);
        return ret;
    }

    for (size_t i = 0; i < m_handle->m_attr.outSize; i++) {
        axpi_runner_tensor_t tensor;

        tensor.index = i;
        tensor.name = std::string(m_handle->m_attr.outInfo[i].pName);
        tensor.size = m_handle->m_attr.outInfo[i].nSize;
        for (size_t j = 0; j < m_handle->m_attr.outInfo[i].nShapeSize; j++) {
            tensor.shape.push_back(m_handle->m_attr.outInfo[i].pShape[j]);
        }

        tensor.phyAddr = m_handle->m_attr.outputs[i].phyAddr;
        tensor.pVirAddr = m_handle->m_attr.outputs[i].pVirAddr;
        mTensors.push_back(tensor);
    }

    return ret;
}

int AxPiRunnerAx620::exit()
{
    if (m_handle && m_handle->m_handle) {
        axjoint_exit(m_handle->m_handle);
    }

    delete m_handle;
    m_handle = nullptr;

    return 0;
}

int AxPiRunnerAx620::getAlgoWidth()
{
    return m_handle->m_attr.width;
}

int AxPiRunnerAx620::getAlgoHeight()
{
    return m_handle->m_attr.height;
}

int AxPiRunnerAx620::getColorSpace()
{
    switch (m_handle->m_attr.format) {
        case AX_FORMAT_RGB888:
            return AXPI_COLOR_SPACE_RGB;

        case AX_FORMAT_BGR888:
            return AXPI_COLOR_SPACE_BGR;

        case AX_YUV420_SEMIPLANAR:
            return AXPI_COLOR_SPACE_NV12;

        default:
            return AXPI_COLOR_SPACE_UNK;
    }
}

int AxPiRunnerAx620::inference(axpi_image_t *pstFrame, const axpi_bbox_t *crop_resize_box)
{
    AX_NPU_CV_Image npu_image;
    convert(pstFrame, &npu_image);
    return axjoint_inference(m_handle->m_handle, &npu_image, crop_resize_box);
}
}
