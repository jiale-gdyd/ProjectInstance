#ifndef MPI_AXMPI_MAIX_MAIX_PRIVATE_HPP
#define MPI_AXMPI_MAIX_MAIX_PRIVATE_HPP

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <utils/export.h>

#ifndef maxix_print
#define maxix_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef maxix_error
#define maxix_error(msg, ...)               maxix_print("\033[1;31m[MAIX][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef maxix_warn
#define maxix_warn(msg, ...)                maxix_print("\033[1;33m[MAIX][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef maxix_info
#define maxix_info(msg, ...)                maxix_print("\033[1;32m[MAIX][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef maxix_debug
#define maxix_debug(msg, ...)               maxix_print("\033[1;34m[MAIX][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
