#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <opencv2/opencv.hpp>

namespace detect {
typedef struct {
    int grid0;
    int grid1;
    int stride;
} GridAndStride;

typedef struct {
    cv::Rect_<float>   rect;
    int                label;
    float              prob;
    cv::Point2f        landmark[5];
    cv::Mat            mask;
    std::vector<float> mask_feat;
} Object;

typedef struct PalmObject {
    cv::Rect_<float> rect;
    float            prob;
    cv::Point2f      vertices[4];
    cv::Point2f      landmarks[7];
    cv::Mat          affine_trans_mat;
    cv::Mat          affine_trans_mat_inv;
} PalmObject;

static float softmax(const float* src, float* dst, int length)
{
    float dis_sum = 0;
    float denominator = 0;
    const float alpha = *std::max_element(src, src + length);

    for (int i = 0; i < length; ++i) {
        dst[i] = exp(src[i] - alpha);
        denominator += dst[i];
    }

    for (int i = 0; i < length; ++i) {
        dst[i] /= denominator;
        dis_sum += i * dst[i];
    }

    return dis_sum;
}

static inline float sigmoid(float x)
{
    return static_cast<float>(1.f / (1.f + exp(-x)));
}

template <typename T>
static inline float intersection_area(const T &a, const T &b)
{
    cv::Rect_<float> inter = a.rect & b.rect;
    return inter.area();
}

template <typename T>
static void qsort_descent_inplace(std::vector<T> &faceobjects, int left, int right)
{
    int i = left;
    int j = right;
    float p = faceobjects[(left + right) / 2].prob;

    while (i <= j) {
        while (faceobjects[i].prob > p) {
            i++;
        }

        while (faceobjects[j].prob < p) {
            j--;
        }

        if (i <= j) {
            std::swap(faceobjects[i], faceobjects[j]);

            i++;
            j--;
        }
    }
#pragma omp parallel sections
    {
#pragma omp section
        {
            if (left < j) {
                qsort_descent_inplace(faceobjects, left, j);
            }
        }
#pragma omp section
        {
            if (i < right) {
                qsort_descent_inplace(faceobjects, i, right);
            }
        }
    }
}

template <typename T>
static void qsort_descent_inplace(std::vector<T> &faceobjects)
{
    if (faceobjects.empty()) {
        return;
    }

    qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
}

template <typename T>
static void nms_sorted_bboxes(const std::vector<T> &faceobjects, std::vector<int> &picked, float nms_threshold)
{
    picked.clear();

    const int n = faceobjects.size();

    std::vector<float> areas(n);
    for (int i = 0; i < n; i++) {
        areas[i] = faceobjects[i].rect.area();
    }

    for (int i = 0; i < n; i++) {
        int keep = 1;
        const T &a = faceobjects[i];

        for (int j = 0; j < (int)picked.size(); j++) {
            const T &b = faceobjects[picked[j]];

            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;

            if ((inter_area / union_area) > nms_threshold) {
                keep = 0;
            }
        }

        if (keep) {
            picked.push_back(i);
        }
    }
}

static void get_out_bbox(std::vector<Object> &proposals, std::vector<Object> &objects, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
{
    qsort_descent_inplace(proposals);
    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_threshold);

    int resize_rows;
    int resize_cols;
    float scale_letterbox;

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

    int count = picked.size();

    objects.resize(count);
    for (int i = 0; i < count; i++) {
        objects[i] = proposals[picked[i]];
        float x0 = (objects[i].rect.x);
        float y0 = (objects[i].rect.y);
        float x1 = (objects[i].rect.x + objects[i].rect.width);
        float y1 = (objects[i].rect.y + objects[i].rect.height);

        x0 = (x0 - tmp_w) * ratio_x;
        y0 = (y0 - tmp_h) * ratio_y;
        x1 = (x1 - tmp_w) * ratio_x;
        y1 = (y1 - tmp_h) * ratio_y;

        for (int l = 0; l < 5; l++) {
            auto lx = objects[i].landmark[l].x;
            auto ly = objects[i].landmark[l].y;
            objects[i].landmark[l] = cv::Point2f((lx - tmp_w) * ratio_x, (ly - tmp_h) * ratio_y);
        }

        x0 = std::max(std::min(x0, (float)(src_cols - 1)), 0.f);
        y0 = std::max(std::min(y0, (float)(src_rows - 1)), 0.f);
        x1 = std::max(std::min(x1, (float)(src_cols - 1)), 0.f);
        y1 = std::max(std::min(y1, (float)(src_rows - 1)), 0.f);

        objects[i].rect.x = x0;
        objects[i].rect.y = y0;
        objects[i].rect.width = x1 - x0;
        objects[i].rect.height = y1 - y0;
    }
}

static void get_out_bbox_yolopv2(std::vector<Object> &proposals, std::vector<Object> &objects, const float *da_ptr, const float *ll_ptr, cv::Mat &ll_seg_mask, cv::Mat &da_seg_mask, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
{
    qsort_descent_inplace(proposals);
    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_threshold);

    int resize_rows;
    int resize_cols;
    float scale_letterbox;

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

    int count = picked.size();

    objects.resize(count);
    for (int i = 0; i < count; i++) {
        objects[i] = proposals[picked[i]];
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

    cv::Mat ll = cv::Mat(cv::Size(letterbox_cols, letterbox_rows), CV_32FC1, (float *)ll_ptr);
    ll_seg_mask = ll(cv::Rect(tmp_w, tmp_h, resize_cols, resize_rows)) > 0.5;

    cv::Mat da = cv::Mat(cv::Size(letterbox_cols, letterbox_rows), CV_32FC1, (float *)da_ptr);
    da_seg_mask = da(cv::Rect(tmp_w, tmp_h, resize_cols, resize_rows)) > 0;
}

static void generate_proposals_scrfd(int feat_stride, const float *score_blob, const float *bbox_blob, const float *kps_blob, float prob_threshold_unsigmoid, std::vector<Object> &faceobjects, int letterbox_cols, int letterbox_rows)
{
    int anchor_group = 1;
    int feat_w = letterbox_cols / feat_stride;
    int feat_h = letterbox_rows / feat_stride;
    int feat_size = feat_w * feat_h;
    static float anchors[] = {-8.f, -8.f, 8.f, 8.f, -16.f, -16.f, 16.f, 16.f, -32.f, -32.f, 32.f, 32.f, -64.f, -64.f, 64.f, 64.f, -128.f, -128.f, 128.f, 128.f, -256.f, -256.f, 256.f, 256.f};

    if (feat_stride == 8) {
        anchor_group = 1;
    }

    if (feat_stride == 16) {
        anchor_group = 2;
    }

    if (feat_stride == 32) {
        anchor_group = 3;
    }

    const int num_anchors = 2;

    for (int q = 0; q < num_anchors; q++) {
        float anchor_y = anchors[(anchor_group - 1) * 8 + q * 4 + 1];
        float anchor_w = anchors[(anchor_group - 1) * 8 + q * 4 + 2] - anchors[(anchor_group - 1) * 8 + q * 4 + 0];
        float anchor_h = anchors[(anchor_group - 1) * 8 + q * 4 + 3] - anchors[(anchor_group - 1) * 8 + q * 4 + 1];

        for (int i = 0; i < feat_h; i++) {
            float anchor_x = anchors[(anchor_group - 1) * 8 + q * 4 + 0];

            for (int j = 0; j < feat_w; j++) {
                int index = i * feat_w + j;

                if (score_blob[q * feat_size + index] >= prob_threshold_unsigmoid) {
                    float dx = bbox_blob[(q * 4 + 0) * feat_size + index] * feat_stride;
                    float dy = bbox_blob[(q * 4 + 1) * feat_size + index] * feat_stride;
                    float dw = bbox_blob[(q * 4 + 2) * feat_size + index] * feat_stride;
                    float dh = bbox_blob[(q * 4 + 3) * feat_size + index] * feat_stride;

                    float cx = anchor_x + anchor_w * 0.5f;
                    float cy = anchor_y + anchor_h * 0.5f;

                    float x0 = cx - dx;
                    float y0 = cy - dy;
                    float x1 = cx + dw;
                    float y1 = cy + dh;

                    Object obj;
                    obj.label = 0;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0 + 1;
                    obj.rect.height = y1 - y0 + 1;
                    obj.prob = sigmoid(score_blob[q * feat_size + index]);

                    if (kps_blob != 0) {
                        obj.landmark[0].x = cx + kps_blob[index] * feat_stride;
                        obj.landmark[0].y = cy + kps_blob[1 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[1].x = cx + kps_blob[2 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[1].y = cy + kps_blob[3 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[2].x = cx + kps_blob[4 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[2].y = cy + kps_blob[5 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[3].x = cx + kps_blob[6 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[3].y = cy + kps_blob[7 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[4].x = cx + kps_blob[8 * feat_h * feat_w + index] * feat_stride;
                        obj.landmark[4].y = cy + kps_blob[9 * feat_h * feat_w + index] * feat_stride;
                    }

                    faceobjects.push_back(obj);
                }

                anchor_x += feat_stride;
            }

            anchor_y += feat_stride;
        }
    }
}

static void generate_proposals_yolov5(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid, int cls_num)
{
    int anchor_group;
    int anchor_num = 3;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    if (stride == 8) {
        anchor_group = 1;
    }

    if (stride == 16) {
        anchor_group = 2;
    }

    if (stride == 32) {
        anchor_group = 3;
    }

    auto feature_ptr = feat;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            for (int a = 0; a <= anchor_num - 1; a++) {
                if (feature_ptr[4] < prob_threshold_unsigmoid) {
                    feature_ptr += (cls_num + 5);
                    continue;
                }

                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s <= cls_num - 1; s++) {
                    float score = feature_ptr[s + 5];
                    if (score > class_score) {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_score = feature_ptr[4];
                float final_score = sigmoid(box_score) * sigmoid(class_score);

                if (final_score >= prob_threshold) {
                    float dx = sigmoid(feature_ptr[0]);
                    float dy = sigmoid(feature_ptr[1]);
                    float dw = sigmoid(feature_ptr[2]);
                    float dh = sigmoid(feature_ptr[3]);
                    float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                    float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                    float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                    float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                    float pred_w = dw * dw * 4.0f * anchor_w;
                    float pred_h = dh * dh * 4.0f * anchor_h;
                    float x0 = pred_cx - pred_w * 0.5f;
                    float y0 = pred_cy - pred_h * 0.5f;
                    float x1 = pred_cx + pred_w * 0.5f;
                    float y1 = pred_cy + pred_h * 0.5f;

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0;
                    obj.rect.height = y1 - y0;
                    obj.label = class_index;
                    obj.prob = final_score;
                    objects.push_back(obj);
                }

                feature_ptr += (cls_num + 5);
            }
        }
    }
}

static void generate_proposals_yolov5_face(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid, int num_landmark)
{
    int cls_num = 1;
    int anchor_group;
    int anchor_num = 3;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    if (stride == 8) {
        anchor_group = 1;
    }

    if (stride == 16) {
        anchor_group = 2;
    }

    if (stride == 32) {
        anchor_group = 3;
    }

    auto feature_ptr = feat;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            for (int a = 0; a <= anchor_num - 1; a++) {
                if (feature_ptr[4] < prob_threshold_unsigmoid) {
                    feature_ptr += (cls_num + 5 + num_landmark * 2);
                    continue;
                }

                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s <= (cls_num - 1); s++) {
                    float score = feature_ptr[s + 5 + num_landmark * 2];
                    if (score > class_score) {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_score = feature_ptr[4];
                float final_score = sigmoid(box_score) * sigmoid(class_score);

                if (final_score >= prob_threshold) {
                    float dx = sigmoid(feature_ptr[0]);
                    float dy = sigmoid(feature_ptr[1]);
                    float dw = sigmoid(feature_ptr[2]);
                    float dh = sigmoid(feature_ptr[3]);
                    float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                    float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                    float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                    float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                    float pred_w = dw * dw * 4.0f * anchor_w;
                    float pred_h = dh * dh * 4.0f * anchor_h;
                    float x0 = pred_cx - pred_w * 0.5f;
                    float y0 = pred_cy - pred_h * 0.5f;
                    float x1 = pred_cx + pred_w * 0.5f;
                    float y1 = pred_cy + pred_h * 0.5f;

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0;
                    obj.rect.height = y1 - y0;
                    obj.label = class_index;
                    obj.prob = final_score;

                    const float *landmark_ptr = feature_ptr + 5;
                    for (int l = 0; l < num_landmark; l++) {
                        float lx = landmark_ptr[l * 2 + 0];
                        float ly = landmark_ptr[l * 2 + 1];
                        lx = lx * anchor_w + w * stride;
                        ly = ly * anchor_h + h * stride;
                        obj.landmark[l] = cv::Point2f(lx, ly);
                    }

                    objects.push_back(obj);
                }

                feature_ptr += (cls_num + 5 + num_landmark * 2);
            }
        }
    }
}

static void generate_proposals_yolox(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, int cls_num = 80)
{
    auto feat_ptr = feat;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            float box_objectness = feat_ptr[4];
            if (box_objectness < prob_threshold) {
                feat_ptr += cls_num + 5;
                continue;
            }

            int class_index = 0;
            float class_score = -FLT_MAX;
            for (int s = 0; s <= (cls_num - 1); s++) {
                float score = feat_ptr[s + 5];
                if (score > class_score) {
                    class_index = s;
                    class_score = score;
                }
            }

            float box_prob = box_objectness * class_score;
            if (box_prob > prob_threshold) {
                float x_center = (feat_ptr[0] + w) * stride;
                float y_center = (feat_ptr[1] + h) * stride;
                float w = exp(feat_ptr[2]) * stride;
                float h = exp(feat_ptr[3]) * stride;
                float x0 = x_center - w * 0.5f;
                float y0 = y_center - h * 0.5f;

                Object obj;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = w;
                obj.rect.height = h;
                obj.label = class_index;
                obj.prob = box_prob;

                objects.push_back(obj);
            }

            feat_ptr += cls_num + 5;
        }
    }
}

static void generate_proposals_yolov6(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, int cls_num = 80)
{
    auto feat_ptr = feat;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            int class_index = 0;
            float class_score = -FLT_MAX;
            for (int s = 0; s <= cls_num - 1; s++) {
                float score = feat_ptr[s + 4];
                if (score > class_score) {
                    class_index = s;
                    class_score = score;
                }
            }

            float box_prob = class_score;
            if (box_prob > prob_threshold) {
                float x0 = (w + 0.5f - feat_ptr[0]) * stride;
                float y0 = (h + 0.5f - feat_ptr[1]) * stride;
                float x1 = (w + 0.5f + feat_ptr[2]) * stride;
                float y1 = (h + 0.5f + feat_ptr[3]) * stride;

                float w = x1 - x0;
                float h = y1 - y0;

                Object obj;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = w;
                obj.rect.height = h;
                obj.label = class_index;
                obj.prob = box_prob;

                objects.push_back(obj);
            }

            feat_ptr += cls_num + 4;
        }
    }
}

static void generate_proposals_yolov7(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, const float *anchors, int cls_num = 80)
{
    auto feat_ptr = feat;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            for (int a_index = 0; a_index < 3; ++a_index) {
                float box_objectness = feat_ptr[4];
                if (box_objectness < prob_threshold) {
                    feat_ptr += cls_num + 5;
                    continue;
                }

                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s <= (cls_num - 1); s++) {
                    float score = feat_ptr[s + 5];
                    if (score > class_score) {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_prob = box_objectness * class_score;
                if (box_prob > prob_threshold) {
                    float x_center = (feat_ptr[0] * 2 - 0.5f + (float)w) * (float)stride;
                    float y_center = (feat_ptr[1] * 2 - 0.5f + (float)h) * (float)stride;
                    float box_w = (feat_ptr[2] * 2) * (feat_ptr[2] * 2) * anchors[a_index * 2];
                    float box_h = (feat_ptr[3] * 2) * (feat_ptr[3] * 2) * anchors[a_index * 2 + 1];
                    float x0 = x_center - box_w * 0.5f;
                    float y0 = y_center - box_h * 0.5f;

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = box_w;
                    obj.rect.height = box_h;
                    obj.label = class_index;
                    obj.prob = box_prob;

                    objects.push_back(obj);
                }

                feat_ptr += cls_num + 5;
            }
        }
    }
}

static void generate_proposals_nanodet(const float *pred_80_32_nhwc, int stride, const int &model_w, const int &model_h, float prob_threshold, std::vector<Object> &objects, int num_class = 80)
{
    const int reg_max_1 = 8;
    const int num_grid_x = model_w / stride;
    const int num_grid_y = model_h / stride;
    const int channel = num_class + reg_max_1 * 4;

    for (int i = 0; i < num_grid_y; i++) {
        for (int j = 0; j < num_grid_x; j++) {
            const int idx = i * num_grid_x + j;
            const float *scores = pred_80_32_nhwc + idx * channel;

            int label = -1;
            float score = -FLT_MAX;
    
            for (int k = 0; k < num_class; k++) {
                if (scores[k] > score) {
                    label = k;
                    score = scores[k];
                }
            }

            score = sigmoid(score);
            if (score >= prob_threshold) {
                float pred_ltrb[4];
                for (int k = 0; k < 4; k++) {
                    float dis = 0.f;
                    float dis_after_sm[8] = {0.};

                    softmax(scores + num_class + k * reg_max_1, dis_after_sm, 8);

                    for (int l = 0; l < reg_max_1; l++) {
                        dis += l * dis_after_sm[l];
                    }

                    pred_ltrb[k] = dis * stride;
                }

                float pb_cx = (j + 0.5f) * stride;
                float pb_cy = (i + 0.5f) * stride;

                float x0 = pb_cx - pred_ltrb[0];
                float y0 = pb_cy - pred_ltrb[1];
                float x1 = pb_cx + pred_ltrb[2];
                float y1 = pb_cy + pred_ltrb[3];

                Object obj;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = x1 - x0;
                obj.rect.height = y1 - y0;
                obj.label = label;
                obj.prob = score;

                objects.push_back(obj);
            }
        }
    }
}

static void generate_grids_and_stride(const int target_w, const int target_h, std::vector<int> &strides, std::vector<GridAndStride> &grid_strides)
{
    for (auto stride : strides) {
        int num_grid_w = target_w / stride;
        int num_grid_h = target_h / stride;

        for (int g1 = 0; g1 < num_grid_h; g1++) {
            for (int g0 = 0; g0 < num_grid_w; g0++) {
                GridAndStride gs;
                gs.grid0 = g0;
                gs.grid1 = g1;
                gs.stride = stride;
                grid_strides.push_back(gs);
            }
        }
    }
}

static void generate_yolox_proposals(std::vector<GridAndStride> grid_strides, float *feat_ptr, float prob_threshold, std::vector<Object> &objects, int wxc, int num_class)
{
    const int num_anchors = grid_strides.size();

    for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++) {
        for (int class_idx = 0; class_idx < num_class; class_idx++) {
            float box_objectness = feat_ptr[4 * wxc + anchor_idx];
            float box_cls_score = feat_ptr[(5 + class_idx) * wxc + anchor_idx];
            float box_prob = box_objectness * box_cls_score;
    
            if (box_prob > prob_threshold) {
                Object obj;

                const int grid0 = grid_strides[anchor_idx].grid0;
                const int grid1 = grid_strides[anchor_idx].grid1;
                const int stride = grid_strides[anchor_idx].stride;

                float x_center = (feat_ptr[0 + anchor_idx] + grid0) * stride;
                float y_center = (feat_ptr[1 * wxc + anchor_idx] + grid1) * stride;
                float w = exp(feat_ptr[2 * wxc + anchor_idx]) * stride;
                float h = exp(feat_ptr[3 * wxc + anchor_idx]) * stride;
                float x0 = x_center - w * 0.5f;
                float y0 = y_center - h * 0.5f;

                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = w;
                obj.rect.height = h;
                obj.label = class_idx;
                obj.prob = box_prob;

                objects.push_back(obj);
            }
        }
    }
}

static void generate_proposals_yolov5_seg(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid, int cls_num = 80, int mask_proto_dim = 32)
{
    int anchor_group;
    int anchor_num = 3;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    if (stride == 8) {
        anchor_group = 1;
    }

    if (stride == 16) {
        anchor_group = 2;
    }

    if (stride == 32) {
        anchor_group = 3;
    }

    auto feature_ptr = feat;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            for (int a = 0; a <= anchor_num - 1; a++) {
                if (feature_ptr[4] < prob_threshold_unsigmoid) {
                    feature_ptr += (cls_num + 5 + mask_proto_dim);
                    continue;
                }

                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s <= cls_num - 1; s++) {
                    float score = feature_ptr[s + 5];
                    if (score > class_score) {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_score = feature_ptr[4];
                float final_score = sigmoid(box_score) * sigmoid(class_score);

                if (final_score >= prob_threshold) {
                    float dx = sigmoid(feature_ptr[0]);
                    float dy = sigmoid(feature_ptr[1]);
                    float dw = sigmoid(feature_ptr[2]);
                    float dh = sigmoid(feature_ptr[3]);
                    float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                    float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                    float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                    float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                    float pred_w = dw * dw * 4.0f * anchor_w;
                    float pred_h = dh * dh * 4.0f * anchor_h;
                    float x0 = pred_cx - pred_w * 0.5f;
                    float y0 = pred_cy - pred_h * 0.5f;
                    float x1 = pred_cx + pred_w * 0.5f;
                    float y1 = pred_cy + pred_h * 0.5f;

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0;
                    obj.rect.height = y1 - y0;
                    obj.label = class_index;
                    obj.prob = final_score;
                    obj.mask_feat.resize(mask_proto_dim);
                    for (int k = 0; k < mask_proto_dim; k++) {
                        obj.mask_feat[k] = feature_ptr[cls_num + 5 + k];
                    }
                    objects.push_back(obj);
                }

                feature_ptr += (cls_num + 5 + mask_proto_dim);
            }
        }
    }
}

