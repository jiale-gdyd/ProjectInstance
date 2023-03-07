#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

AlgoDetector::AlgoDetector(std::string modelFile, size_t imageWidth, size_t imageHeight, size_t classCount, float confThreshold, float nmsThreshold, int algoType, int algoAuthor)
    : mInitFin(false), pAiEngine(nullptr),
    mModelFile(modelFile), mImageWidth(imageWidth), mImageHeight(imageHeight),
    mClassCount(classCount), mConfThreshold(confThreshold), mNmsThreshold(nmsThreshold),
    mAlgoType(algoType), mAlgoAuthor(algoAuthor)
{

}

AlgoDetector::~AlgoDetector()
{
    mInitFin = false;
}

int AlgoDetector::init()
{
    pAiEngine = std::make_shared<Ai::AiEngine>();
    if (pAiEngine == nullptr) {
        axnpu_error("new AiEngine failed");
        return -1;
    }

    int ret = pAiEngine->init(mModelFile);
    if (ret != 0) {
        axnpu_error("ai engine init failed, return:[%d]", ret);
        return -2;
    }

    mInitFin = true;
    return 0;
}

int AlgoDetector::getModelInputWidthHeight(int &width, int &height, int &colorSpace) const
{
    if (!mInitFin) {
        axnpu_error("algo detector not initialized");
        return -1;
    }

    width = pAiEngine->getModelWidth();
    height = pAiEngine->getModelHeight();
    colorSpace = pAiEngine->getModelColorSpace();
}

int AlgoDetector::forward(axdl_frame_t *mediaFrame, axdl_bbox_t *cropResizeBbox, axdl_result_t *lastResult)
{
    if (!mInitFin) {
        axnpu_error("algo detector not initialized");
        return -1;
    }

    if (!mediaFrame) {
        axnpu_error("Invalid frame, maybe nullptr");
        return -2;
    }

    int ret = pAiEngine->inference(mediaFrame, cropResizeBbox);
    if (ret != 0) {
        axnpu_error("inference failed, return:[%d]", ret);
        return -3;
    }

    return extract(lastResult);
}

API_END_NAMESPACE(Ai)
