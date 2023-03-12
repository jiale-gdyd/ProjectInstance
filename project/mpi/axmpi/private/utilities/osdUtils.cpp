#include <map>
#include <sstream>
#include <opencv2/opencv.hpp>

#include "log.hpp"
#include "osdUtils.hpp"
#include "multikeymap.hpp"
#include "../base/pose.hpp"
#include "../common/define.hpp"
#include "../joint/post_process.hpp"

namespace axpi {
typedef int (*display_callback_for_sipeed_py)(int, int, int, char **);
display_callback_for_sipeed_py g_cb_display_sipeed_py = NULL;

int register_display_callback(display_callback_for_sipeed_py cb)
{
    g_cb_display_sipeed_py = cb;
    return 0;
}

typedef void (*draw_func)(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y);

static const std::vector<cv::Scalar> g_CocoColors = {
    {128, 56, 0, 255},  {128, 226, 255, 0}, {128, 0, 94, 255},  {128, 0, 37, 255},  {128, 0, 255, 94},  {128, 255, 226, 0},
    {128, 0, 18, 255},  {128, 255, 151, 0}, {128, 170, 0, 255}, {128, 0, 255, 56},  {128, 255, 0, 75},  {128, 0, 75, 255},
    {128, 0, 255, 169}, {128, 255, 0, 207}, {128, 75, 255, 0},  {128, 207, 0, 255}, {128, 37, 0, 255},  {128, 0, 207, 255},
    {128, 94, 0, 255},  {128, 0, 255, 113}, {128, 255, 18, 0},  {128, 255, 0, 56},  {128, 18, 0, 255},  {128, 0, 255, 226},
    {128, 170, 255, 0}, {128, 255, 0, 245}, {128, 151, 255, 0}, {128, 132, 255, 0}, {128, 75, 0, 255},  {128, 151, 0, 255},
    {128, 0, 151, 255}, {128, 132, 0, 255}, {128, 0, 255, 245}, {128, 255, 132, 0}, {128, 226, 0, 255}, {128, 255, 37, 0},
    {128, 207, 255, 0}, {128, 0, 255, 207}, {128, 94, 255, 0},  {128, 0, 226, 255}, {128, 56, 255, 0},  {128, 255, 94, 0},
    {128, 255, 113, 0}, {128, 0, 132, 255}, {128, 255, 0, 132}, {128, 255, 170, 0}, {128, 255, 0, 188}, {128, 113, 255, 0},
    {128, 245, 0, 255}, {128, 113, 0, 255}, {128, 255, 188, 0}, {128, 0, 113, 255}, {128, 255, 0, 0},   {128, 0, 56, 255},
    {128, 255, 0, 113}, {128, 0, 255, 188}, {128, 255, 0, 94},  {128, 255, 0, 18},  {128, 18, 255, 0},  {128, 0, 255, 132},
    {128, 0, 188, 255}, {128, 0, 245, 255}, {128, 0, 169, 255}, {128, 37, 255, 0},  {128, 255, 0, 151}, {128, 188, 0, 255},
    {128, 0, 255, 37},  {128, 0, 255, 0},   {128, 255, 0, 170}, {128, 255, 0, 37},  {128, 255, 75, 0},  {128, 0, 0, 255},
    {128, 255, 207, 0}, {128, 255, 0, 226}, {128, 255, 245, 0}, {128, 188, 255, 0}, {128, 0, 255, 18},  {128, 0, 255, 75},
    {128, 0, 255, 151}, {128, 255, 56, 0},  {128, 245, 255, 0}
};

void createImage(int charlen, float fontscale, int thickness, axosd_img_t *out)
{
    std::stringstream ss;

    for (int i = 0; i < charlen; i++) {
        ss << "O";
    }
    std::string text = ss.str();

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);

    out->width = label_size.width;
    out->height = label_size.height * 1.5;
    out->channel = 4;
    out->data = new uchar[out->width * out->height * out->channel];
    memset(out->data, 0, out->width * out->height * out->channel);
}