static void get_out_bbox_mask(std::vector<Object> &proposals, std::vector<Object> &objects, int objs_max_count, const float *mask_proto, int mask_proto_dim, int mask_stride, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
{
    qsort_descent_inplace(proposals);
    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_threshold);

    int resize_rows;
    int resize_cols;
    float scale_letterbox;

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

    int mask_proto_h = int(letterbox_rows / mask_stride);
    int mask_proto_w = int(letterbox_cols / mask_stride);

    int count = std::min(objs_max_count, (int)picked.size());
    objects.resize(count);

    for (int i = 0; i < count; i++) {
        objects[i] = proposals[picked[i]];
        float x0 = (objects[i].rect.x);
        float y0 = (objects[i].rect.y);
        float x1 = (objects[i].rect.x + objects[i].rect.width);
        float y1 = (objects[i].rect.y + objects[i].rect.height);

        int hstart = std::floor(objects[i].rect.y / mask_stride);
        int hend = std::ceil(objects[i].rect.y / mask_stride + objects[i].rect.height / mask_stride);
        int wstart = std::floor(objects[i].rect.x / mask_stride);
        int wend = std::ceil(objects[i].rect.x / mask_stride + objects[i].rect.width / mask_stride);

        hstart = std::min(std::max(hstart, 0), mask_proto_h);
        wstart = std::min(std::max(wstart, 0), mask_proto_w);
        hend = std::min(std::max(hend, 0), mask_proto_h);
        wend = std::min(std::max(wend, 0), mask_proto_w);

        int mask_w = wend - wstart;
        int mask_h = hend - hstart;

        cv::Mat mask = cv::Mat(mask_h, mask_w, CV_32FC1);
        if (mask_w > 0 && mask_h > 0) {
            std::vector<cv::Range> roi_ranges;
            roi_ranges.push_back(cv::Range(0, 1));
            roi_ranges.push_back(cv::Range::all());
            roi_ranges.push_back(cv::Range(hstart, hend));
            roi_ranges.push_back(cv::Range(wstart, wend));

            cv::Mat mask_protos = cv::Mat(mask_proto_dim, mask_proto_h * mask_proto_w, CV_32FC1, (float *)mask_proto);
            int sz[] = {1, mask_proto_dim, mask_proto_h, mask_proto_w};
            cv::Mat mask_protos_reshape = mask_protos.reshape(1, 4, sz);
            cv::Mat protos = mask_protos_reshape(roi_ranges).clone().reshape(0, {mask_proto_dim, mask_w * mask_h});
            cv::Mat mask_proposals = cv::Mat(1, mask_proto_dim, CV_32FC1, (float *)objects[i].mask_feat.data());
            cv::Mat masks_feature = (mask_proposals * protos);

            cv::exp(-masks_feature.reshape(1, {mask_h, mask_w}), mask);
            mask = 1.0 / (1.0 + mask);
        }

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
        objects[i].mask = mask > 0.5;
    }
}

