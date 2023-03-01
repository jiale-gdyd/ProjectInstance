#include "../AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::extract_jialelu(std::map<int, std::vector<bbox>> &lastResults, std::vector<int> filterClass)
{
    std::lock_guard<std::mutex> lock(mCallbackMutex);

    int ret;
    static algo_args_t args;
    static bool bAnchorsAssignLock = false;
    std::map<int, std::vector<bbox>> localResults;

    args.algo_author = mAlgoAuthor;
    args.nms_threshold = mNmsThreshold;
    args.conf_threshold = mConfThreshold;
    args.classCount = mClassCount;
    args.filterClass = filterClass;
    args.outputsAttr = mModelOutputChnAttr;
    args.imageWidth = mImageWidth;
    args.imageHeight = mImageHeight;
    args.modelInputWidth = mInputWidth;
    args.modelInputHeight = mInputHeight;
    args.realWidth = mRealWidth;
    args.realHeight = mRealHeight;

    if (!bAnchorsAssignLock) {
        if (mYoloAnchors.empty()) {
            mYoloAnchors = getYolov5DefaultAnchors();
        } else {
            bAnchorsAssignLock = true;
        }

        args.yoloAnchors.clear();
        args.yoloAnchors.assign(mYoloAnchors.begin(), mYoloAnchors.end());
    }

    if (mTensorMem) {
        if (mAlgoType == YOLOV5S) {
            ret = extract_yolov5s_tensormem_jialelu(mOutputsTensorMem, (void *)&args, localResults);
        } else if (mAlgoType == YOLOXS) {
            ret = extract_yoloxs_tensormem_jialelu(mOutputsTensorMem, (void *)&args, localResults);
        } else if (mAlgoType == YOLOX_TINY_FACE) {
            ret = extract_yolox_tiny_face_tensormem_jialelu(mOutputsTensorMem, (void *)&args, localResults);
        } else if (mAlgoType == YOLOX_NANO_FACE) {
            ret = extract_yolox_nano_face_one_anchor_tensormem_jialelu(mOutputsTensorMem, (void *)&args, localResults);
        } else if (mAlgoType == YOLOX_NANO_FACE_TINY) {
            ret = extract_yolox_nano_face_one_anchor_tiny_tensormem_jialelu(mOutputsTensorMem, (void *)&args, localResults);
        } else {
            printf("jialelu algorithm type:[%d] not support now\n", mAlgoType);
            return -1;
        }
    } else {
        if (mAlgoType == YOLOV5S) {
            ret = pAiEngine->extract(extract_yolov5s_non_tensormem_jialelu, pAiEngine->nms, (void *)&args, localResults);
        } else if (mAlgoType == YOLOXS) {
            ret = pAiEngine->extract(extract_yoloxs_non_tensormem_jialelu, pAiEngine->nms, (void *)&args, localResults);
        } else if (mAlgoType == YOLOX_TINY_FACE) {
            ret = pAiEngine->extract(extract_yolox_tiny_face_non_tensormem_jialelu, pAiEngine->nms, (void *)&args, localResults);
        } else if (mAlgoType == YOLOX_NANO_FACE) {
            ret = pAiEngine->extract(extract_yolox_nano_face_one_anchor_non_tensormem_jialelu, pAiEngine->nms, (void *)&args, localResults);
        } else if (mAlgoType == YOLOX_NANO_FACE_TINY) {
            ret = pAiEngine->extract(extract_yolox_nano_face_one_anchor_tiny_non_tensormem_jialelu, pAiEngine->nms, (void *)&args, localResults);
        } else {
            printf("jialelu algorithm type:[%d] not support now", mAlgoType);
            return -1;
        }
    }

    if ((localResults.size() > 0) && (ret == 0)) {
        ret = 0;
        lastResults.insert(localResults.begin(), localResults.end());
    } else {
        ret = -1;
    }

    return ret;
}

API_END_NAMESPACE(Ai)
