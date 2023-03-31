#include <fstream>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include "../base/pose.hpp"
#include "axpi_common_api.hpp"
#include "../utilities/log.hpp"
#include "axpi_model_multi_level_model.hpp"

namespace axpi {
static inline void draw_pose_result(cv::Mat &img, axpi_object_t *pObj, std::vector<pose::skeleton> &pairs, int joints_num, int offset_x, int offset_y)
{
    for (int i = 0; i < joints_num; i++) {
        cv::circle(img, cv::Point(pObj->landmark[i].x * img.cols + offset_x, pObj->landmark[i].y * img.rows + offset_y), 4, cv::Scalar(0, 255, 0), cv::FILLED);
    }

    cv::Point pt1;
    cv::Point pt2;
    cv::Scalar color;

    for (auto &element : pairs) {
        switch (element.left_right_neutral) {
            case 0:
                color = cv::Scalar(255, 255, 0, 0);
                break;

            case 1:
                color = cv::Scalar(255, 0, 0, 255);
                break;

            case 2:
                color = cv::Scalar(255, 0, 255, 0);
                break;

            case 3:
                color = cv::Scalar(255, 255, 0, 255);
                break;

            default:
                color = cv::Scalar(255, 255, 255, 255);
        }

        int x1 = (int)(pObj->landmark[element.connection[0]].x * img.cols) + offset_x;
        int y1 = (int)(pObj->landmark[element.connection[0]].y * img.rows) + offset_y;
        int x2 = (int)(pObj->landmark[element.connection[1]].x * img.cols) + offset_x;
        int y2 = (int)(pObj->landmark[element.connection[1]].y * img.rows) + offset_y;

        x1 = std::max(std::min(x1, (img.cols - 1)), 0);
        y1 = std::max(std::min(y1, (img.rows - 1)), 0);
        x2 = std::max(std::min(x2, (img.cols - 1)), 0);
        y2 = std::max(std::min(y2, (img.rows - 1)), 0);

        pt1 = cv::Point(x1, y1);
        pt2 = cv::Point(x2, y2);
        cv::line(img, pt1, pt2, color, 2);
    }
}

void AxPiModelHumanPoseAxppl::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    drawBbox(image, results, fontscale, thickness, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        static std::vector<pose::skeleton> pairs = {
            {15, 13, 0},
            {13, 11, 0},
            {16, 14, 0},
            {14, 12, 0},
            {11, 12, 0},
            {5, 11, 0},
            {6, 12, 0},
            {5, 6, 0},
            {5, 7, 0},
            {6, 8, 0},
            {7, 9, 0},
            {8, 10, 0},
            {1, 2, 0},
            {0, 1, 0},
            {0, 2, 0},
            {1, 3, 0},
            {2, 4, 0},
            {0, 5, 0},
            {0, 6, 0}
        };

        if ((int)results->objects[i].landmark.size() == BODY_LMK_SIZE) {
            draw_pose_result(image, &results->objects[i], pairs, BODY_LMK_SIZE, offset_x, offset_y);
        }
    }
}

int AxPiModelHumanPoseAxppl::inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    int ret = mModel0->inference(pstFrame, crop_resize_box, results);
    if (ret) {
        return ret;
    }

    std::vector<int> idxs;
    for (size_t i = 0; i < results->objects.size(); i++) {
        auto it = std::find(mClassIds.begin(), mClassIds.end(), results->objects[i].label);
        if (it != mClassIds.end()) {
            idxs.push_back(i);
        }
    }

    int count = MIN(idxs.size(), mMaxSubInferCount);
    results->objects.resize(count);

    for (int i = 0; i < count; i++) {
        int idx = idxs[i];
        mModel1->setCurrentIndex(idx);
        ret = mModel1->inference(pstFrame, crop_resize_box, results);
        if (ret) {
            return ret;
        }
    
        if (idx != 0) {
            memcpy(&results->objects[i], &results->objects[idx], sizeof(axpi_object_t));
        }
    }

    return 0;
}

void AxPiModelAnimalPoseHrnet::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    drawBbox(image, results, fontscale, thickness, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        static std::vector<pose::skeleton> pairs = {
            {19, 15, 0},
            {18, 14, 0},
            {17, 13, 0},
            {16, 12, 0},
            {15, 11, 0},
            {14, 10, 0},
            {13, 9, 0},
            {12, 8, 0},
            {11, 6, 0},
            {10, 6, 0},
            {9, 7, 0},
            {8, 7, 0},
            {6, 7, 0},
            {7, 5, 0},
            {5, 4, 0},
            {0, 2, 0},
            {1, 3, 0},
            {0, 1, 0},
            {0, 4, 0},
            {1, 4, 0}
        };

        if ((int)results->objects[i].landmark.size() == ANIMAL_LMK_SIZE) {
            draw_pose_result(image, &results->objects[i], pairs, ANIMAL_LMK_SIZE, offset_x, offset_y);
        }
    }
}

void AxPiModelHandPose::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    drawBbox(image, results, fontscale, thickness, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        static std::vector<pose::skeleton> hand_pairs = {
            {0, 1, 0},
            {1, 2, 0},
            {2, 3, 0},
            {3, 4, 0},
            {0, 5, 1},
            {5, 6, 1},
            {6, 7, 1},
            {7, 8, 1},
            {0, 9, 2},
            {9, 10, 2},
            {10, 11, 2},
            {11, 12, 2},
            {0, 13, 3},
            {13, 14, 3},
            {14, 15, 3},
            {15, 16, 3},
            {0, 17, 4},
            {17, 18, 4},
            {18, 19, 4},
            {19, 20, 4}
        };

        if ((int)results->objects[i].landmark.size() == HAND_LMK_SIZE) {
            draw_pose_result(image, &results->objects[i], hand_pairs, HAND_LMK_SIZE, offset_x, offset_y);
        }
    }
}

