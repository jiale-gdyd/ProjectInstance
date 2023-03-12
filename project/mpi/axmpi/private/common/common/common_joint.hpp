#pragma once

#include "../../utilities/log.hpp"
#include "../../joint/post_process.hpp"

namespace axpi {
int common_axjoint_exit(axjoint_models_t *handler);
int common_axjoint_init(axjoint_models_t *handler, int image_width, int image_height);
}
