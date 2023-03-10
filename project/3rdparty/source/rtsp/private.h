#ifndef RTSP_RTSP_PRIVATE_H
#define RTSP_RTSP_PRIVATE_H

#include <stdio.h>

#ifndef rtsp_print
#define rtsp_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef rtsp_error
#define rtsp_error(msg, ...)               rtsp_print("\033[1;31m[RTSP][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rtsp_warn
#define rtsp_warn(msg, ...)                rtsp_print("\033[1;33m[RTSP][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rtsp_info
#define rtsp_info(msg, ...)                rtsp_print("\033[1;32m[RTSP][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rtsp_debug
#define rtsp_debug(msg, ...)               rtsp_print("\033[1;34m[RTSP][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
