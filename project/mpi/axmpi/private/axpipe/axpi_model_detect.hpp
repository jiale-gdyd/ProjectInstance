#pragma once

#include "../base/yolo.hpp"
#include "axpi_model_base.hpp"
#include "../base/detection.hpp"
#include "../utilities/ringbuffer.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
class AxPiModelYolov5 : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV5, AxPiModelYolov5)

class AxPiModelYolov5Seg : public AxPiModelSingleBase {
protected:
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};
REGISTER(MT_INSEG_YOLOV5_MASK, AxPiModelYolov5Seg)

class AxPiModelYolov5Face : public AxPiModelSingleBase {
protected:
    SimpleRingBuffer<std::vector<axpi_point_t>> mSimpleRingBuffer;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};
REGISTER(MT_DET_YOLOV5_FACE, AxPiModelYolov5Face)

class AxPiModelYolov5LisencePlate : public AxPiModelSingleBase {
protected:
    SimpleRingBuffer<std::vector<axpi_point_t>> mSimpleRingBuffer;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV5_LICENSE_PLATE, AxPiModelYolov5LisencePlate)

class AxPiModelYolov6 : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV6, AxPiModelYolov6)

class AxPiModelYolov7 : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV7, AxPiModelYolov7)

class AxPiModelYolov7Face : public AxPiModelYolov5Face {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV7_FACE, AxPiModelYolov7Face)

class AxPiModelYolov7FacePlamHand : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV7_PALM_HAND, AxPiModelYolov7FacePlamHand)

class AxPiModelPlamHand : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_PALM_HAND, AxPiModelPlamHand)

class AxPiModelYolox : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOX, AxPiModelYolox)

class AxPiModelYoloxPPL : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOX_PPL, AxPiModelYoloxPPL)

class AxPiModelYoloPV2 : public AxPiModelSingleBase {
protected:
    cv::Mat                   base_canvas;
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg;
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_ll;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};
REGISTER(MT_DET_YOLOPV2, AxPiModelYoloPV2)

class AxPiModelYoloFastBody : public AxPiModelSingleBase {
protected:
    yolo::YoloDetectionOutput yolo{};
    std::vector<yolo::TMat>   yolo_inputs, yolo_outputs;
    std::vector<float>        output_buf;
    bool                      bInit = false;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLO_FASTBODY, AxPiModelYoloFastBody)

class AxPiModelNanoDetect : public AxPiModelSingleBase {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_NANODET, AxPiModelNanoDetect)

class AxPiModelSCRFD : public AxPiModelYolov5Face {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_SCRFD, AxPiModelSCRFD)

class AxPiModelYolov8 : public AxPiModelYolov5 {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV8, AxPiModelYolov8)

class AxPiModelYolov8Seg : public AxPiModelYolov5Seg {
protected:
    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
};
REGISTER(MT_DET_YOLOV8_SEG, AxPiModelYolov8Seg)
}
