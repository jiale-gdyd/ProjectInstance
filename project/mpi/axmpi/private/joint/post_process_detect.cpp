#include <fstream>
#include <joint.h>
#include <nlohmann/json.hpp>

#include "../base/yolo.hpp"
#include "post_process.hpp"
#include "../utilities/log.hpp"
#include "../base/detection.hpp"
#include "post_process_detect.hpp"
#include "../utilities/ringbuffer.hpp"

namespace axpi {
static int g_classCount = 80;
static float g_probThreshold = 0.4f;
static float g_nmsThreshold = 0.45f;

static std::vector<float> g_anchors = {
    12, 16, 19, 36, 40, 28,
    36, 75, 76, 55, 72, 146,
    142, 110, 192, 243, 459, 401
};

static std::vector<std::string> g_classNames = {
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

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, T *val)
{
    if (jsondata.contains(key)) {
        *val = jsondata[key];
    }
}

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, std::vector<T> *val)
{
    if (jsondata.contains(key)) {
        std::vector<T> tmp = jsondata[key];
        *val = tmp;
    }
}

static int get_model_type(std::string confJsonFile)
{
    std::ifstream f(confJsonFile);
    if (!f.is_open()) {
        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);
    int mt = -1;
    update_val(jsondata, "model_type", &mt);

    return mt;
}

int axjoint_parse_param(std::string confJsonFile)
{
    std::ifstream f(confJsonFile);
    if (f.fail()) {
        axmpi_error("%s doesn`t exist, generate it by default param", confJsonFile.c_str());

        nlohmann::json json_data;
        json_data["model_type"] = MT_DET_YOLOV5;
        json_data["prob_threshold"] = g_probThreshold;
        json_data["nms_threshold"] = g_nmsThreshold;
        json_data["class_count"] = g_classCount;
        json_data["anchors"] = g_anchors;
        json_data["class_names"] = g_classNames;

        std::string json_ctx = json_data.dump(4);
        std::ofstream of(confJsonFile);
        of << json_ctx;
        of.close();

        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);

    update_val(jsondata, "prob_threshold", &g_probThreshold);
    update_val(jsondata, "nms_threshold", &g_nmsThreshold);
    update_val(jsondata, "class_count", &g_classCount);
    update_val(jsondata, "anchors", &g_anchors);
    update_val(jsondata, "class_names", &g_classNames);

    if (g_anchors.size() != 18) {
        axmpi_error("anchors size must be 18");
        return -1;
    }

    if (g_classCount != g_classNames.size()) {
        axmpi_error("classCount:[%d] != classNames size:[%d]", g_classCount, g_classNames.size());
        return -1;
    }

    return 0;
}

int axjoint_set_param(void *jsonData)
{
    auto jsondata = *((nlohmann::json *)jsonData);
    update_val(jsondata, "prob_threshold", &g_probThreshold);
    update_val(jsondata, "nms_threshold", &g_nmsThreshold);
    update_val(jsondata, "class_count", &g_classCount);
    update_val(jsondata, "anchors", &g_anchors);
    update_val(jsondata, "class_names", &g_classNames);

    if (g_anchors.size() != 18) {
        axmpi_error("anchors size must be 18");
        return -1;
    }

    if (g_classCount != g_classNames.size()) {
        axmpi_error("classCount:[%d] != classNames size:[%d]", g_classCount, g_classNames.size());
        return -1;
    }

    return 0;
}

