#include "../base/pose.hpp"
#include "post_process.hpp"

namespace axpi {
void axjoint_post_process_pose(axjoint_models_t *handler, axpi_object_t *object)
{
    axjoint_attr_t *pJointAttr = &handler->minorModel.jointAttr;

    int resize_rows;
    int resize_cols;
    float scale_letterbox;
    int src_rows = object->bbox.h;
    int src_cols = object->bbox.w;
    int letterbox_cols = pJointAttr->width;
    int letterbox_rows = pJointAttr->height;

    if ((letterbox_rows * 1.0 / src_rows) < (letterbox_cols * 1.0 / src_cols)) {
        scale_letterbox = letterbox_rows * 1.0 / src_rows;
    } else {
        scale_letterbox = letterbox_cols * 1.0 / src_cols;
    }

    resize_cols = int(scale_letterbox * src_cols);
    resize_rows = int(scale_letterbox * src_rows);

    int tmp_h = (letterbox_rows - resize_rows) / 2;
    int tmp_w = (letterbox_cols - resize_cols) / 2;

    float ratio_x = (float)src_rows / resize_rows;
    float ratio_y = (float)src_cols / resize_cols;

    pose::ai_body_parts_s ai_point_result;
    pose::ai_hand_parts_s ai_hand_point_result;

    switch (handler->modelTypeMain) {
        case MT_MLM_HUMAN_POSE_AXPPL: {
            auto ptr = (float *)pJointAttr->outputs[0].pVirAddr;
            auto ptr_index = (float *)pJointAttr->outputs[1].pVirAddr;
            pose::ppl_pose_post_process(ptr, ptr_index, ai_point_result, BODY_LMK_SIZE);

            for (size_t i = 0; i < BODY_LMK_SIZE; i++) {
                axpi_point_t point;
                point.x = ai_point_result.keypoints[i].x;
                point.y = ai_point_result.keypoints[i].y;
                object->landmark.push_back(point);
            }
        }
        break;

        case MT_MLM_HUMAN_POSE_HRNET: {
            auto ptr = (float *)pJointAttr->outputs[0].pVirAddr;
            pose::hrnet_post_process(ptr, ai_point_result, BODY_LMK_SIZE, pJointAttr->height, pJointAttr->width);

            for (size_t i = 0; i < BODY_LMK_SIZE; i++) {
                axpi_point_t point;
                point.x = ai_point_result.keypoints[i].x;
                point.y = ai_point_result.keypoints[i].y;
                object->landmark.push_back(point);
            }
        }
        break;

        case MT_MLM_ANIMAL_POSE_HRNET: {
            auto ptr = (float *)pJointAttr->outputs[0].pVirAddr;
            pose::hrnet_post_process(ptr, ai_point_result, ANIMAL_LMK_SIZE, pJointAttr->height, pJointAttr->width);

            for (size_t i = 0; i < ANIMAL_LMK_SIZE; i++) {
                axpi_point_t point;
                point.x = (ai_point_result.keypoints[i].x - tmp_w) * ratio_x + object->bbox.x;
                point.y = (ai_point_result.keypoints[i].y - tmp_h) * ratio_y + object->bbox.y;
                object->landmark.push_back(point);
            }
        }
        break;

        case MT_MLM_HAND_POSE: {
            auto &info_point = pJointAttr->outputs[0];
            auto &info_score = pJointAttr->outputs[1];
            auto point_ptr = (float *)info_point.pVirAddr;
            auto score_ptr = (float *)info_score.pVirAddr;

            pose::post_process_hand(point_ptr, score_ptr, ai_hand_point_result, HAND_LMK_SIZE, pJointAttr->height, pJointAttr->width);

            for (size_t i = 0; i < HAND_LMK_SIZE; i++) {
                axpi_point_t point;
                point.x = ai_hand_point_result.keypoints[i].x;
                point.y = ai_hand_point_result.keypoints[i].y;
                object->landmark.push_back(point);
            }
        }
        break;

        default:
            break;
    }
}
}
