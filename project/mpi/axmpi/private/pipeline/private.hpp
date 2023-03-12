#pragma once

#include "../../axpipe.hpp"
#include "../utilities/log.hpp"

namespace axpi {
int axpipe_release_vo();
int axpipe_create_vo(std::string str, axpipe_t *pipe);

int axpipe_create_ivps(axpipe_t *pipe);
int axpipe_release_ivps(axpipe_t *pipe);

int axpipe_create_venc(axpipe_t *pipe);
int axpipe_release_venc(axpipe_t *pipe);

int axpipe_create_vdec(axpipe_t *pipe);
int axpipe_release_vdec(axpipe_t *pipe);

int axpipe_create_jdec(axpipe_t *pipe);
int axpipe_release_jdec(axpipe_t *pipe);
}