int AxPiModelHandPose::exit()
{
    mModel1->exit();
    mModel0->exit();
    axpi_memfree(pstFrame_RGB.phy, pstFrame_RGB.vir);

    return 0;
}

int AxPiModelHandPose::inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (!pstFrame_RGB.vir) {
        memcpy(&pstFrame_RGB, pstFrame, sizeof(axpi_image_t));
        pstFrame_RGB.dtype = AXPI_COLOR_SPACE_RGB;
        axpi_memalloc(&pstFrame_RGB.phy, (void **)&pstFrame_RGB.vir, pstFrame_RGB.size, 0x100, NULL);
    }

    pstFrame_RGB.dtype = AXPI_COLOR_SPACE_BGR;
    axpi_imgproc_csc(pstFrame, &pstFrame_RGB);
    pstFrame_RGB.dtype = AXPI_COLOR_SPACE_RGB;

    int ret = mModel0->inference(&pstFrame_RGB, crop_resize_box, results);
    if (ret) {
        return ret;
    }

    int count = MIN(results->objects.size(), mMaxSubInferCount);
    results->objects.resize(count);
    for (int i = 0; i < count; i++) {
        mModel1->setCurrentIndex(i);
        ret = mModel1->inference(pstFrame, crop_resize_box, results);
        if (ret) {
            return ret;
        }
    }

    return 0;
}

int AxPiModelFaceRecognition::inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (!b_face_database_init) {
        for (size_t i = 0; i < mFaceRegisterIds.size(); i++) {
            auto &faceid = mFaceRegisterIds[i];
            cv::Mat image = cv::imread(faceid.path);
            if (image.empty()) {
                axmpi_error("image:[%s] cannot open, name:[%s] register failed", faceid.path.c_str(), faceid.name.c_str());
                continue;
            }

            axpi_image_t npu_image = {0};
            npu_image.dtype = AXPI_COLOR_SPACE_RGB;
            npu_image.height = image.rows;
            npu_image.width = image.cols;
            npu_image.strideW = npu_image.width;
            npu_image.size = npu_image.width * npu_image.height * 3;
            axpi_memalloc(&npu_image.phy, (void **)&npu_image.vir, npu_image.size, 0x100, "SAMPLE-CV");
            memcpy(npu_image.vir, image.data, npu_image.size);

            int width, height;
            axpi_results_t Results = {0};

            mModel0->getDetRestoreResolution(width, height);
            mModel0->setDetRestoreResolution(npu_image.width, npu_image.height);
            int ret = mModel0->inference(&npu_image, nullptr, &Results);

            mModel0->setDetRestoreResolution(width, height);
            if (ret) {
                axpi_memfree(npu_image.phy, npu_image.vir);
                continue;
            }

            if (Results.objects.size()) {
                mModel1->setCurrentIndex(0);
                ret = mModel1->inference(&npu_image, nullptr, &Results);
                if (ret) {
                    axpi_memfree(npu_image.phy, npu_image.vir);
                    continue;
                }

                faceid.feat.resize(mFaceFeatLength);
                memcpy(faceid.feat.data(), Results.objects[0].faceFeat.data, mFaceFeatLength * sizeof(float));
            }

            axpi_memfree(npu_image.phy, npu_image.vir);
        }

        b_face_database_init = true;
    }

    int ret = mModel0->inference(pstFrame, crop_resize_box, results);
    if (ret) {
        return ret;
    }

    int count = MIN(results->objects.size(), mMaxSubInferCount);
    results->objects.resize(count);
    for (int i = 0; i < count; i++) {
        mModel1->setCurrentIndex(i);
        ret = mModel1->inference(pstFrame, crop_resize_box, results);
        if (ret) {
            axmpi_error("sub model inference failed");
            return ret;
        }

        int maxidx = -1;
        float max_score = 0;
        for (size_t j = 0; j < mFaceRegisterIds.size(); j++) {
            if (mFaceRegisterIds[j].feat.size() != mFaceFeatLength) {
                continue;
            }

            float sim = _calcSimilar((float *)results->objects[i].faceFeat.data, mFaceRegisterIds[j].feat.data(), mFaceFeatLength);
            if ((sim > max_score) && (sim > mFaceRecThreshold)) {
                maxidx = j;
                max_score = sim;
            }
        }

        if (maxidx >= 0) {
            if (max_score >= mFaceRecThreshold) {
                strcpy(results->objects[i].objname, mFaceRegisterIds[maxidx].name.c_str());
            } else {
                strcpy(results->objects[i].objname, "unknown");
            }
        } else {
           strcpy(results->objects[i].objname, "unknown");
        }
    }

    return 0;
}

int AxPiModelVehicleLicenseRecognition::inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    int ret = mModel0->inference(pstFrame, crop_resize_box, results);
    if (ret) {
        return ret;
    }

    int count = MIN(results->objects.size(), mMaxSubInferCount);
    results->objects.resize(count);
    for (int i = 0; i < count; i++) {
        mModel1->setCurrentIndex(i);
        ret = mModel1->inference(pstFrame, crop_resize_box, results);
        if (ret) {
            return ret;
        }
    }

    return 0;
}
}