int putText(const char *text, float fontscale, int thickness, axosd_img_t *base, axosd_img_t *out)
{
    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);
    out->width = label_size.width;
    out->height = label_size.height * 1.5;
    out->channel = 4;

    if (out->width * out->height * out->channel > base->width * base->height * base->channel) {
        return -1;
    }
    out->data = base->data;

    memset(out->data, 0, out->width * out->height * out->channel);

    cv::Mat src(out->height, out->width, CV_8UC4, out->data);
    cv::putText(src, text, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale, cv::Scalar(255, 0, 0, 255), 2);
    return 0;
}

void releaseImage(axosd_img_t *img)
{
    if (img) {
        delete[] img->data;
        memset(img, 0, sizeof(axosd_img_t));
    }
}

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

static void _draw_bbox(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    int x, y;
    int baseLine = 0;
    cv::Size label_size;

    for (size_t i = 0; i < results->objects.size(); i++) {
        cv::Rect rect(results->objects[i].bbox.x * out->width + offset_x, results->objects[i].bbox.y * out->height + offset_y, results->objects[i].bbox.w * out->width, results->objects[i].bbox.h * out->height);
        label_size = cv::getTextSize(results->objects[i].objname, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);

        if (results->objects[i].bHasBoxVertices) {
            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[0].x * out->width + offset_x, results->objects[i].bbox_vertices[0].y * out->height + offset_y),
                cv::Point(results->objects[i].bbox_vertices[1].x * out->width + offset_x, results->objects[i].bbox_vertices[1].y * out->height + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[1].x * out->width + offset_x, results->objects[i].bbox_vertices[1].y * out->height + offset_y),
                cv::Point(results->objects[i].bbox_vertices[2].x * out->width + offset_x, results->objects[i].bbox_vertices[2].y * out->height + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[2].x * out->width + offset_x, results->objects[i].bbox_vertices[2].y * out->height + offset_y),
                cv::Point(results->objects[i].bbox_vertices[3].x * out->width + offset_x, results->objects[i].bbox_vertices[3].y * out->height + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[3].x * out->width + offset_x, results->objects[i].bbox_vertices[3].y * out->height + offset_y),
                cv::Point(results->objects[i].bbox_vertices[0].x * out->width + offset_x, results->objects[i].bbox_vertices[0].y * out->height + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            x = results->objects[i].bbox_vertices[0].x * out->width + offset_x;
            y = results->objects[i].bbox_vertices[0].y * out->height + offset_y - label_size.height - baseLine;
        } else {
            if (results->objects[i].label < g_CocoColors.size()) {
                cv::rectangle(image, rect, g_CocoColors[results->objects[i].label], thickness);
            } else {
                cv::rectangle(image, rect, cv::Scalar(255, 128, 128, 128), thickness);
            }

            x = rect.x;
            y = rect.y - label_size.height - baseLine;
        }

        if (y < 0) {
            y = 0;
        }

        if ((x + label_size.width) > image.cols) {
            x = image.cols - label_size.width;
        }

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)), cv::Scalar(255, 255, 255, 255), -1);
        cv::putText(image, results->objects[i].objname, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale, cv::Scalar(0, 0, 0, 255), thickness);
    }
}

static void _draw_yolov5_face(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    _draw_bbox(image, out, fontscale, thickness, results, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        for (int j = 0; j < FACE_LMK_SIZE; j++) {
            cv::Point p(results->objects[i].landmark[j].x * out->width + offset_x, results->objects[i].landmark[j].y * out->height + offset_y);
            cv::circle(image, p, 1, cv::Scalar(255, 0, 0, 255), 2);
        }
    }
}

