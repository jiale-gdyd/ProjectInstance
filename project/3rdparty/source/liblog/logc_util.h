#ifndef LIBC_UTIL_H
#define LIBC_UTIL_H

#include <sys/types.h>

#define logc_max(a, b)          ((a) > (b) ? (a) : (b))
#define logc_min(a, b)          ((a) < (b) ? (a) : (b))

size_t logc_parse_byte_size(char *astring);
int logc_str_replace_env(char *str, size_t str_size);

#endif