static void generate_proposals_yolov7_face(int stride, const float *feat, float prob_threshold, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid)
{
    int cls_num = 1;
    int anchor_group;
    int anchor_num = 3;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    if (stride == 8) {
        anchor_group = 1;
    }

    if (stride == 16) {
        anchor_group = 2;
    }

    if (stride == 32) {
        anchor_group = 3;
    }

    auto feature_ptr = feat;

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            for (int a = 0; a <= anchor_num - 1; a++) {
                if (feature_ptr[4] < prob_threshold_unsigmoid) {
                    feature_ptr += (cls_num + 5 + 15);
                    continue;
                }

                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int s = 0; s <= cls_num - 1; s++) {
                    float score = feature_ptr[s + 5 + 15];
                    if (score > class_score) {
                        class_index = s;
                        class_score = score;
                    }
                }

                float box_score = feature_ptr[4];
                float final_score = sigmoid(box_score) * sigmoid(class_score);

                if (final_score >= prob_threshold) {
                    float dx = sigmoid(feature_ptr[0]);
                    float dy = sigmoid(feature_ptr[1]);
                    float dw = sigmoid(feature_ptr[2]);
                    float dh = sigmoid(feature_ptr[3]);
                    float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                    float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                    float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                    float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                    float pred_w = dw * dw * 4.0f * anchor_w;
                    float pred_h = dh * dh * 4.0f * anchor_h;
                    float x0 = pred_cx - pred_w * 0.5f;
                    float y0 = pred_cy - pred_h * 0.5f;
                    float x1 = pred_cx + pred_w * 0.5f;
                    float y1 = pred_cy + pred_h * 0.5f;

                    Object obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0;
                    obj.rect.height = y1 - y0;
                    obj.label = class_index;
                    obj.prob = final_score;

                    const float *landmark_ptr = feature_ptr + 6;
                    for (int l = 0; l < 5; l++) {
                        float lx = (landmark_ptr[3 * l] * 2.0f - 0.5f + w) * stride;
                        float ly = (landmark_ptr[3 * l + 1] * 2.0f - 0.5f + h) * stride;
                        obj.landmark[l] = cv::Point2f(lx, ly);
                    }

                    objects.push_back(obj);
                }

                feature_ptr += (cls_num + 5 + 15);
            }
        }
    }
}