static void _draw_yolov5_seg(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    _draw_bbox(image, out, fontscale, thickness, results, offset_x, offset_y);

    for (size_t i = 0; i < results->objects.size(); i++) {
        cv::Rect rect(results->objects[i].bbox.x * out->width + offset_x,
            results->objects[i].bbox.y * out->height + offset_y,
            results->objects[i].bbox.w * out->width, results->objects[i].bbox.h * out->height);

        if (results->objects[i].bHasMask && results->objects[i].yolov5Mask.data) {
            cv::Mat mask(results->objects[i].yolov5Mask.h, results->objects[i].yolov5Mask.w, CV_8U, results->objects[i].yolov5Mask.data);
            if (!mask.empty()) {
                cv::Mat mask_target;
                cv::resize(mask, mask_target, cv::Size(results->objects[i].bbox.w * out->width, results->objects[i].bbox.h * out->height), 0, 0, cv::INTER_NEAREST);

                if (results->objects[i].label < g_CocoColors.size()) {
                    image(rect).setTo(g_CocoColors[results->objects[i].label], mask_target);
                } else {
                    image(rect).setTo(cv::Scalar(128, 128, 128, 128), mask_target);
                }
            }
        }
    }
}

static void _draw_yolopv2(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    if (results->bYolopv2Mask && results->yolopv2ll.data && results->yolopv2seg.data) {
        static cv::Mat base(g_majorStreamHeight, g_majorStreamWidth, CV_8UC1);
        cv::Mat tmp(image.rows, image.cols, CV_8UC1, base.data);

        cv::Mat seg_mask(results->yolopv2seg.h, results->yolopv2seg.w, CV_8UC1, results->yolopv2seg.data);
        cv::resize(seg_mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 0, 128), tmp);

        cv::Mat ll_mask(results->yolopv2ll.h, results->yolopv2ll.w, CV_8UC1, results->yolopv2ll.data);
        cv::resize(ll_mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 128, 0), tmp);
    }

    _draw_bbox(image, out, fontscale, thickness, results, offset_x, offset_y);
}

static void _draw_human_pose(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    _draw_bbox(image, out, fontscale, thickness, results, offset_x, offset_y);

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

        draw_pose_result(image, &results->objects[i], pairs, HAND_LMK_SIZE, offset_x, offset_y);
    }
}

static void _draw_hand_pose(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    _draw_bbox(image, out, fontscale, thickness, results, offset_x, offset_y);

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

        draw_pose_result(image, &results->objects[i], hand_pairs, HAND_LMK_SIZE, offset_x, offset_y);
    }
}

static void _draw_animal_pose(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    _draw_bbox(image, out, fontscale, thickness, results, offset_x, offset_y);

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

        draw_pose_result(image, &results->objects[i], pairs, ANIMAL_LMK_SIZE, offset_x, offset_y);
    }
}

static void _draw_pphumseg(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    if (results->bPPHumSeg && results->PPHumSeg.data) {
        static cv::Mat base(g_majorStreamHeight, g_majorStreamWidth, CV_8UC1);
        cv::Mat tmp(image.rows, image.cols, CV_8UC1, base.data);
        cv::Mat mask(results->PPHumSeg.h, results->PPHumSeg.w, CV_8UC1, results->PPHumSeg.data);

        cv::resize(mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 0, 128), tmp);
    }
}

