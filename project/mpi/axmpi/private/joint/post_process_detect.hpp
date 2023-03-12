#pragma once

#include "post_process.hpp"

namespace axpi {
int axjoint_set_param(void *jsonData);
int axjoint_parse_param(std::string confJsonFile);

void axjoint_post_process_palm_hand(axpi_results_t *results, axjoint_models_t *handler);
void axjoint_post_process_detection(axpi_results_t *results, axjoint_models_t *handler);
void axjoint_post_process_yolov5_seg(axpi_results_t *results, axjoint_models_t *handler);
void axjoint_post_process_detect_single_func(axpi_results_t *results, axjoint_models_t *handler);
}
