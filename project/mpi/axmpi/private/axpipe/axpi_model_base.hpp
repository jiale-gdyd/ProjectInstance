#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

#include "../../axpi.hpp"
#include "axpi_model_runner.hpp"

namespace axpi {
class AxPiModelBase {
protected:
    int   mModelType = MT_UNKNOWN;
    int   mRunnerType = RUNNER_AX620;
    int   mDetBboxRestoreWidth = 1920;
    int   mDetBboxRestoreHeight = 1080;

    int   mFaceFeatLength = 512;
    int   mMaxSubInferCount = 3;
    int   mMaxMaskObjectCount = 8;
    bool  mUseWarpPreprocess = false;

    int   mClassCount = 80;
    float mProbThreshold = 0.4f;
    float mNmsThreshold = 0.45f;

    std::vector<float> mAnchors = {
        12, 16, 19, 36, 40, 28,
        36, 75, 76, 55, 72, 146,
        142, 110, 192, 243, 459, 401
    };

    std::vector<int> mStrides = {8, 16, 32};

    std::vector<std::string> mClassName = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"
    };

    const std::vector<cv::Scalar> mCocoColors = {
        {128, 56, 0, 255},  {128, 226, 255, 0}, {128, 0, 94, 255},  {128, 0, 37, 255},  {128, 0, 255, 94},
        {128, 255, 226, 0}, {128, 0, 18, 255},  {128, 255, 151, 0}, {128, 170, 0, 255}, {128, 0, 255, 56},
        {128, 255, 0, 75},  {128, 0, 75, 255},  {128, 0, 255, 169}, {128, 255, 0, 207}, {128, 75, 255, 0},
        {128, 207, 0, 255}, {128, 37, 0, 255},  {128, 0, 207, 255}, {128, 94, 0, 255},  {128, 0, 255, 113},
        {128, 255, 18, 0},  {128, 255, 0, 56},  {128, 18, 0, 255},  {128, 0, 255, 226}, {128, 170, 255, 0},
        {128, 255, 0, 245}, {128, 151, 255, 0}, {128, 132, 255, 0}, {128, 75, 0, 255},  {128, 151, 0, 255},
        {128, 0, 151, 255}, {128, 132, 0, 255}, {128, 0, 255, 245}, {128, 255, 132, 0}, {128, 226, 0, 255},
        {128, 255, 37, 0},  {128, 207, 255, 0}, {128, 0, 255, 207}, {128, 94, 255, 0},  {128, 0, 226, 255},
        {128, 56, 255, 0},  {128, 255, 94, 0},  {128, 255, 113, 0}, {128, 0, 132, 255}, {128, 255, 0, 132},
        {128, 255, 170, 0}, {128, 255, 0, 188}, {128, 113, 255, 0}, {128, 245, 0, 255}, {128, 113, 0, 255},
        {128, 255, 188, 0}, {128, 0, 113, 255}, {128, 255, 0, 0},   {128, 0, 56, 255},  {128, 255, 0, 113},
        {128, 0, 255, 188}, {128, 255, 0, 94},  {128, 255, 0, 18},  {128, 18, 255, 0},  {128, 0, 255, 132},
        {128, 0, 188, 255}, {128, 0, 245, 255}, {128, 0, 169, 255}, {128, 37, 255, 0},  {128, 255, 0, 151},
        {128, 188, 0, 255}, {128, 0, 255, 37},  {128, 0, 255, 0},   {128, 255, 0, 170}, {128, 255, 0, 37},
        {128, 255, 75, 0},  {128, 0, 0, 255},   {128, 255, 207, 0}, {128, 255, 0, 226}, {128, 255, 245, 0},
        {128, 188, 255, 0}, {128, 0, 255, 18},  {128, 0, 255, 75},  {128, 0, 255, 151}, {128, 255, 56, 0},
        {128, 245, 255, 0}
    };

    typedef struct {
        std::string        name;
        std::string        path;
        std::vector<float> feat;
    } AxPiModelFaceId;

    float                        mFaceRecThreshold = 0.4f;
    std::vector<AxPiModelFaceId> mFaceRegisterIds;

    std::vector<int>             mClassIds;
    int                          mCurrentIdx = 0;
    std::string                  mFpsInfo;

    void drawFps(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y);
    void drawBbox(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y);

    virtual void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) {
        drawBbox(image, results, fontscale, thickness, offset_x, offset_y);
    }