static void generate_proposals_yolov7_palm(int stride, const float *feat, float prob_threshold, std::vector<PalmObject> &objects, int letterbox_cols, int letterbox_rows, const float *anchors, float prob_threshold_unsigmoid)
{
    int cls_num = 1;
    int anchor_group;
    int anchor_num = 3;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    if (stride == 8) {
        anchor_group = 1;
    }

    if (stride == 16) {
        anchor_group = 2;
    }

    if (stride == 32) {
        anchor_group = 3;
    }

    auto feature_ptr = feat;
    const int landmark_sort[7] = {0, 3, 4, 5, 6, 1, 2};

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            for (int a = 0; a <= anchor_num - 1; a++) {
                if (feature_ptr[4] < prob_threshold_unsigmoid) {
                    feature_ptr += (cls_num + 5 + 21);
                    continue;
                }

                float class_score = -FLT_MAX;
                for (int s = 0; s <= cls_num - 1; s++) {
                    float score = feature_ptr[s + 5 + 21];
                    if (score > class_score) {
                        class_score = score;
                    }
                }

                float box_score = feature_ptr[4];
                float final_score = sigmoid(box_score) * sigmoid(class_score);

                if (final_score >= prob_threshold) {
                    float dx = sigmoid(feature_ptr[0]);
                    float dy = sigmoid(feature_ptr[1]);
                    float dw = sigmoid(feature_ptr[2]);
                    float dh = sigmoid(feature_ptr[3]);
                    float pred_cx = (dx * 2.0f - 0.5f + w) * stride;
                    float pred_cy = (dy * 2.0f - 0.5f + h) * stride;
                    float anchor_w = anchors[(anchor_group - 1) * 6 + a * 2 + 0];
                    float anchor_h = anchors[(anchor_group - 1) * 6 + a * 2 + 1];
                    float pred_w = dw * dw * 4.0f * anchor_w;
                    float pred_h = dh * dh * 4.0f * anchor_h;
                    float x0 = pred_cx - pred_w * 0.5f;
                    float y0 = pred_cy - pred_h * 0.5f;
                    float x1 = pred_cx + pred_w * 0.5f;
                    float y1 = pred_cy + pred_h * 0.5f;

                    PalmObject obj;
                    obj.rect.x = x0 / (float)letterbox_cols;
                    obj.rect.y = y0 / (float)letterbox_rows;
                    obj.rect.width = (x1 - x0) / (float)letterbox_cols;
                    obj.rect.height = (y1 - y0) / (float)letterbox_rows;
                    obj.prob = final_score;

                    const float *landmark_ptr = feature_ptr + 6;
                    std::vector<cv::Point2f> tmp(7);
                    float min_x = FLT_MAX, min_y = FLT_MAX, max_x = 0, max_y = 0;
                    for (int l = 0; l < 7; l++) {
                        float lx = (landmark_ptr[3 * l] * 2.0f - 0.5f + w) * stride;
                        float ly = (landmark_ptr[3 * l + 1] * 2.0f - 0.5f + h) * stride;
                        lx /= (float)letterbox_cols;
                        ly /= (float)letterbox_rows;

                        tmp[l] = cv::Point2f(lx, ly);
                        min_x = lx < min_x ? lx : min_x;
                        min_y = ly < min_y ? ly : min_y;
                        max_x = lx > max_x ? lx : max_x;
                        max_y = ly > max_y ? ly : max_y;
                    }

                    float w = max_x - min_x;
                    float h = max_y - min_y;
                    float long_side = h > w ? h : w;

                    long_side *= 1.1f;
                    obj.rect.x = min_x + w * 0.5f - long_side * 0.5f;
                    obj.rect.y = min_y + h * 0.5f - long_side * 0.5f;
                    obj.rect.width = long_side;
                    obj.rect.height = long_side;
                    for (int l = 0; l < 7; l++) {
                        obj.landmarks[l] = tmp[landmark_sort[l]];
                    }

                    objects.push_back(obj);
                }

                feature_ptr += (cls_num + 5 + 21);
            }
        }
    }
}

