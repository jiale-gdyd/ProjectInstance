#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::dispObjects(std::vector<Ai::bbox> lastResult)
{
    getRegion()->putText(0, "有目标啦 - ^_^", cv::Point(20, 50), 50, CV_RGB(255, 0, 255));
    for (auto item : lastResult) {
        getRegion()->drawRect(cv::Point(item.left, item.top), cv::Point(item.right, item.bottom), CV_RGB(255, 0, 0), 3);
    }

    if (!mBlendImage.empty()) {
        int iconW = mBlendImage.cols;
        int iconH = mBlendImage.rows;
        int startX = (mOriginWidth - iconW) / 2;
        int startY = (mOriginHeight - iconH) / 2;
        getRegion()->drawImage(mBlendImage, cv::Point(startX, startY), iconW, iconH);
    }

    return 0;
}

API_END_NAMESPACE(EMS)
