#pragma once

#include <string>
#include <vector>

#include "axpi_model_base.hpp"
#include "../utilities/ringbuffer.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
class AxPiModelCrowdCount : public AxPiModelSingleBase {
protected:
    std::string                                 info;
    int                                         width_anchor = 0;
    int                                         height_anchor = 0;
    std::vector<float>                          all_anchor_points;
    SimpleRingBuffer<std::vector<axpi_point_t>> mSimpleRingBuffer;

    int post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results) override;
    void drawCustom(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};

REGISTER(MT_DET_CROWD_COUNT, AxPiModelCrowdCount)
}
