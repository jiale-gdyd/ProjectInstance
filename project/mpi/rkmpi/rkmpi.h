#ifndef PLATFORM_MPI_RKMPI_H
#define PLATFORM_MPI_RKMPI_H

#include <stdio.h>

#ifndef rkmpi_print
#define rkmpi_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef rkmpi_error
#define rkmpi_error(msg, ...)               rkmpi_print("\033[1;31m[MPI][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rkmpi_warn
#define rkmpi_warn(msg, ...)                rkmpi_print("\033[1;33m[MPI][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rkmpi_info
#define rkmpi_info(msg, ...)                rkmpi_print("\033[1;32m[MPI][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef rkmpi_debug
#define rkmpi_debug(msg, ...)               rkmpi_print("\033[1;34m[MPI][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
