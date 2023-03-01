#ifndef AARCH32_ROCKCHIP_RKNPU1_PRIVATE_HPP
#define AARCH32_ROCKCHIP_RKNPU1_PRIVATE_HPP

#include <map>
#include <vector>
#include <functional>
#include <rknn/rknn_runtime.h>

#include "../AiStruct.hpp"

API_BEGIN_NAMESPACE(Ai)

using nms_func_ptr = std::function<void (std::vector<bbox> &inputBbox, float threshold)>;
using extract_func_ptr = std::function<int (rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t count, nms_func_ptr nms_cb, void *args, std::map<int, std::vector<bbox>> &lastResult)>;

API_END_NAMESPACE(Ai)

#endif
