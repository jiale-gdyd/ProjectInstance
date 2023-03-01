#include <rockchip/rkrga/im2d.h>
#include <media/drm_media_buffer.h>

#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

AlgoDetector::AlgoDetector(std::string modelFile, size_t imageWidth, size_t imageHeight, size_t classCount, float confThreshold, float nmsThreshold, int algoType, int algoAuthor)
    : mInitFin(false), mZeroCopy(false), mTensorMem(false), mLockInputMem(false),
    pAiEngine(nullptr), pAccelerator(nullptr),
    mModelFile(modelFile), mImageWidth(imageWidth), mImageHeight(imageHeight),
    mClassCount(classCount), mConfThreshold(confThreshold), mNmsThreshold(nmsThreshold),
    mAlgoType(algoType), mAlgoAuthor(algoAuthor)
{
    pAiEngine = new AiEngine();
    mCacheData[CACHE_PING] = nullptr;
    mCacheData[CACHE_PONG] = nullptr;
}

AlgoDetector::~AlgoDetector()
{
    mInitFin = false;
    if (pAiEngine) {
        delete pAiEngine;
        pAiEngine = nullptr;
    }

    if (pAccelerator) {
        delete pAccelerator;
        pAccelerator = nullptr;
    }

    if (pAccelerator && mZeroCopy) {
        pAccelerator->releaseTensorMemory(mInputsTensorMem);
        pAccelerator->releaseTensorMemory(mOutputsTensorMem);
    }

    if (mCacheData[CACHE_PING]) {
        delete[] mCacheData[CACHE_PING];
        mCacheData[CACHE_PING] = nullptr;
    }

    if (mCacheData[CACHE_PONG]) {
        delete[] mCacheData[CACHE_PONG];
        mCacheData[CACHE_PONG] = nullptr;
    }
}

int AlgoDetector::init()
{
    pAccelerator = new hardware::Accelerator();
    if (pAccelerator == nullptr) {
        printf("new accelerator failed\n");
        return -1;
    }

    if (pAiEngine == nullptr) {
        printf("new AiEngine failed\n");
        return -2;
    }

    int ret = pAiEngine->init(mModelFile);
    if (ret != 0) {
        printf("AiEngine init failed\n");
        return -3;
    }

    mModelInputCount = pAiEngine->getInputCount();
    mModelOutputCount = pAiEngine->getOutputCount();
    mModelInputChnAttr = pAiEngine->getInputAttr();
    mModelOutputChnAttr = pAiEngine->getOutputAttr();

    if (mModelInputChnAttr[0].fmt == RKNN_TENSOR_NHWC) {
        mModelChns = mModelInputChnAttr[0].dims[0];
        mInputWidth = mRealWidth = mModelInputChnAttr[0].dims[1];
        mInputHeight = mRealHeight = mModelInputChnAttr[0].dims[2];

    } else if (mModelInputChnAttr[0].fmt == RKNN_TENSOR_NCHW) {
        mModelChns = mModelInputChnAttr[0].dims[2];
        mInputWidth = mRealWidth = mModelInputChnAttr[0].dims[0];
        mInputHeight = mRealHeight = mModelInputChnAttr[0].dims[1];
    } else {
        printf("unknow tensor format\n");
        return -4;
    }

    if (mZeroCopy) {
        if (pAccelerator->init(mInputWidth, mInputHeight, mImageWidth, mImageHeight, mModelChns) != 0) {
            printf("pAccelerator init failed\n");
            return -5;
        }
    }

    mCacheDataSize = mInputWidth * mInputHeight * mModelChns;
    mCacheData[CACHE_PING] = new unsigned char[mCacheDataSize + 1];
    if (mCacheData[CACHE_PING] == nullptr) {
        return -6;
    }

    mCacheData[CACHE_PONG] = new unsigned char[mCacheDataSize + 1];
    if (mCacheData[CACHE_PONG] == nullptr) {
        return -7;
    }

    if (mZeroCopy) {
        mInputsTensorMem.resize(mModelInputCount);
        mOutputsTensorMem.resize(mModelOutputCount);

        ret = pAccelerator->createTensorMemory(mInputsTensorMem, mModelInputChnAttr, false, "input ");
        if (ret != 0) {
            mZeroCopy = false;
            printf("pAccelerator->createTensorMemory[I] failed\n");
            return -8;
        }

        ret = pAccelerator->createTensorMemory(mOutputsTensorMem, mModelOutputChnAttr, true, "output");
        if (ret != 0) {
            mZeroCopy = false;
            pAccelerator->releaseTensorMemory(mInputsTensorMem);
            printf("pAccelerator->createTensorMemory[O] failed\n");
            return -9;
        }

        pAiEngine->setOutputMemory(mOutputsTensorMem);
    }

    mInitFin = true;
    return 0;
}

void AlgoDetector::setYoloAnchor(std::vector<yolo_layer_t> anchor)
{
    if (!anchor.empty()) {
        mYoloAnchors.clear();
        mYoloAnchors.assign(anchor.begin(), anchor.end());
    }
}

void AlgoDetector::getModelWHC(size_t &width, size_t &height, size_t &channels)
{
    width = mInputWidth;
    height = mInputHeight;
    channels = mModelChns;
}

void AlgoDetector::getResizeRealWidthHeight(size_t &width, size_t &height)
{
    width = mRealWidth;
    height = mRealHeight;
}

