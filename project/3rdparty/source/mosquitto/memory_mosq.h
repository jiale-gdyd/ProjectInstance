#ifndef MOSQ_MEMORY_MOSQ_H
#define MOSQ_MEMORY_MOSQ_H

#include <stdio.h>
#include <sys/types.h>

void *mosquitto__calloc(size_t nmemb, size_t size);
void mosquitto__free(void *mem);
void *mosquitto__malloc(size_t size);

void *mosquitto__realloc(void *ptr, size_t size);
char *mosquitto__strdup(const char *s);
char *mosquitto__strndup(const char *s, size_t n);

#define mosquitto__FREE(A)      do { mosquitto__free(A); (A) = NULL;} while (0)
#define SAFE_FREE(A)            do { free(A); (A) = NULL;} while (0)

#endif
