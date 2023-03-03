#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::extract(std::map<int, std::vector<bbox>> &lastResults)
{
    std::vector<int> empty;
    return extract(lastResults, empty);
}

int AlgoDetector::extract(std::map<int, std::vector<bbox>> &lastResults, std::vector<int> filterClass)
{
    int ret = -1;

    if (mAlgoAuthor == AUTHOR_ROCKCHIP) {
        ret = extract_rockchip(lastResults, filterClass);
    } else if (mAlgoAuthor == AUTHOR_JIALELU) {
        ret = extract_jialelu(lastResults, filterClass);
    } else {
        rknpu_error("author:[%d] algorithm not support now", mAlgoAuthor);
        return -1;
    }

    return ret;
}

API_END_NAMESPACE(Ai)
