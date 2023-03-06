#ifndef MAIX_JOINT_MIDDLEWARE_IO_HPP
#define MAIX_JOINT_MIDDLEWARE_IO_HPP

#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstring>

#include <joint.h>
#include <joint_adv.h>
#include <ax_sys_api.h>
#include <utils/export.h>
#include <ax_interpreter_external_api.h>

#include "../private.hpp"

API_BEGIN_NAMESPACE(joint)

typedef AX_JOINT_IO_T axjoint_io_t;
typedef AX_JOINT_IOMETA_T axjoint_iometa_t;
typedef AX_JOINT_IO_INFO_T axjoint_io_info_t;
typedef AX_JOINT_IO_BUFFER_T axjoint_io_buff_t;

static inline int parse_npu_mode_from_joint(const char *data, uint32_t &data_size, int *pNPUMode)
{
    AX_NPU_SDK_EX_MODEL_TYPE_T npu_type;

    auto ret = AX_JOINT_GetJointModelType(data, data_size, &npu_type);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        maxix_error("Get joint model type failed, return:[%X]", ret);
        return -1;
    }

    if (AX_NPU_MODEL_TYPE_DEFUALT == npu_type) {
        *pNPUMode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_DISABLE;
    } else if ((npu_type == AX_NPU_MODEL_TYPE_1_1_1) || (npu_type == AX_NPU_MODEL_TYPE_1_1_2)) {
        *pNPUMode = AX_NPU_SDK_EX_HARD_MODE_T::AX_NPU_VIRTUAL_1_1;
    } else {
        maxix_error("Unknown npu mode:[%d]", (int)npu_type);
        return -2;
    }

    return 0;
}

static inline int alloc_joint_buffer(const axjoint_iometa_t *pMeta, axjoint_io_buff_t *pBuf)
{
    axjoint_iometa_t meta = *pMeta;
    auto ret = AX_JOINT_AllocBuffer(&meta, pBuf, AX_JOINT_ABST_DEFAULT);
    if (0 != ret) {
        maxix_error("Cannot allocate memory");
        return -1;
    }

    return 0;
}

static inline int free_joint_buffer(axjoint_io_buff_t* pBuf)
{
    auto ret = AX_JOINT_FreeBuffer(pBuf);
    if (0 != ret) {
        maxix_error("Free allocated memory failed");
        return -1;
    }

    return 0;
}

static inline int copy_to_device(const void *buf, void **vir_addr, long file_size)
{
    memcpy(*vir_addr, buf, file_size);
    return 0;
}

static inline int copy_to_device(const void* buf, const size_t &size, axjoint_io_buff_t *pBuf)
{
    if (size > pBuf->nSize) {
        maxix_error("Target space is not large enough");
        return -1;
    }

    std::memcpy(pBuf->pVirAddr, buf, size);
    return 0;
}

static inline int copy_to_device(const void *buf, const size_t &size, const axjoint_iometa_t *pMeta, axjoint_io_buff_t *pBuf)
{
    if (size != pMeta->nSize) {
        maxix_error("Target space is not large enough");
        return -1;
    }

    uint32_t stride_len_wc = pBuf->pStride[1];
    uint32_t write_len_wc = pMeta->pShape[3] * pMeta->pShape[2];

    for (int j = 0; j < pMeta->pShape[0]; j++) {
        uint32_t dst_offset = j * pBuf->pStride[0];
        uint32_t src_offset = pMeta->nSize * j / pMeta->pShape[0];

        for (int i = 0; i < pMeta->pShape[1]; i++) {
            auto src_ptr = (uint8_t *)buf + src_offset + i * write_len_wc;
            auto dst_ptr = (uint8_t *)pBuf->pVirAddr + dst_offset + i * stride_len_wc;
            memcpy(dst_ptr, src_ptr, write_len_wc);
        }
    }

    return 0;
}

