#ifndef ___MPP_PRIVATE_H___
#define ___MPP_PRIVATE_H___

#include <linux/kconfig.h>

#if defined(CONFIG_RKMPP_MPEG2D)
#define HAVE_MPEG2D         1
#endif

#if defined(CONFIG_RKMPP_MPEG4D)
#define HAVE_MPEG4D         1
#endif

#if defined(CONFIG_RKMPP_H263D)
#define HAVE_H263D          1
#endif

#if defined(CONFIG_RKMPP_H264D)
#define HAVE_H264D          1
#endif

#if defined(CONFIG_RKMPP_H265D)
#define HAVE_H265D          1
#endif

#if defined(CONFIG_RKMPP_VP8D)
#define HAVE_VP8D          1
#endif

#if defined(CONFIG_RKMPP_VP9D)
#define HAVE_VP9D          1
#endif

#if defined(CONFIG_RKMPP_AVSD)
#define HAVE_AVSD          1
#endif

#if defined(CONFIG_RKMPP_AVS2D)
#define HAVE_AVS2D         1
#endif

#if defined(CONFIG_RKMPP_JPEGD)
#define HAVE_JPEGD         1
#endif

#if defined(CONFIG_RKMPP_AV1D)
#define HAVE_AV1D          1
#endif

#if defined(CONFIG_RKMPP_H264E)
#define HAVE_H264E         1
#endif

#if defined(CONFIG_RKMPP_JPEGE)
#define HAVE_JPEGE         1
#endif

#if defined(CONFIG_RKMPP_H265E)
#define HAVE_H265E         1
#endif

#if defined(CONFIG_RKMPP_VP8E)
#define HAVE_VP8E          1
#endif

#if defined(CONFIG_FASTPLAY_ONCE)
#define ENABLE_FASTPLAY_ONCE
#endif

#endif
