#include <nlohmann/json.hpp>

#include "../utilities/log.hpp"
#include "axpi_model_detect.hpp"

namespace axpi {
#define ANCHOR_SIZE_PER_STRIDE 6

int AxPiModelYolov5::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != (nOutputSize * ANCHOR_SIZE_PER_STRIDE)) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov5(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid, mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objsSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objsSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov5Seg::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != ((nOutputSize - 1) * ANCHOR_SIZE_PER_STRIDE)) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", (nOutputSize - 1) * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov5_seg(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid);
    }

    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;

    auto &output = pOutputsInfo[3];
    auto ptr = (float *)output.pVirAddr;

    detect::get_out_bbox_mask(proposals, objects, MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);

    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(MAX_YOLOV5_MASK_OBJ_COUNT * RINGBUFFER_CACHE_COUNT);

    size_t objsSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objsSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;
        axobj.label = obj.label;
        axobj.prob = obj.prob;

        results->objects[i].bHasMask = !obj.mask.empty();

        if (results->objects[i].bHasMask) {
            cv::Mat &mask = mSimpleRingBuffer.next();
            mask = obj.mask;
            axobj.yolov5Mask.w = mask.cols;
            axobj.yolov5Mask.h = mask.rows;
            axobj.yolov5Mask.data = mask.data;
        }

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

void AxPiModelYolov5Seg::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    drawBbox(image, results, fontscale, thickness, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        cv::Rect rect(results->objects[i].bbox.x * image.cols + offset_x,
            results->objects[i].bbox.y * image.rows + offset_y,
            results->objects[i].bbox.w * image.cols, results->objects[i].bbox.h * image.rows);

        if (results->objects[i].bHasMask && results->objects[i].yolov5Mask.data) {
            cv::Mat mask(results->objects[i].yolov5Mask.h, results->objects[i].yolov5Mask.w, CV_8U, results->objects[i].yolov5Mask.data);
            if (!mask.empty()) {
                cv::Mat mask_target;
                cv::resize(mask, mask_target, cv::Size(results->objects[i].bbox.w * image.cols, results->objects[i].bbox.h * image.rows), 0, 0, cv::INTER_NEAREST);

                if (results->objects[i].label < (int)mCocoColors.size()) {
                    image(rect).setTo(mCocoColors[results->objects[i].label], mask_target);
                } else {
                    image(rect).setTo(cv::Scalar(128, 128, 128, 128), mask_target);
                }
            }
        }
    }
}

int AxPiModelYolov5Face::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT * MAX_FACE_BBOX_COUNT);
    }

    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != (nOutputSize * ANCHOR_SIZE_PER_STRIDE)) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov5_face(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid, FACE_LMK_SIZE);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objsSize = MIN(objects.size(), MAX_FACE_BBOX_COUNT);
    for (size_t i = 0; i < objsSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
        points.resize(FACE_LMK_SIZE);
        // results->objects[i].landmark = points;

        std::vector<axpi_point_t> landmark;
        for (size_t j = 0; j < FACE_LMK_SIZE; j++) {
            axpi_point_t point;
            point.x = obj.landmark[j].x;
            point.y = obj.landmark[j].y;
            landmark.push_back(point);
        }
        axobj.landmark = landmark;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

void AxPiModelYolov5Face::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    drawBbox(image, results, fontscale, thickness, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        for (size_t j = 0; j < results->objects[i].landmark.size(); j++) {
            cv::Point p(results->objects[i].landmark[j].x * image.cols + offset_x, results->objects[i].landmark[j].y * image.rows + offset_y);
            cv::circle(image, p, 1, cv::Scalar(255, 0, 0, 255), 2);
        }
    }
}

