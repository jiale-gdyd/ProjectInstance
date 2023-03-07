#include <npu/Detector.hpp>
#include <npu/tracker/ObjectTracker.hpp>

#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

AiDetector::AiDetector() : mInitFin(false)
{

}

AiDetector::~AiDetector()
{
    mInitFin = false;
}

int AiDetector::init(std::string modelFile, size_t imageWidth, size_t imageHeight, size_t classCount, float confThreshold, float nmsThreshold, int algoType, int algoAuthor)
{
    mAlgoType = algoType;
    mAlgoAuthor = algoAuthor;

    mClsCount = classCount;
    mImageWidth = imageWidth;
    mImageHeight = imageHeight;

    for (size_t i = 0; i < mClsCount; ++i) {
        mClsTracker.push_back(std::make_shared<ObjectTracker>());
    }

    mDetector = std::make_shared<AlgoDetector>(modelFile, imageWidth, imageHeight, classCount, confThreshold, nmsThreshold, algoType, algoAuthor);
    if (mDetector == nullptr) {
        rknpu_error("new AlgoDetector failed");
        return -1;
    }

    if (mDetector->init()) {
        rknpu_error("mDetector detector init failed");
        return -2;
    }

    mDetector->getModelWHC(mInputWidth, mInputHeight, mImgChns);
    mInitFin = true;

    return 0;
}

int AiDetector::getModelInputWidthHeight(int &width, int &height, int &colorSpace) const
{
    if (!mInitFin) {
        rknpu_error("detector not init or with null mediaFrame");
        return -1;
    }

    width = mInputWidth;
    height = mInputHeight;
    return 0;
}

void AiDetector::setYoloAnchor(std::vector<yolo_layer_t> anchor)
{
    if (mDetector) {
        mDetector->setYoloAnchor(anchor);
    }
}

int AiDetector::forward(void *mediaFrame)
{
    if (!mInitFin || !mediaFrame || !mDetector) {
        rknpu_error("detector not init or with null mediaFrame");
        return -1;
    }

    return mDetector->forward(mediaFrame);
}

int AiDetector::forward(void *mediaFrame, std::vector<bbox> &lastResult)
{
    std::vector<int> empty;
    return forward(mediaFrame, lastResult, empty);
}

int AiDetector::forward(void *mediaFrame, std::vector<bbox> &lastResult, std::vector<int> filterClass)
{
    bool ret = forward(mediaFrame);
    if (!ret) {
        return -1;
    }

    return extract(lastResult, filterClass);
}

int AiDetector::extract(std::vector<bbox> &lastResult)
{
    std::vector<int> empty;
    return extract(lastResult, empty);
}

int AiDetector::extract(std::vector<bbox> &lastResult, std::vector<int> filterClass)
{
    if (!mInitFin || !mDetector) {
        return -1;
    }

    int ret = -1;
    std::map<int, std::vector<bbox>> algoResultMap;
    std::map<int, std::vector<bbox>> algoAfterResultMap, trackerResultMap;

    ret = mDetector->extract(algoResultMap, filterClass);
    if (ret) {
        return -2;
    }

    size_t mRealWidth = mInputWidth;
    size_t mRealHeight = mInputWidth;
    mDetector->getResizeRealWidthHeight(mRealWidth, mRealHeight);

    if (algoResultMap.size() > 0) {
        for (auto result : algoResultMap) {
            for (auto item : result.second) {
                if (mAlgoAuthor == AUTHOR_JIALELU) {
                    // 1.将填充后的坐标换算回填充前的坐标
                    item.left -= ((mInputWidth - mRealWidth) / 2.0f);
                    item.right -= ((mInputWidth - mRealWidth) / 2.0f);
                    item.top -= ((mInputHeight - mRealHeight) / 2.0f);
                    item.bottom -= ((mInputHeight - mRealHeight) / 2.0f);

                    // 2.将从填充后的坐标换算回填充前的坐标换算回原图上
                    item.left = std::max(0.0f, std::min((item.left) * (mImageWidth / (float)mRealWidth), (float)(mImageWidth - 1)));
                    item.top = std::max(0.0f, std::min((item.top) * (mImageHeight / (float)mRealHeight), (float)(mImageHeight - 1)));
                    item.right = std::max(0.0f, std::min((item.right) * (mImageWidth / (float)mRealWidth), (float)(mImageWidth - 1)));
                    item.bottom = std::max(0.0f, std::min((item.bottom) * (mImageHeight / (float)mRealHeight), (float)(mImageHeight - 1)));
                }

                auto it = algoAfterResultMap.find(result.first);
                if (it != algoAfterResultMap.end()) {
                    it->second.emplace_back(item);
                } else {
                    algoAfterResultMap.insert(std::make_pair(result.first, std::vector<bbox>{item}));
                }
            }
        }

        if ((mAlgoType == YOLOXS)
            || (mAlgoType == YOLOV5S)
            || (mAlgoType == YOLOX_TINY_FACE)
            || (mAlgoType == YOLOX_NANO_FACE)
            || (mAlgoType == YOLOX_NANO_FACE_TINY))
        {
            if (algoAfterResultMap.size() > 0) {
                for (auto result : algoAfterResultMap) {
                    if (((result.first >= 0) && (result.first < 80)) && (mClsTracker[result.first] != nullptr)) {
                        std::vector<bbox> tmp;
                        mClsTracker[result.first]->update(result.second);
                        mClsTracker[result.first]->get_boxes(tmp);

                        if (tmp.size() > 0) {
                            auto it = trackerResultMap.find(result.first);
                            if (it != trackerResultMap.end()) {
                                it->second.insert(it->second.end(), tmp.begin(), tmp.end());
                            } else {
                                trackerResultMap.insert(std::make_pair(result.first, tmp));
                            }
                        }
                    }
                }

                if (trackerResultMap.size() > 0) {
                    for (auto result : trackerResultMap) {
                        lastResult.insert(lastResult.end(), result.second.begin(), result.second.end());
                    }
                } else {
                    for (auto result : algoAfterResultMap) {
                        lastResult.insert(lastResult.end(), result.second.begin(), result.second.end());
                    }
                }
            }
        }
    }

    return 0;
}

API_END_NAMESPACE(Ai)