static MultikeyMap<std::string, int, draw_func> mDrawtable = {
    {"null",                      MT_UNKNOWN,                         nullptr},
    {"yolov5",                    MT_DET_YOLOV5,                      _draw_bbox},
    {"ax_person_det",             MT_DET_YOLOX_PPL,                   _draw_bbox},
    {"palm_hand_det",             MT_DET_PALM_HAND,                   _draw_bbox},
    {"yolov6",                    MT_DET_YOLOV6,                      _draw_bbox},
    {"yolov7",                    MT_DET_YOLOV7,                      _draw_bbox},
    {"yolov7_face",               MT_DET_YOLOV7_FACE,                 _draw_yolov5_face},
    {"yolov7_palm_hand",          MT_DET_YOLOV7_PALM_HAND,            _draw_bbox},
    {"yolox",                     MT_DET_YOLOX,                       _draw_bbox},
    {"nanodet",                   MT_DET_NANODET,                     _draw_bbox},
    {"yolo_fastbody",             MT_DET_YOLO_FASTBODY,               _draw_bbox},
    {"yolo_license_plate",        MT_DET_YOLOV5_LICENSE_PLATE,        _draw_bbox},
    {"yolov5_face",               MT_DET_YOLOV5_FACE,                 _draw_yolov5_face},
    {"yolov5_seg",                MT_INSEG_YOLOV5_MASK,               _draw_yolov5_seg},
    {"yolopv2",                   MT_DET_YOLOPV2,                     _draw_yolopv2},
    {"pp_human_seg",              MT_SEG_PPHUMSEG,                    _draw_pphumseg},
    {"hrnet_human_pose",          MT_MLM_HUMAN_POSE_HRNET,            _draw_human_pose},
    {"ax_human_pose",             MT_MLM_HUMAN_POSE_AXPPL,            _draw_human_pose},
    {"hrnet_animal_pose",         MT_MLM_ANIMAL_POSE_HRNET,           _draw_animal_pose},
    {"hand_pose",                 MT_MLM_HAND_POSE,                   _draw_hand_pose},
    {"license_plate_recognition", MT_MLM_VEHICLE_LICENSE_RECOGNITION, _draw_bbox},
    {"face_recognition",          MT_MLM_FACE_RECOGNITION,            _draw_bbox},
};

static void _draw_fps(cv::Mat &image, axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    static char common_info[128];

    if (mDrawtable.contain(results->modelType)) {
        int mt = results->modelType;
        auto match_vec = mDrawtable.get2(mt);
        if (match_vec.size() > 1) {
            axmpi_error("[%d] multi define in mDrawtable, please check mDrawtable", mt);
            return;
        }

        sprintf(common_info, "%s fps:%02d", match_vec[0]->key1.c_str(), results->inFps);
    } else {
        sprintf(common_info, "%s fps:%02d", "unknown", results->inFps);
    }

    cv::Size label_size = cv::getTextSize(common_info, cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, thickness, NULL);
    cv::putText(image, common_info, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, cv::Scalar(255, 0, 255, 255), thickness);
}

void drawResults(axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y)
{
    if (g_cb_display_sipeed_py && (g_cb_display_sipeed_py(out->height, out->width, CV_8UC4, (char **)&out->data) != 0)) {
        return;
    }

    cv::Mat image(out->height, out->width, CV_8UC4, out->data);

    if (mDrawtable.contain(results->modelType) && results->modelType != MT_UNKNOWN) {
        int mt = results->modelType;
        auto match_vec = mDrawtable.get2(mt);
        if (match_vec.size() > 1) {
            axmpi_error("[%d] multi define in mDrawtable, please check mDrawtable", mt);
            return;
        }

        if (match_vec[0]->val != nullptr) {
            match_vec[0]->val(image, out, fontscale, thickness, results, offset_x, offset_y);
        } else {
            axmpi_error("[%s] draw func got null", match_vec[0]->key1.c_str());
        }
    }

    _draw_fps(image, out, fontscale, thickness, results, offset_x, offset_y);
}

int releaseObjects(axpi_results_t *results)
{
    results->bPPHumSeg = false;
    results->bYolopv2Mask = false;

    for (int i = 0; i < results->objects.size(); i++) {
        if (results->objects[i].bHasMask && results->objects[i].yolov5Mask.data) {
            results->objects[i].yolov5Mask.data = nullptr;
        }
    }

    results->objects.clear();
    results->PPHumSeg.data = nullptr;
    results->yolopv2ll.data = nullptr;
    results->yolopv2seg.data = nullptr;

    return 0;
}
}