int AxPiModelYolov5LisencePlate::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT * MAX_BBOX_COUNT);
    }

    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov5_face(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid, PLATE_LMK_SIZE);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
        points.resize(PLATE_LMK_SIZE);
        // results->objects[i].landmark = point;

        axobj.bHasBoxVertices = true;

        std::vector<axpi_point_t> landmarks;
        std::vector<axpi_point_t> bbox_vertices;

        for (size_t j = 0; j < PLATE_LMK_SIZE; j++) {
            axpi_point_t point;

            point.x = obj.landmark[j].x;
            point.y = obj.landmark[j].y;
            landmarks.push_back(point);
            bbox_vertices.push_back(point);
        }
        axobj.landmark = landmarks;

        std::vector<axpi_point_t> pppp(4);
        memcpy(pppp.data(), &results->objects[i].bbox_vertices[0], 4 * sizeof(axpi_point_t));
        std::sort(pppp.begin(), pppp.end(), [](axpi_point_t &a, axpi_point_t &b) {
            return a.x < b.x;
        });
    
        if (pppp[0].y < pppp[1].y) {
            bbox_vertices[0] = pppp[0];
            bbox_vertices[3] = pppp[1];
        } else {
            bbox_vertices[0] = pppp[1];
            bbox_vertices[3] = pppp[0];
        }

        if (pppp[2].y < pppp[3].y) {
            bbox_vertices[1] = pppp[2];
            bbox_vertices[2] = pppp[3];
        } else {
            bbox_vertices[1] = pppp[3];
            bbox_vertices[2] = pppp[2];
        }
        axobj.bbox_vertices = bbox_vertices;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov6::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov6(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov7::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov7(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data() + i * ANCHOR_SIZE_PER_STRIDE, mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov7Face::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT * MAX_FACE_BBOX_COUNT);
    }

    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov7_face(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_FACE_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
        points.resize(FACE_LMK_SIZE);
        // results->objects[i].landmark = points;

        std::vector<axpi_point_t> landmark;
        for (size_t j = 0; j < FACE_LMK_SIZE; j++) {
            axpi_point_t point;
            point.x = obj.landmark[j].x;
            point.y = obj.landmark[j].y;
            landmark.push_back(point);
        }
        axobj.landmark = landmark;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov7FacePlamHand::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::PalmObject> objects;
    std::vector<detect::PalmObject> proposals;
    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != (nOutputSize * ANCHOR_SIZE_PER_STRIDE)) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov7_palm(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid);
    }

    detect::get_out_bbox_palm(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::PalmObject &a, detect::PalmObject &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::PalmObject &obj = objects[i];

        axobj.label = 0;
        axobj.prob = obj.prob;
        axobj.bHasBoxVertices = true;
        axobj.bbox.x = obj.rect.x * mDetBboxRestoreWidth;
        axobj.bbox.y = obj.rect.y * mDetBboxRestoreHeight;
        axobj.bbox.w = obj.rect.width * mDetBboxRestoreWidth;
        axobj.bbox.h = obj.rect.height * mDetBboxRestoreHeight;

        std::vector<axpi_point_t> bbox_vertices;
        for (size_t j = 0; j < 4; j++) {
            axpi_point_t point;
            point.x = obj.vertices[j].x;
            point.y = obj.vertices[j].y;
            bbox_vertices.push_back(point);
        }
        axobj.bbox_vertices = bbox_vertices;

        strcpy(axobj.objname, "hand");
        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelPlamHand::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    static const int strides[2] = {8, 16};
    static const int map_size[2] = {24, 12};
    static const int anchor_size[2] = {2, 6};
    static const float anchor_offset[2] = {0.5f, 0.5f};

    std::vector<detect::PalmObject> objects;
    std::vector<detect::PalmObject> proposals;

    auto &bboxes_info = mRunner->getSpecOutput(0);
    auto bboxes_ptr = (float *)bboxes_info.pVirAddr;

    auto &scores_info = mRunner->getSpecOutput(1);
    auto scores_ptr = (float *)scores_info.pVirAddr;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    detect::generate_proposals_palm(proposals, mProbThreshold, getAlgoWidth(), getAlgoHeight(), scores_ptr, bboxes_ptr, 2, strides, anchor_size, anchor_offset, map_size, prob_threshold_unsigmoid);

    detect::get_out_bbox_palm(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);

    std::sort(objects.begin(), objects.end(), [&](detect::PalmObject &a, detect::PalmObject &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::PalmObject &obj = objects[i];

        axobj.label = 0;
        axobj.prob = obj.prob;
        axobj.bHasBoxVertices = true;
        axobj.bbox.x = obj.rect.x * mDetBboxRestoreWidth;
        axobj.bbox.y = obj.rect.y * mDetBboxRestoreHeight;
        axobj.bbox.w = obj.rect.width * mDetBboxRestoreWidth;
        axobj.bbox.h = obj.rect.height * mDetBboxRestoreHeight;

        std::vector<axpi_point_t> bbox_vertices;
        for (size_t j = 0; j < 4; j++) {
            axpi_point_t point;
            point.x = obj.vertices[j].x;
            point.y = obj.vertices[j].y;
            bbox_vertices.push_back(point);
        }
        axobj.bbox_vertices = bbox_vertices;

        strcpy(axobj.objname, "hand");
        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolox::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolox(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYoloxPPL::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    for (uint32_t i = 0; i < nOutputSize; ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;

        int wxc = output.shape[2] * output.shape[3];
        std::vector<detect::GridAndStride> grid_stride;
        static std::vector<std::vector<int>> stride_ppl = {{8}, {16}, {32}};

        detect::generate_grids_and_stride(getAlgoWidth(), getAlgoHeight(), stride_ppl[i], grid_stride);
        detect::generate_yolox_proposals(grid_stride, ptr, mProbThreshold, proposals, wxc, mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYoloPV2::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if ((int)mAnchors.size() != ((nOutputSize - 2) * ANCHOR_SIZE_PER_STRIDE)) {
        axmpi_error("anchors size failed, should be:[%d], got:[%d]", (nOutputSize - 2) * ANCHOR_SIZE_PER_STRIDE, (int)mAnchors.size());
        return -1;
    }

    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i + 2];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_yolov5(mStrides[i], ptr, mProbThreshold, proposals, getAlgoWidth(), getAlgoHeight(), mAnchors.data(), prob_threshold_unsigmoid, 80);
    }

    if (mSimpleRingBuffer_seg.size() == 0) {
        mSimpleRingBuffer_seg.resize(RINGBUFFER_CACHE_COUNT);
        mSimpleRingBuffer_ll.resize(RINGBUFFER_CACHE_COUNT);
    }

    auto &da_info = mRunner->getSpecOutput(0);
    auto da_ptr = (float *)da_info.pVirAddr;

    auto &ll_info = mRunner->getSpecOutput(1);
    auto ll_ptr = (float *)ll_info.pVirAddr;

    cv::Mat &da_seg_mask = mSimpleRingBuffer_seg.next();
    cv::Mat &ll_seg_mask = mSimpleRingBuffer_ll.next();

    detect::get_out_bbox_yolopv2(proposals, objects, da_ptr, ll_ptr, ll_seg_mask, da_seg_mask, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        results->objects[i].label = 0;
        strcpy(axobj.objname, "car");
        results->objects.push_back(axobj);
    }

    results->bYolopv2Mask = true;
    results->yolopv2seg.h = da_seg_mask.rows;
    results->yolopv2seg.w = da_seg_mask.cols;
    results->yolopv2seg.data = da_seg_mask.data;
    results->yolopv2ll.h = ll_seg_mask.rows;
    results->yolopv2ll.w = ll_seg_mask.cols;
    results->yolopv2ll.data = ll_seg_mask.data;

    return 0;
}

void AxPiModelYoloPV2::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (results->bYolopv2Mask && results->yolopv2ll.data && results->yolopv2seg.data) {
        if (base_canvas.empty() || ((base_canvas.rows * base_canvas.cols) < (image.rows * image.cols))) {
            base_canvas = cv::Mat(image.rows, image.cols, CV_8UC1);
        }
        cv::Mat tmp(image.rows, image.cols, CV_8UC1, base_canvas.data);

        cv::Mat seg_mask(results->yolopv2seg.h, results->yolopv2seg.w, CV_8UC1, results->yolopv2seg.data);
        cv::resize(seg_mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 0, 128), tmp);

        cv::Mat ll_mask(results->yolopv2ll.h, results->yolopv2ll.w, CV_8UC1, results->yolopv2ll.data);
        cv::resize(ll_mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 128, 0), tmp);
    }

    drawBbox(image, results, fontscale, thickness, offset_x, offset_y);
}