void axjoint_post_process_detection(axpi_results_t *results, axjoint_models_t *handler)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;

    AX_U32 nOutputSize = handler->majorModel.jointAttr.outSize;
    AX_JOINT_IO_BUFFER_T *pOutputs = handler->majorModel.jointAttr.outputs;
    AX_JOINT_IOMETA_T *pOutputsInfo = handler->majorModel.jointAttr.outInfo;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / g_probThreshold) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i) {
        auto &info = pOutputs[i];
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        switch (handler->majorModel.modelType) {
            case MT_DET_YOLOV5:
                detect::generate_proposals_yolov5(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid, g_classCount);
                break;

            case MT_DET_YOLOV5_FACE:
                detect::generate_proposals_yolov5_face(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid, FACE_LMK_SIZE);
                break;

            case MT_DET_YOLOV5_LICENSE_PLATE:
                detect::generate_proposals_yolov5_face(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid, PLATE_LMK_SIZE);
                break;

            case MT_DET_YOLOV6:
                detect::generate_proposals_yolov6(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_classCount);
                break;

            case MT_DET_YOLOV7:
                detect::generate_proposals_yolov7(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data() + i * 6, g_classCount);
                break;

            case MT_DET_YOLOX:
                detect::generate_proposals_yolox(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_classCount);
                break;

            case MT_DET_YOLOV7_FACE:
                detect::generate_proposals_yolov7_face(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid);
                break;

            case MT_DET_NANODET: {
                static const int DEFAULT_STRIDES[] = {32, 16, 8};
                detect::generate_proposals_nanodet(ptr, DEFAULT_STRIDES[i], handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_probThreshold, proposals, g_classCount);
            }
            break;

            case MT_DET_YOLOX_PPL: {
                std::vector<detect::GridAndStride> grid_stride;
                int wxc = output.pShape[2] * output.pShape[3];
                static std::vector<std::vector<int>> stride_ppl = {{8}, {16}, {32}};

                detect::generate_grids_and_stride(handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, stride_ppl[i], grid_stride);
                detect::generate_yolox_proposals(grid_stride, ptr, g_probThreshold, proposals, wxc, g_classCount);
            }
            break;

            default:
                break;
        }
    }

    detect::get_out_bbox(proposals, objects, g_nmsThreshold, handler->majorModel.jointAttr.height, handler->majorModel.jointAttr.width, handler->restoreHeight, handler->restoreWidth);
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

        if ((handler->majorModel.modelType == MT_DET_YOLOV5_FACE) || (handler->majorModel.modelType == MT_DET_YOLOV7_FACE)) {
            std::vector<axpi_point_t> landmark;
            for (size_t j = 0; j < FACE_LMK_SIZE; j++) {
                axpi_point_t point;
                point.x = obj.landmark[j].x;
                point.y = obj.landmark[j].y;
                landmark.push_back(point);
            }

            axobj.landmark = landmark;
        } else if (handler->majorModel.modelType == MT_DET_YOLOV5_LICENSE_PLATE) {
            axobj.bHasBoxVertices = true;

            std::vector<axpi_point_t> landmark;
            std::vector<axpi_point_t> bbox_vertices(4);

            for (size_t j = 0; j < PLATE_LMK_SIZE; j++) {
                axpi_point_t point;
                point.x = obj.landmark[j].x;
                point.y = obj.landmark[j].y;
                landmark.push_back(point);

                point.x = results->objects[i].landmark[j].x;
                point.y = results->objects[i].landmark[j].y;
                bbox_vertices.push_back(point);
            }

            axobj.landmark = landmark;

            std::vector<axpi_point_t> pppp(4);
            memcpy(pppp.data(), bbox_vertices.data(), 4 * sizeof(axpi_point_t));
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
        }

        if (obj.label < g_classNames.size()) {
            axobj.objname = g_classNames[obj.label];
        } else {
            axobj.objname = "unknown";
        }

        results->objects.push_back(axobj);
    }
}

void axjoint_post_process_yolov5_seg(axpi_results_t *results, axjoint_models_t *handler)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    AX_U32 nOutputSize = handler->majorModel.jointAttr.outSize;
    AX_JOINT_IO_BUFFER_T *pOutputs = handler->majorModel.jointAttr.outputs;
    AX_JOINT_IOMETA_T *pOutputsInfo = handler->majorModel.jointAttr.outInfo;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / g_probThreshold) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize - 1; ++i) {
        auto &info = pOutputs[i];
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        detect::generate_proposals_yolov5_seg(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid);
    }

    auto &info = pOutputs[3];
    auto &output = pOutputsInfo[3];
    auto ptr = (float *)info.pVirAddr;
    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;

    detect::get_out_bbox_mask(proposals, objects, MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, g_nmsThreshold,
        handler->majorModel.jointAttr.height, handler->majorModel.jointAttr.width, handler->restoreHeight, handler->restoreWidth);

    std::sort(objects.begin(), objects.end(), [&](detect::Object &a, detect::Object &b) {
        return a.rect.area() > b.rect.area();
    });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(MAX_YOLOV5_MASK_OBJ_COUNT * RINGBUFFER_CACHE_COUNT);

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

        axobj.bHasMask = !obj.mask.empty();
        if (results->objects[i].bHasMask) {
            cv::Mat &mask = mSimpleRingBuffer.next();

            mask = obj.mask;
            axobj.yolov5Mask.w = mask.cols;
            axobj.yolov5Mask.h = mask.rows;
            axobj.yolov5Mask.data = mask.data;
        }

        if (obj.label < g_classNames.size()) {
            axobj.objname = g_classNames[obj.label];
        } else {
            axobj.objname = "unknown";
        }

        results->objects.push_back(axobj);
    }
}

