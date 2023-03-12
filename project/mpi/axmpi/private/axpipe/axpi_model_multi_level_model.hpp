#pragma once

#include <vector>

#include "axpi_model_base.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
class AxPiModelHumanPoseAxppl : public AxPiModelMultiBase {
protected:
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;

public:
    int inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_MLM_HUMAN_POSE_AXPPL, AxPiModelHumanPoseAxppl)

class AxPiModelHumanPoseHrnet : public AxPiModelHumanPoseAxppl {

};
REGISTER(MT_MLM_HUMAN_POSE_HRNET, AxPiModelHumanPoseHrnet)

class AxPiModelAnimalPoseHrnet : public AxPiModelHumanPoseAxppl {
protected:
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};
REGISTER(MT_MLM_ANIMAL_POSE_HRNET, AxPiModelAnimalPoseHrnet)

class AxPiModelHandPose : public AxPiModelMultiBase {
protected:
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;

    axpi_image_t pstFrame_RGB = {0};

public:
    int exit() override;
    int inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_MLM_HAND_POSE, AxPiModelHandPose)

class AxPiModelFaceRecognition : public AxPiModelMultiBase {
protected:
    bool b_face_database_init = false;

    double _calcSimilar(float *feature1, float *feature2, int feature_len) {
        double sim = 0.0;
        for (int i = 0; i < feature_len; i++) {
            sim += feature1[i] * feature2[i];
        }

        sim = sim < 0 ? 0 : sim > 1 ? 1 : sim;
        return sim;
    }

public:
    int inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_MLM_FACE_RECOGNITION, AxPiModelFaceRecognition)

class AxPiModelVehicleLicenseRecognition : public AxPiModelMultiBase {
public:
    int inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_MLM_VEHICLE_LICENSE_RECOGNITION, AxPiModelVehicleLicenseRecognition)
}
