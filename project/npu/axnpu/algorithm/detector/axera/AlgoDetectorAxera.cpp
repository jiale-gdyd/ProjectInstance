#include "../AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::axera_extract(axdl_result_t *lastResult)
{
    if (mAlgoType == MT_DET_YOLOV5) {
        return axera_extract_yolov5(lastResult);
    }

    axnpu_error("Author axera detect algorithm:[%d] not support now", mAlgoType);
    return -1;
}

API_END_NAMESPACE(Ai)
