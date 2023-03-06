#ifndef NPU_AXNPU_MODELBASE_HPP
#define NPU_AXNPU_MODELBASE_HPP

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

#include "axapi.h"
#include "runnerBase.hpp"

API_BEGIN_NAMESPACE(Ai)

class modelBase {
public:
    virtual int init(std::string model) = 0;
    virtual void deinit() = 0;

    virtual int getColorSpace() = 0;
    virtual int getModelWidth() = 0;
    virtual int getModelHeight() = 0;

    virtual int forward(axframe_t *pstFrame, axbbox_t *cropResizeBbox, axres_t *results) = 0;

public:
    int getModelType() {
        return mModelType;
    }

    void setModelType(int modelType) {
        mModelType = modelType;
    }

    int getClassCount() {
        return mClassCount;
    }

    void setClassCount(int classCount) {
        mClassCount = classCount;
    }

    float getNmsThreshold() {
        return mNMSThreahold;
    }

    void setNmsThreshold(float nmsThreshold) {
        mNMSThreahold = nmsThreshold;
    }

    float getConfThreshold() {
        return mConfThreahold;
    }

    void setConfThreahold(float confThreahold) {
        mConfThreahold = confThreahold;
    }

    virtual void getDetRestoreResolution(int &width, int &height) {
        width = mDetRestoreWidth;
        height = mDetRestoreHeight;
    }

    virtual void setDetRestoreResolution(int width, int height) {
        mDetRestoreWidth = width;
        mDetRestoreHeight = height;
    }

    virtual void draw_results(cv::Mat &canvas, axres_t *results, float fontscale, int thickness, int offset_x, int offset_y) {

    }

public:
    int   mModelType;
    int   mClassCount;
    float mNMSThreahold;
    float mConfThreahold;

    int   mDetRestoreWidth;
    int   mDetRestoreHeight;
};

class modelSingleBase : public modelBase {
public:
    virtual int init(std::string model) override;
    virtual void deinit() override;
    
    int getColorSpace() override {
        return mRunner->getColorSpace();
    }

    int getModelWidth() override {
        return mRunner->getModelWidth();
    }

    int getModelHeight() override {
        return mRunner->getModelHeight();
    }

    virtual int forward(axframe_t *pstFrame, axbbox_t *cropResizeBbox, axres_t *results) override;

protected:
    std::string                   mModel;
    std::shared_ptr<AxRunnerBase> mRunner;

    bool                          bMalloc = false;
    axframe_t                     mDstFrame = {0};

    virtual int preprocess(axframe_t *srcFrame, axbbox_t *cropResizeBbox, axres_t *result);
    virtual int postprocess(axframe_t *srcFrame, axbbox_t *cropResizeBbox, axres_t *result) = 0;
};

API_END_NAMESPACE(Ai)

#endif
