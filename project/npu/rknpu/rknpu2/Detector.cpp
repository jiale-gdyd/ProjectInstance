#include <npu/Detector.hpp>
#include <npu/tracker/ObjectTracker.hpp>

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
    return -1;
}

int AiDetector::forward(void *mediaFrame)
{
    return -1;
}

int AiDetector::forward(void *mediaFrame, std::vector<bbox> &lastResult)
{
    return -1;
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

void AiDetector::setYoloAnchor(std::vector<yolo_layer_t> anchor)
{

}

API_END_NAMESPACE(Ai)
