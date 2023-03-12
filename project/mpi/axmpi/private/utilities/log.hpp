#pragma once

#include <stdio.h>

namespace axpi {
#ifndef axmpi_print
#define axmpi_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef axmpi_error
#define axmpi_error(msg, ...)               axmpi_print("\033[1;31m[AXMPI][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axmpi_warn
#define axmpi_warn(msg, ...)                axmpi_print("\033[1;33m[AXMPI][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axmpi_info
#define axmpi_info(msg, ...)                axmpi_print("\033[1;32m[AXMPI][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axmpi_debug
#define axmpi_debug(msg, ...)               axmpi_print("\033[1;34m[AXMPI][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif
}
