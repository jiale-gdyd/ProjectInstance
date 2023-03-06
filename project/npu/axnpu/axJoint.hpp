#ifndef NPU_AXNPU_AXJOINT_HPP
#define NPU_AXNPU_AXJOINT_HPP

#include <string>
#include <vector>
#include <fstream>

#include "axapi.h"
#include <joint.h>

API_BEGIN_NAMESPACE(Ai)

typedef struct {
    int                  width;         // 模型输入宽度
    int                  height;        // 模型输入高度
    int                  format;        // 模型输入格式
    uint32_t             outputs;       // 模型输出节点个数
    AX_JOINT_IOMETA_T    *iometa;       // 模型输出的节点信息数组指针，数组的长度对应outputs
    AX_JOINT_IO_BUFFER_T *iobuff;       // 模型输出的节点张量数组指针，数组的长度对应outputs
} axjoint_attr_t;

/**
 * 函数名称: axjoint_release
 * 功能描述: 释放算法句柄
 * 输入参数: handler --> 算法句柄
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int axjoint_release(void *handler);

/**
 * 函数名称: axjoint_create
 * 功能描述: 创建算法handler，并输出算法的输入分辨率，以供其他模块设置
 * 输入参数: model   --> 算法模型文件
 *          attr    --> 一些你可能需要的参数输出，推理完之后，模型输出的数据会自动填充到这里面的结构体指针里
 * 输出参数: handler --> 算法句柄
 * 返回说明: 成功返回0，其他则失败
 */
int axjoint_create(std::string model, void **handler, axjoint_attr_t *attr);

/**
 * 函数名称: axjoint_forward
 * 功能描述: 推理输入的图像，检测出bbox，并将映射到width/height上，以便osd绘制
 * 输入参数: handler        --> 算法句柄
 *          mediaFrame     --> 图像帧
 *          cropResizeBbox --> 需要扣的部分图像，如果不需要扣，就置为NULL
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int axjoint_forward(void *handler, void *mediaFrame, void *cropResizeBbox);

API_END_NAMESPACE(Ai)

#endif
