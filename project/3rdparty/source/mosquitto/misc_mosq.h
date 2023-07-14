#ifndef MOSQ_MISC_MOSQ_H
#define MOSQ_MISC_MOSQ_H

#include <stdio.h>
#include <stdbool.h>

char *misc__trimblanks(char *str);
char *fgets_extending(char **buf, int *buflen, FILE *stream);
FILE *mosquitto__fopen(const char *path, const char *mode, bool restrict_read);

int mosquitto_write_file(const char *target_path, bool restrict_read, int (*write_fn)(FILE *fptr, void *user_data), void *user_data, void (*log_fn)(const char *msg));

#endif
