#include <math.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#define __AXERA_PIPELINE_HPP_INSIDE__
#include "../../axapi.h"
#include "common_venc.hpp"
#undef __AXERA_PIPELINE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

#define VENC_MIN_WIDTH                  136
#define VENC_MIN_HEIGHT                 136

#define VENC_MAX_WIDTH                  5584
#define VENC_MAX_HEIGHT                 4188

#define QpMapBufNum                     10
#define AVC_MAX_CU_SIZE                 16
#define HEVC_MAX_CU_SIZE                64

#define REQUEST_IDR_INTERVAL            5
#define ROIMAP_PREFETCH_EXT_SIZE        1536

#define HEVC_STREAM                     1   /* 0: HEVC, 1:264 */

#define gettid()                        syscall(__NR_gettid)

#define MIN(a, b)                       ((a) < (b) ? (a) : (b))
#define CLIP3(x, y, z)                  ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))

API_END_NAMESPACE(media)
