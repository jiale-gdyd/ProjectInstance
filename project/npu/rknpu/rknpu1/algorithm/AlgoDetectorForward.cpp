#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::forward(cv::Mat frame)
{
    mTensorMem = false;

    if (!mInitFin || (frame.data == NULL)) {
        printf("mInitFin:[%s], frame == NULL:[%s]\n", mInitFin ? "true" : "false", frame.data == NULL ? "true" : "false");
        return -1;
    }

    int ret = processInferImage(frame, mInferMat[mPingPongFlag], mCacheData[mPingPongFlag]);
    if (ret != 0) {
        printf("processInferImage failed, return:[%d]\n", ret);
        return -2;
    }

    ret = pAiEngine->forward(mInferMat[mPingPongFlag]);
    if (ret != 0) {
        printf("pAiEngine->forward failed, return:[%d]\n", ret);
    }

    if (mPingPongFlag == CACHE_PING) {
        mPingPongFlag = CACHE_PONG;
    } else {
        mPingPongFlag = CACHE_PING;
    }

    return ret;
}

int AlgoDetector::forward(void *mediaFrame)
{
    mTensorMem = false;

    if (!mInitFin || (mediaFrame == NULL)) {
        printf("mInitFin:[%s], mediaFrame == NULL:[%s]\n", mInitFin ? "true" : "false", mediaFrame == NULL ? "true" : "false");
        return -1;
    }

    int ret = processInferImage(mediaFrame, mInferMat[mPingPongFlag], mCacheData[mPingPongFlag]);
    if (ret != 0) {
        printf("processInferImage failed, return:[%d]\n", ret);
        return -2;
    }

    ret = pAiEngine->forward(mInferMat[mPingPongFlag]);
    if (ret != 0) {
        printf("pAiEngine->forward failed, return:[%d]\n", ret);
    }

    if (mPingPongFlag == CACHE_PING) {
        mPingPongFlag = CACHE_PONG;
    } else {
        mPingPongFlag = CACHE_PING;
    }

    return ret;
}

int AlgoDetector::forward(unsigned char *data, size_t dataSize)
{
    if (!mInitFin || (data == nullptr)) {
        printf("mInitFin:[%s], data == nullptr:[%s]\n", mInitFin ? "true" : "false", data == nullptr ? "true" : "false");
        return -1;
    }

    int ret = -1;
    if (mZeroCopy) {
        int index = 0;

        ret = pAccelerator->imageResize(data, mInputsTensorMem[index].fd);
        if (ret != 0) {
            mTensorMem = false;
            printf("[4] use physical address resize image failed, so try to use virtual address resize\n");
            goto USE_VIRADDR;
        }

        ret = pAiEngine->forward(mInputsTensorMem, index, mLockInputMem);
        if (ret != 0) {
            mTensorMem = false;
            printf("pAiEngine->forward failed\n");
            return -2;
        }

        mTensorMem = true;
    } else {
        mTensorMem = false;

USE_VIRADDR:
        if (0) {
            ret = pAccelerator->imageResize(data, mCacheData[mPingPongFlag]);
        } else {
            ret = pAccelerator->imageResizeEx(data, mCacheData[mPingPongFlag]);
        }

        if (ret != 0) {
            printf("pAccelerator->imageResize failed\n");
            return -3;
        }

        ret = pAiEngine->forward(mCacheData[mPingPongFlag], mCacheDataSize);
        if (ret != 0) {
            printf("pAiEngine->forward failed\n");
        }

        if (mPingPongFlag == CACHE_PING) {
            mPingPongFlag = CACHE_PONG;
        } else {
            mPingPongFlag = CACHE_PING;
        }
    }

    return ret;
}

API_END_NAMESPACE(Ai)