static void generate_proposals_yolov8(int stride, const float *dfl_feat, const float *cls_feat, const float *cls_idx, float prob_threshold_unsigmoid, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, int cls_num = 80)
{
    int reg_max = 16;
    auto dfl_ptr = dfl_feat;
    auto cls_ptr = cls_feat;
    auto cls_idx_ptr = cls_idx;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    std::vector<float> dis_after_sm(reg_max, 0.f);

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            int class_index = static_cast<int>(cls_idx_ptr[h * feat_w + w]);
            float class_score = cls_ptr[h * feat_w * cls_num + w * cls_num + class_index];

            if (class_score > prob_threshold_unsigmoid) {
                float pred_ltrb[4];
                for (int k = 0; k < 4; k++) {
                    float dis = softmax(dfl_ptr + k * reg_max, dis_after_sm.data(), reg_max);
                    pred_ltrb[k] = dis * stride;
                }

                float pb_cx = (w + 0.5f) * stride;
                float pb_cy = (h + 0.5f) * stride;

                float x0 = pb_cx - pred_ltrb[0];
                float y0 = pb_cy - pred_ltrb[1];
                float x1 = pb_cx + pred_ltrb[2];
                float y1 = pb_cy + pred_ltrb[3];

                x0 = std::max(std::min(x0, (float)(letterbox_cols - 1)), 0.f);
                y0 = std::max(std::min(y0, (float)(letterbox_rows - 1)), 0.f);
                x1 = std::max(std::min(x1, (float)(letterbox_cols - 1)), 0.f);
                y1 = std::max(std::min(y1, (float)(letterbox_rows - 1)), 0.f);

                Object obj;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = x1 - x0;
                obj.rect.height = y1 - y0;
                obj.label = class_index;
                obj.prob = sigmoid(class_score);

                objects.push_back(obj);
            }

            dfl_ptr += (4 * reg_max);
        }
    }
}