static void axjoint_post_process_yolov7_palm_hand(axpi_results_t *results, axjoint_models_t *handler)
{
    std::vector<detect::PalmObject> objects;
    std::vector<detect::PalmObject> proposals;
    AX_U32 nOutputSize = handler->majorModel.jointAttr.outSize;
    AX_JOINT_IO_BUFFER_T *pOutputs = handler->majorModel.jointAttr.outputs;
    AX_JOINT_IOMETA_T *pOutputsInfo = handler->majorModel.jointAttr.outInfo;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / g_probThreshold) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i) {
        auto &info = pOutputs[i];
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        detect::generate_proposals_yolov7_palm(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid);
    }

    detect::get_out_bbox_palm(proposals, objects, g_nmsThreshold, handler->majorModel.jointAttr.height, handler->majorModel.jointAttr.width, handler->restoreHeight, handler->restoreWidth);

    std::sort(objects.begin(), objects.end(), [&](detect::PalmObject &a, detect::PalmObject &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t objsSize = MIN(objects.size(), MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < objsSize; i++) {
        axpi_object_t axobj;
        const detect::PalmObject &obj = objects[i];

        axobj.label = 0;
        axobj.prob = obj.prob;
        axobj.bHasBoxVertices = true;
        axobj.bbox.x = obj.rect.x * handler->restoreWidth;
        axobj.bbox.y = obj.rect.y * handler->restoreHeight;
        axobj.bbox.w = obj.rect.width * handler->restoreWidth;
        axobj.bbox.h = obj.rect.height * handler->restoreHeight;

        for (size_t j = 0; j < 4; j++) {
            results->objects[i].bbox_vertices[j].x = obj.vertices[j].x;
            results->objects[i].bbox_vertices[j].y = obj.vertices[j].y;
        }

        axobj.objname = "hand";
        results->objects.push_back(axobj);
    }
}

void axjoint_post_process_palm_hand(axpi_results_t *results, axjoint_models_t *handler)
{
    static const int strides[2] = {8, 16};
    static const int map_size[2] = {24, 12};
    static const int anchor_size[2] = {2, 6};
    static const float anchor_offset[2] = {0.5f, 0.5f};

    std::vector<detect::PalmObject> objects;
    std::vector<detect::PalmObject> proposals;

    auto &bboxes_info = handler->majorModel.jointAttr.outputs[0];
    auto bboxes_ptr = (float *)bboxes_info.pVirAddr;

    auto &scores_info = handler->majorModel.jointAttr.outputs[1];
    auto scores_ptr = (float *)scores_info.pVirAddr;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / g_probThreshold) - 1.0f);

    detect::generate_proposals_palm(proposals, g_probThreshold, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, scores_ptr, bboxes_ptr, 2, strides, anchor_size, anchor_offset, map_size, prob_threshold_unsigmoid);
    detect::get_out_bbox_palm(proposals, objects, g_nmsThreshold, handler->majorModel.jointAttr.height, handler->majorModel.jointAttr.width, handler->restoreHeight, handler->restoreWidth);

    std::sort(objects.begin(), objects.end(), [&](detect::PalmObject &a, detect::PalmObject &b) {
        return a.rect.area() > b.rect.area();
    });

    size_t ObjsSize = MIN(objects.size(), MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < ObjsSize; i++) {
        axpi_object_t axobj;
        const detect::PalmObject &obj = objects[i];

        axobj.label = 0;
        axobj.prob = obj.prob;
        axobj.bHasBoxVertices = true;
        axobj.bbox.x = obj.rect.x * handler->restoreWidth;
        axobj.bbox.y = obj.rect.y * handler->restoreHeight;
        axobj.bbox.w = obj.rect.width * handler->restoreWidth;
        axobj.bbox.h = obj.rect.height * handler->restoreHeight;

        std::vector<axpi_point_t> bbox_vertices;
        for (size_t j = 0; j < 4; j++) {
            axpi_point_t point;
            point.x = obj.vertices[j].x;
            point.y = obj.vertices[j].y;
            bbox_vertices.push_back(point);
        }
        axobj.bbox_vertices = bbox_vertices;

        axobj.objname = "hand";
        results->objects.push_back(axobj);
    }
}