public:
    int getSubInferCount() {
        return mMaxSubInferCount;
    }

    void setSubInferCount(int count) {
        mMaxSubInferCount = count;
    }

    int getMaxMaskObjectCount() {
        return mMaxMaskObjectCount;
    }

    void setMaxMaskObjectCount(int count) {
        mMaxMaskObjectCount = count;
    }

    int getFaceFeatLength() {
        return mFaceFeatLength;
    }

    void setFaceFeatLength(int length) {
        mFaceFeatLength = length;
    }

    void setCurrentIndex(int idx) {
        mCurrentIdx = idx;
    }

    virtual void setDetRestoreResolution(int width, int height) {
        mDetBboxRestoreWidth = width;
        mDetBboxRestoreHeight = height;
    }

    virtual void getDetRestoreResolution(int &width, int &height) {
        width = mDetBboxRestoreWidth;
        height = mDetBboxRestoreHeight;
    }

    int getModelType() {
        return mModelType;
    }

    void setNmsThreshold(float nms_threshold) {
        mNmsThreshold = nms_threshold;
    }

    float getNmsThreshold() {
        return mNmsThreshold;
    }

    void setProbThreshold(float prob_threshold) {
        mProbThreshold = prob_threshold;
    }

    float getProbThreshold() {
        return mProbThreshold;
    }

    void setClassCount(int num_class) {
        mClassCount = num_class;
    }

    int getClassCount() {
        return mClassCount;
    }

    void setAnchors(std::vector<float> &anchors) {
        mAnchors = anchors;
    }

    std::vector<float> getAnchors() {
        return mAnchors;
    }

    void setStrides(std::vector<int> &strides) {
        mStrides = strides;
    }

    std::vector<int> getStrides() {
        return mStrides;
    }

    void setClassNames(std::vector<std::string> &class_namse) {
        mClassName = class_namse;
    }

    std::vector<std::string> getClassNames() {
        return mClassName;
    }

    void setFaceRecognitionThreshold(float face_recognition_threshold) {
        mFaceRecThreshold = face_recognition_threshold;
    }

    float getFaceRecognitionThreshold() {
        return mFaceRecThreshold;
    }

    void setFaceRegisterIds(std::vector<AxPiModelFaceId> &ids) {
        mFaceRegisterIds = ids;
    }

    std::vector<AxPiModelFaceId> getFaceRegisterIds() {
        return mFaceRegisterIds;
    }

    static int getModelType(void *jsonData, std::string &modelType);
    static int getRunnerType(void *jsonData, std::string &runnerType);

    virtual int exit() = 0;
    virtual int init(void *jsonData) = 0;

    virtual int getAlgoWidth() = 0;
    virtual int getAlgoHeight() = 0;
    virtual int getColorSpace() = 0;

    virtual int inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) = 0;
    virtual void draw_results(cv::Mat&canvas, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) {
        drawCustom(canvas, results, fontscale, thickness, offset_x, offset_y);
        drawFps(canvas, results, fontscale, thickness, offset_x, offset_y);
    }
};

class AxPiModelSingleBase : public AxPiModelBase {
protected:
    std::shared_ptr<AxPiRunnerBase> mRunner;
    std::string                     mModelPath;
    axpi_image_t                    mDstFrame = {0};
    bool                            mMalloc = false;

    virtual int preprocess(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results);
    virtual int post_process(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) = 0;

public:
    virtual int exit() override;
    virtual int init(void *jsonData) override;

    int getAlgoWidth() override {
        return mRunner->getAlgoWidth();
    }

    int getAlgoHeight() override {
        return mRunner->getAlgoHeight();
    }

    int getColorSpace() override {
        return mRunner->getColorSpace();
    }

    virtual int inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};

class AxPiModelMultiBase : public AxPiModelBase {
protected:
    std::shared_ptr<AxPiModelBase> mModel0;
    std::shared_ptr<AxPiModelBase> mModel1;

public:
    void setDetRestoreResolution(int width, int height) override {
        mModel0->setDetRestoreResolution(width, height);
    }

    void getDetRestoreResolution(int &width, int &height) override {
        mModel0->getDetRestoreResolution(width, height);
    }

    virtual int exit() override;
    virtual int init(void *jsonData) override;

    int getAlgoWidth() override {
        return mModel0->getAlgoWidth();
    }

    int getAlgoHeight() override {
        return mModel0->getAlgoHeight();
    }

    int getColorSpace() override {
        return mModel0->getColorSpace();
    }
};
}