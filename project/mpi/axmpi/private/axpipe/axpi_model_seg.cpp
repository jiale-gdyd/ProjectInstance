#include "axpi_model_seg.hpp"

namespace axpi {
int AxPiModelPPHumSeg::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    results->bPPHumSeg = 1;
    auto ptr = (float *)mRunner->getSpecOutput(0).pVirAddr;
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT);
    }

    int seg_h = mRunner->getSpecOutput(0).shape[2];
    int seg_w = mRunner->getSpecOutput(0).shape[3];
    int seg_size = seg_h * seg_w;

    cv::Mat &seg_mat = mSimpleRingBuffer.next();
    if (seg_mat.empty()) {
        seg_mat = cv::Mat(seg_h, seg_w, CV_8UC1);
    }

    results->PPHumSeg.h = seg_h;
    results->PPHumSeg.w = seg_w;
    results->PPHumSeg.data = seg_mat.data;

    for (int j = 0; j < seg_h * seg_w; ++j) {
        results->PPHumSeg.data[j] = (ptr[j] < ptr[j + seg_size]) ? 255 : 0;
    }

    return 0;
}

void AxPiModelPPHumSeg::drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (!results->bPPHumSeg || !results->PPHumSeg.data) {
        return;
    }

    if (base_canvas.empty() || ((base_canvas.rows * base_canvas.cols) < (image.rows * image.cols))) {
        base_canvas = cv::Mat(image.rows, image.cols, CV_8UC1);
    }

    cv::Mat tmp(image.rows, image.cols, CV_8UC1, base_canvas.data);
    cv::Mat mask(results->PPHumSeg.h, results->PPHumSeg.w, CV_8UC1, results->PPHumSeg.data);
    cv::resize(mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
    image.setTo(cv::Scalar(66, 0, 0, 128), tmp);
}
}
