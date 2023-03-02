#ifndef PLATFORM_NPU_PRIVATE_H
#define PLATFORM_NPU_PRIVATE_H

#include <stdio.h>

#ifndef npu_print
#define npu_print(msg, ...)                 fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef npu_error
#define npu_error(msg, ...)                 npu_print("\033[1;31m[NPU][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef npu_warn
#define npu_warn(msg, ...)                  npu_print("\033[1;33m[NPU][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef npu_info
#define npu_info(msg, ...)                  npu_print("\033[1;32m[NPU][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef npu_debug
#define npu_debug(msg, ...)                 npu_print("\033[1;34m[NPU][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
