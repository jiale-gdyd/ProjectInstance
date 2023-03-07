#include "../detection.hpp"
#include "../AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::axera_extract_yolov5(axdl_result_t *lastResult)
{
    std::vector<Object> objects;
    std::vector<Object> proposals;

    int outputSize = pAiEngine->getOutputSize();
    const AiRunTensor *pOutputInfo = pAiEngine->getOutputPtr();
    int modelInputWidth = pAiEngine->getModelWidth();
    int modelInputHeight = pAiEngine->getModelHeight();

    if ((int)mAnchors.size() != (outputSize * mAnchorSizePerStride)) {
        axnpu_error("Anchors size mismatch, should be %d, but got %d", outputSize * mAnchorSizePerStride, mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mConfThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputInfo[i];
        auto ptr = (float *)output.virAddr;

        generate_proposals_yolov5(mStrides[i], ptr, mConfThreshold, proposals, modelInputWidth, modelInputHeight, mAnchors.data(), prob_threshold_unsigmoid, mClassCount);
    }

    get_out_bbox(proposals, objects, mNmsThreshold, modelInputHeight, modelInputWidth, mImageHeight, mImageWidth);
    std::sort(objects.begin(), objects.end(), [&](Object &a, Object &b) {
        return a.rect.area() > b.rect.area();
    });

    lastResult->objectSize = std::min((int)objects.size(), AXDL_BBOX_MAX);
    for (int i = 0; i < lastResult->objectSize; i++) {
        const Object &obj = objects[i];
        lastResult->objects[i].bbox.x = obj.rect.x;
        lastResult->objects[i].bbox.y = obj.rect.y;
        lastResult->objects[i].bbox.w = obj.rect.width;
        lastResult->objects[i].bbox.h = obj.rect.height;
        lastResult->objects[i].label = obj.label;
        lastResult->objects[i].prob = obj.prob;
    }

    return lastResult->objectSize > 0 ? 0 : -2;
}

API_END_NAMESPACE(Ai)