int AxPiModelYoloFastBody::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    if (!bInit) {
        bInit = true;
        yolo.init(yolo::YOLO_FASTEST_BODY, mNmsThreshold, mProbThreshold, 1);
        yolo_inputs.resize(nOutputSize);
        yolo_outputs.resize(1);
        output_buf.resize(1000 * 6, 0);
    }

    for (uint32_t i = 0; i < nOutputSize; ++i) {
        auto &output = pOutputsInfo[i];

        auto ptr = (float *)output.pVirAddr;

        yolo_inputs[i].batch = output.shape[0];
        yolo_inputs[i].h = output.shape[1];
        yolo_inputs[i].w = output.shape[2];
        yolo_inputs[i].c = output.shape[3];
        yolo_inputs[i].data = ptr;
    }

    yolo_outputs[0].batch = 1;
    yolo_outputs[0].c = 1;
    yolo_outputs[0].h = 1000;
    yolo_outputs[0].w = 6;
    yolo_outputs[0].data = output_buf.data();

    yolo.forward_nhwc(yolo_inputs, yolo_outputs);

    std::vector<detect::Object> objects(yolo_outputs[0].h);

    int resize_rows;
    int resize_cols;
    float scale_letterbox;

    int letterbox_cols = getAlgoWidth();
    int letterbox_rows = getAlgoHeight();
    int src_cols = mDetBboxRestoreWidth;
    int src_rows = mDetBboxRestoreHeight;

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

    for (int i = 0; i < yolo_outputs[0].h; i++) {
        float *data_row = yolo_outputs[0].row((int)i);
        detect::Object &object = objects[i];

        object.rect.x = data_row[2] * (float)getAlgoWidth();
        object.rect.y = data_row[3] * (float)getAlgoHeight();
        object.rect.width = (data_row[4] - data_row[2]) * (float)getAlgoWidth();
        object.rect.height = (data_row[5] - data_row[3]) * (float)getAlgoHeight();
        object.label = (int)data_row[0];
        object.prob = data_row[1];

        float x0 = (objects[i].rect.x);
        float y0 = (objects[i].rect.y);
        float x1 = (objects[i].rect.x + objects[i].rect.width);
        float y1 = (objects[i].rect.y + objects[i].rect.height);

        x0 = (x0 - tmp_w) * ratio_x;
        y0 = (y0 - tmp_h) * ratio_y;
        x1 = (x1 - tmp_w) * ratio_x;
        y1 = (y1 - tmp_h) * ratio_y;

        x0 = std::max(std::min(x0, (float)(src_cols - 1)), 0.f);
        y0 = std::max(std::min(y0, (float)(src_rows - 1)), 0.f);
        x1 = std::max(std::min(x1, (float)(src_cols - 1)), 0.f);
        y1 = std::max(std::min(y1, (float)(src_rows - 1)), 0.f);

        objects[i].rect.x = x0;
        objects[i].rect.y = y0;
        objects[i].rect.width = x1 - x0;
        objects[i].rect.height = y1 - y0;
    }

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        axobj.label = 0;
        strcpy(axobj.objname, "person");

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelNanoDetect::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        detect::generate_proposals_nanodet(ptr, mStrides[i], getAlgoWidth(), getAlgoHeight(), mProbThreshold, proposals, mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelSCRFD::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT * MAX_FACE_BBOX_COUNT);
    }

    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    int nOutputSize = mRunner->getOutputCount();
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    std::map<std::string, float *> output_map;
    for (uint32_t i = 0; i < nOutputSize; i++) {
        output_map[pOutputsInfo[i].name] = (float *)pOutputsInfo[i].pVirAddr;
    }

    static const char *kps_pred_name[] = { "kps_8", "kps_16", "kps_32"};
    static const char *bbox_pred_name[] = { "bbox_8", "bbox_16", "bbox_32"};
    static const char *score_pred_name[] = { "score_8", "score_16", "score_32"};

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (int stride_index = 0; stride_index < mStrides.size(); stride_index++) {
        float *score_pred = output_map[score_pred_name[stride_index]];
        float *bbox_pred = output_map[bbox_pred_name[stride_index]];
        float *kps_pred = output_map[kps_pred_name[stride_index]];

        detect::generate_proposals_scrfd(mStrides[stride_index], score_pred, bbox_pred, kps_pred, prob_threshold_unsigmoid, proposals, getAlgoHeight(), getAlgoWidth());
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_FACE_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
        points.resize(FACE_LMK_SIZE);
        // results->objects[i].landmark = points;

        std::vector<axpi_point_t> landmark;
        for (size_t j = 0; j < FACE_LMK_SIZE; j++) {
            axpi_point_t point;
            point.x = obj.landmark[j].x;
            point.y = obj.landmark[j].y;
            landmark.push_back(point);
        }
        axobj.landmark = landmark;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov8::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &dfl_info = pOutputsInfo[i];
        auto dfl_ptr = (float *)dfl_info.pVirAddr;

        auto &cls_info = pOutputsInfo[i + 3];
        auto cls_ptr = (float *)cls_info.pVirAddr;

        auto &cls_idx_info = pOutputsInfo[i + 6];
        auto cls_idx_ptr = (float *)cls_idx_info.pVirAddr;

        detect::generate_proposals_yolov8(mStrides[i], dfl_ptr, cls_ptr, cls_idx_ptr, prob_threshold_unsigmoid, proposals, getAlgoWidth(), getAlgoHeight(), mClassCount);
    }

    detect::get_out_bbox(proposals, objects, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);
    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++){
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}

