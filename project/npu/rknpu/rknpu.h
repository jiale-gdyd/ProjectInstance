#ifndef NPU_RKNPU_RKNPU_H
#define NPU_RKNPU_RKNPU_H

#include <stdio.h>

#ifndef rknpu_print
#define rknpu_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef rknpu_error
#define rknpu_error(msg, ...)               rknpu_print("\033[1;31m[RKNPU][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rknpu_warn
#define rknpu_warn(msg, ...)                rknpu_print("\033[1;33m[RKNPU][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rknpu_info
#define rknpu_info(msg, ...)                rknpu_print("\033[1;32m[RKNPU][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rknpu_debug
#define rknpu_debug(msg, ...)               rknpu_print("\033[1;34m[RKNPU][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
