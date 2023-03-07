#include <npu/Detector.hpp>
#include <npu/tracker/ObjectTracker.hpp>

#include "detector/AlgoDetector.hpp"

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

    mDetector = std::make_shared<AlgoDetector>(modelFile, imageWidth, imageHeight, classCount, confThreshold, nmsThreshold, algoType, algoAuthor);
    if (mDetector == nullptr) {
        axnpu_error("new AlgoDetector failed");
        return -1;
    }

    if (mDetector->init()) {
        axnpu_error("mDetector detector init failed");
        return -2;
    }

    mInitFin = true;
    return 0;
}

int AiDetector::getModelInputWidthHeight(int &width, int &height, int &colorSpace) const
{
    return mDetector->getModelInputWidthHeight(width, height, colorSpace);
}

void AiDetector::setYoloAnchor(std::vector<yolo_layer_t> anchor)
{

}

int AiDetector::forward(void *mediaFrame)
{
    return -1;
}

int AiDetector::forward(void *mediaFrame, std::vector<bbox> &lastResult)
{
    std::vector<int> empty;
    return forward(mediaFrame, lastResult);
}

int AiDetector::forward(void *mediaFrame, std::vector<bbox> &lastResult, std::vector<int> filterClass)
{
    return -1;
}

int AiDetector::extract(std::vector<bbox> &lastResult)
{
    return -1;
}

int AiDetector::extract(std::vector<bbox> &lastResult, std::vector<int> filterClass)
{
    return -1;
}

int AiDetector::forward(void *mediaFrame, axdl_result_t *lastResult)
{
    std::vector<int> empty;
    return forward(mediaFrame, lastResult);
}

int AiDetector::forward(void *mediaFrame, axdl_result_t *lastResult, std::vector<int> filterClass)
{
    if (!mInitFin) {
        return -1;
    }

    return mDetector->forward((axdl_frame_t *)mediaFrame, NULL, lastResult);
}

API_END_NAMESPACE(Ai)
