#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::dispObjects(media_buffer_t &mediaFrame, std::vector<Ai::bbox> lastResult)
{
    if (!mediaFrame || (lastResult.size() == 0)) {
        return -1;
    }

    cv::Mat frame = cv::Mat(mOriginHeight, mOriginWidth, CV_8UC(mOriginChns), getMedia()->getSys().getFrameData(mediaFrame));
    for (auto item : lastResult) {
        cv::rectangle(frame, cv::Point2f(item.left, item.top), cv::Point2f(item.right, item.bottom), CV_RGB(255, 0, 0), 3);
    }

    return 0;
}

API_END_NAMESPACE(EMS)
