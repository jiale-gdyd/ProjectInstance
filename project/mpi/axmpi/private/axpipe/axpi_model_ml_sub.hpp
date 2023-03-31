#pragma once

#include <opencv2/opencv.hpp>

#include "axpi_model_base.hpp"
#include "../utilities/ringbuffer.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
class AxPiModelMLSub : public AxPiModelSingleBase {
protected:
    cv::Mat                                     affine_trans_mat;
    cv::Mat                                     affine_trans_mat_inv;
    SimpleRingBuffer<std::vector<axpi_point_t>> mSimpleRingBuffer;
};

class AxPiModelPoseHrnetSub : public AxPiModelMLSub {
protected:
    int preprocess(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};

class AxPiModelPoseAxpplSub : public AxPiModelPoseHrnetSub {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};

class AxPiModelPoseHrnetAnimalSub : public AxPiModelPoseHrnetSub {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};

class AxPiModelPoseHandSub : public AxPiModelMLSub {
protected:
    int preprocess(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};

class AxPiModelFaceFeatExtractorSub : public AxPiModelMLSub {
protected:
    SimpleRingBuffer<std::vector<float>> mSimpleRingBuffer_FaceFeat;

    void _normalize(float *feature, int feature_len);
    int preprocess(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};

class AxPiModelLicensePlateRecognitionSub : public AxPiModelMLSub {
protected:
    float argmax_idx[21];
    float argmax_data[21];

    int preprocess(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
}