static void axjoint_post_process_yolopv2(axpi_results_t *results, axjoint_models_t *handler)
{
    std::vector<detect::Object> objects;
    std::vector<detect::Object> proposals;
    AX_U32 nOutputSize = handler->majorModel.jointAttr.outSize;
    AX_JOINT_IO_BUFFER_T *pOutputs = handler->majorModel.jointAttr.outputs;
    AX_JOINT_IOMETA_T *pOutputsInfo = handler->majorModel.jointAttr.outInfo;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / g_probThreshold) - 1.0f);
    for (uint32_t i = 2; i < nOutputSize; ++i) {
        auto &info = pOutputs[i];
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)info.pVirAddr;

        int32_t stride = (1 << (i - 2)) * 8;
        detect::generate_proposals_yolov5(stride, ptr, g_probThreshold, proposals, handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height, g_anchors.data(), prob_threshold_unsigmoid, 80);
    }

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg(RINGBUFFER_CACHE_COUNT), mSimpleRingBuffer_ll(RINGBUFFER_CACHE_COUNT);

    auto &da_info = pOutputs[0];
    auto da_ptr = (float *)da_info.pVirAddr;

    auto &ll_info = pOutputs[1];
    auto ll_ptr = (float *)ll_info.pVirAddr;

    cv::Mat &da_seg_mask = mSimpleRingBuffer_seg.next();
    cv::Mat &ll_seg_mask = mSimpleRingBuffer_ll.next();

    detect::get_out_bbox_yolopv2(proposals, objects, da_ptr, ll_ptr, ll_seg_mask, da_seg_mask, g_nmsThreshold, handler->majorModel.jointAttr.height, handler->majorModel.jointAttr.width, handler->restoreHeight, handler->restoreWidth);
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

        axobj.label = 0;
        axobj.objname = "car";

        results->objects.push_back(axobj);
    }

    results->bYolopv2Mask = true;
    results->yolopv2seg.h = da_seg_mask.rows;
    results->yolopv2seg.w = da_seg_mask.cols;
    results->yolopv2seg.data = da_seg_mask.data;
    results->yolopv2ll.h = ll_seg_mask.rows;
    results->yolopv2ll.w = ll_seg_mask.cols;
    results->yolopv2ll.data = ll_seg_mask.data;
}

static void axjoint_post_process_yolofastbody(axpi_results_t *results, axjoint_models_t *handler)
{
    AX_U32 nOutputSize = handler->majorModel.jointAttr.outSize;
    AX_JOINT_IO_BUFFER_T *pOutputs = handler->majorModel.jointAttr.outputs;
    AX_JOINT_IOMETA_T *pOutputsInfo = handler->majorModel.jointAttr.outInfo;

    static bool bInit = false;
    static std::vector<float> output_buf;
    static yolo::YoloDetectionOutput yolo{};
    static std::vector<yolo::TMat> yolo_inputs, yolo_outputs;

    if (!bInit) {
        bInit = true;
        yolo.init(yolo::YOLO_FASTEST_BODY, g_nmsThreshold, g_probThreshold, 1);
        yolo_inputs.resize(nOutputSize);
        yolo_outputs.resize(1);
        output_buf.resize(1000 * 6, 0);
    }

    for (uint32_t i = 0; i < nOutputSize; ++i) {
        auto &info = pOutputs[i];
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)info.pVirAddr;

        yolo_inputs[i].batch = output.pShape[0];
        yolo_inputs[i].h = output.pShape[1];
        yolo_inputs[i].w = output.pShape[2];
        yolo_inputs[i].c = output.pShape[3];
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
    int src_cols = handler->restoreWidth;
    int src_rows = handler->restoreHeight;
    int letterbox_cols = handler->algoWidth;
    int letterbox_rows = handler->algoHeight;

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
        detect::Object &object = objects[i];
        float *data_row = yolo_outputs[0].row((int)i);

        object.rect.x = data_row[2] * (float)handler->algoWidth;
        object.rect.y = data_row[3] * (float)handler->algoHeight;
        object.rect.width = (data_row[4] - data_row[2]) * (float)handler->algoWidth;
        object.rect.height = (data_row[5] - data_row[3]) * (float)handler->algoHeight;
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

    size_t objsSize = MIN(objects.size(), MAX_BBOX_COUNT);
    for (size_t i = 0; i < objsSize; i++) {
        axpi_object_t axobj;
        const detect::Object &obj = objects[i];

        axobj.label = 0;
        axobj.prob = obj.prob;
        axobj.label = obj.label;
        axobj.bbox.x = obj.rect.x;
        axobj.bbox.y = obj.rect.y;
        axobj.bbox.w = obj.rect.width;
        axobj.bbox.h = obj.rect.height;
        axobj.objname = "person";

        results->objects.push_back(axobj);
    }
}