static void generate_proposals_yolov8_seg(int stride, const float *dfl_feat, const float *cls_feat, const float *cls_idx, float prob_threshold_unsigmoid, std::vector<Object> &objects, int letterbox_cols, int letterbox_rows, int cls_num = 80, int mask_proto_dim = 32)
{
    int reg_max = 16;
    auto dfl_ptr = dfl_feat;
    auto cls_ptr = cls_feat;
    auto cls_idx_ptr = cls_idx;
    int feat_w = letterbox_cols / stride;
    int feat_h = letterbox_rows / stride;

    std::vector<float> dis_after_sm(reg_max, 0.f);

    for (int h = 0; h <= feat_h - 1; h++) {
        for (int w = 0; w <= feat_w - 1; w++) {
            int class_index = static_cast<int>(cls_idx_ptr[h * feat_w + w]);
            float class_score = cls_ptr[h * feat_w * cls_num + w * cls_num + class_index];

            if (class_score > prob_threshold_unsigmoid) {
                float pred_ltrb[4];
                for (int k = 0; k < 4; k++) {
                    float dis = softmax(dfl_ptr + k * reg_max, dis_after_sm.data(), reg_max);
                    pred_ltrb[k] = dis * stride;
                }

                float pb_cx = (w + 0.5f) * stride;
                float pb_cy = (h + 0.5f) * stride;

                float x0 = pb_cx - pred_ltrb[0];
                float y0 = pb_cy - pred_ltrb[1];
                float x1 = pb_cx + pred_ltrb[2];
                float y1 = pb_cy + pred_ltrb[3];

                x0 = std::max(std::min(x0, (float)(letterbox_cols - 1)), 0.f);
                y0 = std::max(std::min(y0, (float)(letterbox_rows - 1)), 0.f);
                x1 = std::max(std::min(x1, (float)(letterbox_cols - 1)), 0.f);
                y1 = std::max(std::min(y1, (float)(letterbox_rows - 1)), 0.f);

                Object obj;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = x1 - x0;
                obj.rect.height = y1 - y0;
                obj.label = class_index;
                obj.prob = sigmoid(class_score);
                obj.mask_feat.resize(mask_proto_dim);
                for (int k = 0; k < mask_proto_dim; k++) {
                    obj.mask_feat[k] = dfl_ptr[4 * reg_max + k];
                }

                objects.push_back(obj);
            }

            dfl_ptr += (4 * reg_max + mask_proto_dim);
        }
    }
}

