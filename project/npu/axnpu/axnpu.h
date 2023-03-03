#ifndef NPU_AXNPU_AXNPU_H
#define NPU_AXNPU_AXNPU_H

#include <stdio.h>

#ifndef axnpu_print
#define axnpu_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef axnpu_error
#define axnpu_error(msg, ...)               axnpu_print("\033[1;31m[AXNPU][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axnpu_warn
#define axnpu_warn(msg, ...)                axnpu_print("\033[1;33m[AXNPU][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axnpu_info
#define axnpu_info(msg, ...)                axnpu_print("\033[1;32m[AXNPU][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axnpu_debug
#define axnpu_debug(msg, ...)               axnpu_print("\033[1;34m[AXNPU][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
