#ifndef PLATFORM_MPI_PRIVATE_H
#define PLATFORM_MPI_PRIVATE_H

#include <stdio.h>

#ifndef mpi_print
#define mpi_print(msg, ...)                 fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef mpi_error
#define mpi_error(msg, ...)                 mpi_print("\033[1;31m[MPI][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef mpi_warn
#define mpi_warn(msg, ...)                  mpi_print("\033[1;33m[MPI][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef mpi_info
#define mpi_info(msg, ...)                  mpi_print("\033[1;32m[MPI][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef mpi_debug
#define mpi_debug(msg, ...)                 mpi_print("\033[1;34m[MPI][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
