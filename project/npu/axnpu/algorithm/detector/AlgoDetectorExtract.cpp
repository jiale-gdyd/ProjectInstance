#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::extract(axdl_result_t *lastResult)
{
    if (!lastResult) {
        axnpu_error("Invalid result cache");
        return -1;
    }

    if (mAlgoAuthor == AUTHOR_AXERA) {
        return axera_extract(lastResult);
    }

    axnpu_warn("author detect algorithm not support now");
    return -2;
}

API_END_NAMESPACE(Ai)