static void generate_proposals_palm(std::vector<PalmObject> &region_list, float score_thresh, int input_img_w, int input_img_h, float *scores_ptr, float *bboxes_ptr, int head_count, const int *strides, const int *anchor_size, const float *anchor_offset, const int *feature_map_size, float prob_threshold_unsigmoid)
{
    int idx = 0;

    for (int i = 0; i < head_count; i++) {
        for (int y = 0; y < feature_map_size[i]; y++) {
            for (int x = 0; x < feature_map_size[i]; x++) {
                for (int k = 0; k < anchor_size[i]; k++) {
                    if (scores_ptr[idx] < prob_threshold_unsigmoid) {
                        idx++;
                        continue;
                    }

                    const float x_center = (x + anchor_offset[i]) * 1.0f / feature_map_size[i];
                    const float y_center = (y + anchor_offset[i]) * 1.0f / feature_map_size[i];
                    float score = sigmoid(scores_ptr[idx]);

                    if (score > score_thresh) {
                        float *p = bboxes_ptr + (idx * 18);

                        float cx = p[0] / input_img_w + x_center;
                        float cy = p[1] / input_img_h + y_center;
                        float w = p[2] / input_img_w;
                        float h = p[3] / input_img_h;

                        float x0 = cx - w * 0.5f;
                        float y0 = cy - h * 0.5f;
                        float x1 = cx + w * 0.5f;
                        float y1 = cy + h * 0.5f;

                        PalmObject region;
                        region.prob = score;
                        region.rect.x = x0;
                        region.rect.y = y0;
                        region.rect.width = x1 - x0;
                        region.rect.height = y1 - y0;

                        for (int j = 0; j < 7; j++) {
                            float lx = p[4 + (2 * j) + 0];
                            float ly = p[4 + (2 * j) + 1];
                            lx += x_center * input_img_w;
                            ly += y_center * input_img_h;
                            lx /= (float)input_img_w;
                            ly /= (float)input_img_h;

                            region.landmarks[j].x = lx;
                            region.landmarks[j].y = ly;
                        }

                        region_list.push_back(region);
                    }

                    idx++;
                }
            }
        }
    }
}

