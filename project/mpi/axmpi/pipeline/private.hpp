#ifndef MPI_AXMPI_PIPELINE_PRIVATE_HPP
#define MPI_AXMPI_PIPELINE_PRIVATE_HPP

#if !defined(__AXERA_PIPELINE_HPP_INSIDE__)
#error "can't be included directly."
#endif

#include "axpipe.hpp"

API_BEGIN_NAMESPACE(media)

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

API_END_NAMESPACE(media)

#endif