void axjoint_post_process_detect_single_func(axpi_results_t *results, axjoint_models_t *handler)
{
    typedef void (*post_process_func)(axpi_results_t *results, axjoint_models_t *handler);

    static std::map<int, post_process_func> m_func_map = {
        {MT_DET_YOLOV5,               axjoint_post_process_detection},
        {MT_DET_YOLOV5_FACE,          axjoint_post_process_detection},
        {MT_DET_YOLOV6,               axjoint_post_process_detection},
        {MT_DET_YOLOV7,               axjoint_post_process_detection},
        {MT_DET_YOLOX,                axjoint_post_process_detection},
        {MT_DET_NANODET,              axjoint_post_process_detection},
        {MT_DET_YOLOX_PPL,            axjoint_post_process_detection},
        {MT_DET_YOLOV5_LICENSE_PLATE, axjoint_post_process_detection},
        {MT_DET_YOLOV7_FACE,          axjoint_post_process_detection},

        {MT_DET_YOLOPV2,              axjoint_post_process_yolopv2},

        {MT_DET_YOLO_FASTBODY,        axjoint_post_process_yolofastbody},

        {MT_INSEG_YOLOV5_MASK,        axjoint_post_process_yolov5_seg},

        {MT_DET_PALM_HAND,            axjoint_post_process_palm_hand},
        {MT_DET_YOLOV7_PALM_HAND,     axjoint_post_process_yolov7_palm_hand},
    };

    auto item = m_func_map.find(handler->majorModel.modelType);
    if (item != m_func_map.end()) {
        item->second(results, handler);
    } else {
        axmpi_error("cannot find process func for model type:[%d]", handler->majorModel.modelType);
    }

    switch (handler->modelTypeMain) {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_ANIMAL_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            break;

        default:
            for (size_t i = 0; i < results->objects.size(); i++) {
                results->objects[i].bbox.x /= handler->restoreWidth;
                results->objects[i].bbox.y /= handler->restoreHeight;
                results->objects[i].bbox.w /= handler->restoreWidth;
                results->objects[i].bbox.h /= handler->restoreHeight;

                if ((handler->majorModel.modelType == MT_DET_YOLOV5_FACE) || (handler->majorModel.modelType == MT_DET_YOLOV7_FACE)) {
                    for (int j = 0; j < FACE_LMK_SIZE; j++) {
                        results->objects[i].landmark[j].x /= handler->restoreWidth;
                        results->objects[i].landmark[j].y /= handler->restoreHeight;
                    }
                } else if (results->modelType == MT_DET_YOLOV5_LICENSE_PLATE) {
                    for (int j = 0; j < PLATE_LMK_SIZE; j++) {
                        results->objects[i].landmark[j].x /= handler->restoreWidth;
                        results->objects[i].landmark[j].y /= handler->restoreHeight;
                    }
                }

                if (results->objects[i].bHasBoxVertices) {
                    for (size_t j = 0; j < 4; j++) {
                        results->objects[i].bbox_vertices[j].x /= handler->restoreWidth;
                        results->objects[i].bbox_vertices[j].y /= handler->restoreHeight;
                    }
                }
            }
            break;
    }
}
}