static void transform_rects_palm(PalmObject &object)
{
    float x0 = object.landmarks[0].x;
    float y0 = object.landmarks[0].y;
    float x1 = object.landmarks[2].x;
    float y1 = object.landmarks[2].y;
    float rotation = M_PI * 0.5f - std::atan2(-(y1 - y0), x1 - x0);

    float hand_cx;
    float hand_cy;
    float shift_x = 0.0f;
    float shift_y = -0.5f;

    if (rotation == 0) {
        hand_cx = object.rect.x + object.rect.width * 0.5f + (object.rect.width * shift_x);
        hand_cy = object.rect.y + object.rect.height * 0.5f + (object.rect.height * shift_y);
    } else {
        float dx = (object.rect.width * shift_x) * std::cos(rotation) - (object.rect.height * shift_y) * std::sin(rotation);
        float dy = (object.rect.width * shift_x) * std::sin(rotation) + (object.rect.height * shift_y) * std::cos(rotation);
        hand_cx = object.rect.x + object.rect.width * 0.5f + dx;
        hand_cy = object.rect.y + object.rect.height * 0.5f + dy;
    }

    float long_side = (std::max)(object.rect.width, object.rect.height);
    float dx = long_side * 1.3f;
    float dy = long_side * 1.3f;

    object.vertices[0].x = -dx;
    object.vertices[0].y = -dy;
    object.vertices[1].x = +dx;
    object.vertices[1].y = -dy;
    object.vertices[2].x = +dx;
    object.vertices[2].y = +dy;
    object.vertices[3].x = -dx;
    object.vertices[3].y = +dy;

    for (int i = 0; i < 4; i++) {
        float sx = object.vertices[i].x;
        float sy = object.vertices[i].y;
        object.vertices[i].x = sx * std::cos(rotation) - sy * std::sin(rotation);
        object.vertices[i].y = sx * std::sin(rotation) + sy * std::cos(rotation);
        object.vertices[i].x += hand_cx;
        object.vertices[i].y += hand_cy;
    }
}

static void get_out_bbox_palm(std::vector<PalmObject> &proposals, std::vector<PalmObject> &objects, const float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows, int src_cols)
{
    qsort_descent_inplace(proposals);
    std::vector<int> picked;
    nms_sorted_bboxes(proposals, picked, nms_threshold);

    int count = picked.size();
    objects.resize(count);
    for (int i = 0; i < count; i++) {
        objects[i] = proposals[picked[i]];
        transform_rects_palm(objects[i]);
    }

    int resize_rows;
    int resize_cols;
    float scale_letterbox;

    if ((letterbox_rows * 1.0 / src_rows) < (letterbox_cols * 1.0 / src_cols)) {
        scale_letterbox = letterbox_rows * 1.0 / src_rows;
    } else {
        scale_letterbox = letterbox_cols * 1.0 / src_cols;
    }

    resize_cols = int(scale_letterbox * src_cols);
    resize_rows = int(scale_letterbox * src_rows);

    int tmp_h = (letterbox_rows - resize_rows) / 2;
    int tmp_w = (letterbox_cols - resize_cols) / 2;

    float ratio_x = (float)src_cols / resize_cols;
    float ratio_y = (float)src_rows / resize_rows;

    for (auto &object : objects) {
        for (auto &vertice : object.vertices) {
            vertice.x = (vertice.x * letterbox_cols - tmp_w) * ratio_x;
            vertice.y = (vertice.y * letterbox_rows - tmp_h) * ratio_y;
        }

        for (auto &ld : object.landmarks) {
            ld.x = (ld.x * letterbox_cols - tmp_w) * ratio_x;
            ld.y = (ld.y * letterbox_rows - tmp_h) * ratio_y;
        }
    }
}
}
