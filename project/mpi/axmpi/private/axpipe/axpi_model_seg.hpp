#pragma once

#include <opencv2/opencv.hpp>

#include "axpi_model_base.hpp"
#include "../utilities/ringbuffer.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
class AxPiModelPPHumSeg : AxPiModelSingleBase {
protected:
    cv::Mat                   base_canvas;
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};

REGISTER(MT_SEG_PPHUMSEG, AxPiModelPPHumSeg)
}
