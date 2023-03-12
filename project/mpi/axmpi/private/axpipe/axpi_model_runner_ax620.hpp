#pragma once

#include "axpi_model_base.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
class AxPiRunnerAx620 : public AxPiRunnerBase {
protected:
    struct axjoint_runner_ax620_handle_t *m_handle = nullptr;

public:
    int exit() override;
    int init(std::string modelFile) override;

    int getAlgoWidth() override;
    int getAlgoHeight() override;
    int getColorSpace() override;

    int inference(axpi_image_t *pstFrame, const axpi_bbox_t *crop_resize_box) override;
};

REGISTER(RUNNER_AX620, AxPiRunnerAx620)
}