int AlgoDetector::processInferImage(void *mediaFrame, cv::Mat &inferMat, unsigned char *virtualCache)
{
    if (!virtualCache) {
        printf("virtualCache NULL\n");
        return -1;
    }

    bool bEqual = false;
    float scale_letterbox;
    rga_buffer_t src, dst;
    im_rect src_rect, dst_rect;
    int resize_rows, resize_cols;
    static bool bPrintErr = false;
    IM_STATUS ret = IM_STATUS_NOERROR;

    memset(&src, 0x00, sizeof(src));
    memset(&dst, 0x00, sizeof(dst));
    memset(&src_rect, 0x00, sizeof(src_rect));
    memset(&dst_rect, 0x00, sizeof(dst_rect));

    if ((mInputWidth * 1.0f / mImageWidth) != (mInputHeight * 1.0f / mImageHeight)) {
        if ((mInputWidth * 1.0f / mImageWidth) > (mInputHeight * 1.0f / mImageHeight)) {
            scale_letterbox = mInputHeight * 1.0f / mImageHeight;
        } else {
            scale_letterbox = mInputWidth * 1.0f / mImageWidth;
        }
    } else {
        bEqual = true;
        scale_letterbox = mInputWidth * 1.0f / mImageWidth;
    }

    resize_cols = int(scale_letterbox * mImageWidth);
    resize_rows = int(scale_letterbox * mImageHeight);
    mRealWidth = resize_cols;
    mRealHeight = resize_rows;

    int top = (mInputHeight - resize_rows) / 2;
    int left = (mInputWidth - resize_cols) / 2;
    int right = (mInputWidth - resize_cols + 1) / 2;
    int bottom = (mInputHeight - resize_rows + 1) / 2;

    src = wrapbuffer_fd(drm_mpi_mb_get_fd(mediaFrame), mImageWidth, mImageHeight, RK_FORMAT_BGR_888);
    dst = wrapbuffer_virtualaddr(virtualCache, resize_cols, resize_rows, RK_FORMAT_BGR_888);

    ret = imcheck(src, dst, src_rect, dst_rect, 1);
    if (ret != IM_STATUS_NOERROR) {
        if (!bPrintErr) {
            bPrintErr = true;
            printf("\033[1;31m[%s:%04d] %s execute failed, return:[%d]\033[0m\n", __FILE__, __LINE__, __func__, ret);
        }

        return -2;
    }

    IM_STATUS status = imresize(src, dst);
    if ((status == IM_STATUS_NOERROR) || (status == IM_STATUS_SUCCESS)) {
        if (bEqual) {
            inferMat = cv::Mat(resize_rows, resize_cols, CV_8UC3, virtualCache);
        } else {
            inferMat = cv::Mat(mInputHeight, mInputWidth, CV_8UC3, cv::Scalar(0));
            cv::Mat resizeImage = cv::Mat(resize_rows, resize_cols, CV_8UC3, virtualCache);
            cv::copyMakeBorder(resizeImage, inferMat, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        }

        return 0;
    }

    return -3;
}

int AlgoDetector::processInferImage(cv::Mat rawRGBImage, cv::Mat &inferMat, unsigned char *virtualCache)
{
    if (!virtualCache) {
        return -1;
    }

    bool bEqual = false;
    float scale_letterbox;
    rga_buffer_t src, dst;
    im_rect src_rect, dst_rect;
    int resize_rows, resize_cols;
    static bool bPrintErr = false;
    IM_STATUS ret = IM_STATUS_NOERROR;

    memset(&src, 0x00, sizeof(src));
    memset(&dst, 0x00, sizeof(dst));
    memset(&src_rect, 0x00, sizeof(src_rect));
    memset(&dst_rect, 0x00, sizeof(dst_rect));

    if ((mInputWidth * 1.0f / mImageWidth) != (mInputHeight * 1.0f / mImageHeight)) {
        if ((mInputWidth * 1.0f / mImageWidth) > (mInputHeight * 1.0f / mImageHeight)) {
            scale_letterbox = mInputHeight * 1.0f / mImageHeight;
        } else {
            scale_letterbox = mInputWidth * 1.0f / mImageWidth;
        }
    } else {
        bEqual = true;
        scale_letterbox = mInputWidth * 1.0f / mImageWidth;
    }

    resize_cols = int(scale_letterbox * mImageWidth);
    resize_rows = int(scale_letterbox * mImageHeight);
    mRealWidth = resize_cols;
    mRealHeight = resize_rows;

    int top = (mInputHeight - resize_rows) / 2;
    int left = (mInputWidth - resize_cols) / 2;
    int right = (mInputWidth - resize_cols + 1) / 2;
    int bottom = (mInputHeight - resize_rows + 1) / 2;

    src = wrapbuffer_virtualaddr(rawRGBImage.data, mImageWidth, mImageHeight, RK_FORMAT_BGR_888);
    dst = wrapbuffer_virtualaddr(virtualCache, resize_cols, resize_rows, RK_FORMAT_BGR_888);

    ret = imcheck(src, dst, src_rect, dst_rect, 1);
    if (ret != IM_STATUS_NOERROR) {
        if (!bPrintErr) {
            bPrintErr = true;
            printf("\033[1;31m[%s:%04d] %s execute failed, return:[%d]\033[0m\n", __FILE__, __LINE__, __func__, ret);
        }

        return -2;
    }

    IM_STATUS status = imresize(src, dst);
    if ((status == IM_STATUS_NOERROR) || (status == IM_STATUS_SUCCESS)) {
        if (bEqual) {
            inferMat = cv::Mat(resize_rows, resize_cols, CV_8UC3, virtualCache);
        } else {
            inferMat = cv::Mat(mInputHeight, mInputWidth, CV_8UC3, cv::Scalar(0));
            cv::Mat resizeImage = cv::Mat(resize_rows, resize_cols, CV_8UC3, virtualCache);
            cv::copyMakeBorder(resizeImage, inferMat, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        }

        return 0;
    }

    return -3;
}

API_END_NAMESPACE(Ai)
