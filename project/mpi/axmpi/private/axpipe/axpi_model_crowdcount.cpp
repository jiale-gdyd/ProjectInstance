#include "../utilities/log.hpp"
#include "../base/detection.hpp"
#include "axpi_model_crowdcount.hpp"

namespace axpi {
static void shift(int w, int h, int stride, std::vector<float> anchor_points, std::vector<float> &shifted_anchor_points)
{
    std::vector<float> x_, y_;

    for (int i = 0; i < w; i++) {
        float x = (i + 0.5) * stride;
        x_.push_back(x);
    }

    for (int i = 0; i < h; i++) {
        float y = (i + 0.5) * stride;
        y_.push_back(y);
    }

    std::vector<float> shift_x(w * h, 0), shift_y(w * h, 0);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            shift_x[i * w + j] = x_[j];
        }
    }

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            shift_y[i * w + j] = y_[i];
        }
    }

    std::vector<float> shifts(w * h * 2, 0);
    for (int i = 0; i < w * h; i++) {
        shifts[i * 2] = shift_x[i];
        shifts[i * 2 + 1] = shift_y[i];
    }

    shifted_anchor_points.resize(2 * w * h * anchor_points.size() / 2, 0);
    for (int i = 0; i < w * h; i++) {
        for (int j = 0; j < (int)anchor_points.size() / 2; j++) {
            float x = anchor_points[j * 2] + shifts[i * 2];
            float y = anchor_points[j * 2 + 1] + shifts[i * 2 + 1];
            shifted_anchor_points[i * anchor_points.size() / 2 * 2 + j * 2] = x;
            shifted_anchor_points[i * anchor_points.size() / 2 * 2 + j * 2 + 1] = y;
        }
    }
}
static void generate_anchor_points(int stride, int row, int line, std::vector<float> &anchor_points)
{
    float row_step = (float)stride / row;
    float line_step = (float)stride / line;

    std::vector<float> x_, y_;
    for (int i = 1; i < line + 1; i++) {
        float x = (i - 0.5) * line_step - stride / 2;
        x_.push_back(x);
    }

    for (int i = 1; i < row + 1; i++) {
        float y = (i - 0.5) * row_step - stride / 2;
        y_.push_back(y);
    }

    std::vector<float> shift_x(row * line, 0), shift_y(row * line, 0);
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < line; j++) {
            shift_x[i * line + j] = x_[j];
        }
    }

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < line; j++)   {
            shift_y[i * line + j] = y_[i];
        }
    }

    anchor_points.resize(row * line * 2, 0);
    for (int i = 0; i < row * line; i++) {
        float x = shift_x[i];
        float y = shift_y[i];
        anchor_points[i * 2] = x;
        anchor_points[i * 2 + 1] = y;
    }
}

static void generate_anchor_points(int img_w, int img_h, std::vector<int> pyramid_levels, int row, int line, std::vector<float> &all_anchor_points)
{
    std::vector<int> strides;
    std::vector<std::pair<int, int>> image_shapes;

    for (int i = 0; i < (int)pyramid_levels.size(); i++) {
        int new_h = std::floor((img_h + std::pow(2, pyramid_levels[i]) - 1) / std::pow(2, pyramid_levels[i]));
        int new_w = std::floor((img_w + std::pow(2, pyramid_levels[i]) - 1) / std::pow(2, pyramid_levels[i]));
        image_shapes.push_back(std::make_pair(new_w, new_h));
        strides.push_back(std::pow(2, pyramid_levels[i]));
    }

    all_anchor_points.clear();
    for (int i = 0; i < (int)pyramid_levels.size(); i++) {
        std::vector<float> anchor_points;
        generate_anchor_points(std::pow(2, pyramid_levels[i]), row, line, anchor_points);
        std::vector<float> shifted_anchor_points;
        shift(image_shapes[i].first, image_shapes[i].second, strides[i], anchor_points, shifted_anchor_points);
        all_anchor_points.insert(all_anchor_points.end(), shifted_anchor_points.begin(), shifted_anchor_points.end());
    }
}

int AxPiModelCrowdCount::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (width_anchor != getAlgoWidth() || height_anchor != getAlgoHeight()) {
        std::vector<int> pyramid_levels(1, 3);
        generate_anchor_points(getAlgoWidth(), getAlgoHeight(), pyramid_levels, 2, 2, all_anchor_points);
    }

    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT);
    }

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

    const axpi_runner_tensor_t *pOutputsInfo = mRunner->getOutputsPtr();

    axpi_point_t *pred_points_ptr = (axpi_point_t *)pOutputsInfo[0].pVirAddr;
    axpi_point_t *pred_scores_ptr = (axpi_point_t *)pOutputsInfo[1].pVirAddr;

    int len = pOutputsInfo[0].size / sizeof(float) / 2;
    axpi_point_t *anchor_points_ptr = (axpi_point_t *)all_anchor_points.data();

    std::vector<float> _softmax_result(2, 0);

    auto &vCrowdCount = mSimpleRingBuffer.next();
    vCrowdCount.clear();
    vCrowdCount.reserve(100);

    for (int i = 0; i < len; i++) {
        if (pred_scores_ptr[i].x < pred_scores_ptr[i].y) {
            detect::softmax(&pred_scores_ptr[i].x, _softmax_result.data(), 2);
            if (_softmax_result[1] > mProbThreshold) {
                axpi_point_t p;

                p.x = pred_points_ptr[i].x * 100 + anchor_points_ptr[i].x;
                p.y = pred_points_ptr[i].y * 100 + anchor_points_ptr[i].y;
                p.x = (p.x - tmp_w) * ratio_x;
                p.y = (p.y - tmp_h) * ratio_y;
                vCrowdCount.push_back(p);
            }
        }
    }

    results->crowdCountPts = vCrowdCount;
    return 0;
}

void AxPiModelCrowdCount::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    info = "real-time count of people:%" + std::to_string(results->crowdCountPts.size());

    cv::Size label_size = cv::getTextSize(info, cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, thickness * 2, NULL);
    cv::putText(image, info, cv::Point(0, label_size.height * 2), cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, cv::Scalar(255, 0, 0, 255), thickness * 2);

    for (size_t i = 0; i < results->crowdCountPts.size(); i++) {
        cv::Point p(results->crowdCountPts[i].x * image.cols + offset_x, results->crowdCountPts[i].y * image.rows + offset_y);
        cv::circle(image, p, 3, cv::Scalar(255, 0, 255, 0), 3);
    }
}
}