static inline std::vector<int> io_get_input_size(const axjoint_io_info_t *io_info)
{
    const axjoint_iometa_t* pMeta = io_info->pInputs;
    if (pMeta->nShapeSize <= 0) {
        maxix_error("Dimension:[%u] of shape is not allowed", (uint32_t)pMeta->nShapeSize);
    }

    return {pMeta->pShape[1], pMeta->pShape[2]};
}

static inline axjoint_io_buff_t *prepare_io_no_copy(const size_t &size, axjoint_io_t &io, const axjoint_io_info_t *io_info, const uint32_t &batch = 1)
{
    std::memset(&io, 0, sizeof(io));

    io.nInputSize = io_info->nInputSize;
    if (1 != io.nInputSize) {
        maxix_error("Only single input was accepted(got %u)", io.nInputSize);
        return nullptr;
    }

    io.pInputs = new axjoint_io_buff_t[io.nInputSize];

    axjoint_io_buff_t *pBuf = io.pInputs;
    const axjoint_iometa_t *pMeta = io_info->pInputs;

    if (pMeta->nShapeSize <= 0) {
        maxix_error("Dimension:[%u] of shape is not allowed", (uint32_t)pMeta->nShapeSize);
        return nullptr;
    }

    auto actual_data_size = pMeta->nSize / pMeta->pShape[0] * batch;
    if (size != actual_data_size) {
        maxix_error("The buffer size is not equal to model input:[%s] size:[%u vs %u]", io_info->pInputs[0].pName, (uint32_t)size, actual_data_size);
        return nullptr;
    }

    auto ret = alloc_joint_buffer(pMeta, pBuf);
    if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
        maxix_error("Can not allocate memory for model input");
        return nullptr;
    }

    io.nOutputSize = io_info->nOutputSize;
    io.pOutputs = new axjoint_io_buff_t[io.nOutputSize];

    for (size_t i = 0; i < io.nOutputSize; ++i) {
        axjoint_io_buff_t *pBuf = io.pOutputs + i;
        const axjoint_iometa_t *pMeta = io_info->pOutputs + i;

        alloc_joint_buffer(pMeta, pBuf);
    }

    return pBuf;
}

static inline int prepare_io(const void *buf, const size_t &size, axjoint_io_t &io, const axjoint_io_info_t *io_info, const uint32_t &batch = 1)
{
    std::memset(&io, 0, sizeof(io));

    io.nInputSize = io_info->nInputSize;
    if (1 != io.nInputSize) {
        maxix_error("Only single input was accepted(got %u)", io.nInputSize);
        return -1;
    }

    io.pInputs = new axjoint_io_buff_t[io.nInputSize];

    {
        axjoint_io_buff_t *pBuf = io.pInputs;
        const axjoint_iometa_t *pMeta = io_info->pInputs;

        if (pMeta->nShapeSize <= 0) {
            maxix_error("Dimension:[%u] of shape is not allowed", (uint32_t)pMeta->nShapeSize);
            return -1;
        }

        auto actual_data_size = pMeta->nSize / pMeta->pShape[0] * batch;
        if (size != actual_data_size) {
            maxix_error("The buffer size is not equal to model input:[%s] size:[%u vs %u]", io_info->pInputs[0].pName, (uint32_t)size, actual_data_size);
            return -1;
        }

        auto ret = alloc_joint_buffer(pMeta, pBuf);
        if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
            maxix_error("Can not allocate memory for model input");
            return -1;
        }

        ret = copy_to_device(buf, size, pBuf);
        if (AX_ERR_NPU_JOINT_SUCCESS != ret) {
            maxix_error("Can not copy data to input");
            return -1;
        }
    }

    {
        io.nOutputSize = io_info->nOutputSize;
        io.pOutputs = new axjoint_io_buff_t[io.nOutputSize];

        for (size_t i = 0; i < io.nOutputSize; ++i)  {
            axjoint_io_buff_t *pBuf = io.pOutputs + i;
            const axjoint_iometa_t *pMeta = io_info->pOutputs + i;

            alloc_joint_buffer(pMeta, pBuf);
        }
    }

    return 0;
}

API_END_NAMESPACE(joint)

#endif