int AxPiModelYolov8Seg::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / mProbThreshold) - 1.0f);
    for (uint32_t i = 0; i < mStrides.size(); ++i) {
        auto &dfl_info = pOutputsInfo[i];
        auto dfl_ptr = (float *)dfl_info.pVirAddr;

        auto &cls_info = pOutputsInfo[i + 3];
        auto cls_ptr = (float *)cls_info.pVirAddr;

        auto &cls_idx_info = pOutputsInfo[i + 6];
        auto cls_idx_ptr = (float *)cls_idx_info.pVirAddr;

        detect::generate_proposals_yolov8_seg(mStrides[i], dfl_ptr, cls_ptr, cls_idx_ptr, prob_threshold_unsigmoid, proposals, getAlgoWidth(), getAlgoHeight());
    }

    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;

    auto &output = pOutputsInfo[9];
    auto ptr = (float *)output.pVirAddr;

    detect::get_out_bbox_mask(proposals, objects, MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, mNmsThreshold, getAlgoHeight(), getAlgoWidth(), mDetBboxRestoreHeight, mDetBboxRestoreWidth);

    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(MAX_YOLOV5_MASK_OBJ_COUNT * RINGBUFFER_CACHE_COUNT);

    size_t objSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;

        axobj.bHasMask = !obj.mask.empty();

        if (axobj.bHasMask) {
            cv::Mat &mask = mSimpleRingBuffer.next();
            mask = obj.mask;
            axobj.yolov5Mask.w = mask.cols;
            axobj.yolov5Mask.h = mask.rows;
            axobj.yolov5Mask.data = mask.data;
        }

        if (obj.label < (int)mClassName.size()) {
            strcpy(axobj.objname, mClassName[obj.label].c_str());
        } else {
            strcpy(axobj.objname, "unknown");
        }

        results->objects.push_back(axobj);
    }

    return 0;
}
}
