#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "memory_mosq.h"

void *mosquitto__calloc(size_t nmemb, size_t size)
{
    void *mem;
    mem = calloc(nmemb, size);
    return mem;
}

void mosquitto__free(void *mem)
{
    free(mem);
}

void *mosquitto__malloc(size_t size)
{
    void *mem;
    mem = malloc(size);
    return mem;
}

void *mosquitto__realloc(void *ptr, size_t size)
{
    void *mem;
    mem = realloc(ptr, size);
    return mem;
}

char *mosquitto__strdup(const char *s)
{
    char *str;
    str = strdup(s);
    return str;
}

char *mosquitto__strndup(const char *s, size_t n)
{
    char *str;
#ifdef REAL_WITH_MEMORY_TRACKING
    if (mem_limit && ((memcount + strlen(s)) > mem_limit)) {
        return NULL;
    }
#endif

    str = strndup(s, n);

#ifdef REAL_WITH_MEMORY_TRACKING
    if (str) {
        memcount += malloc_usable_size(str);
        if (memcount > max_memcount) {
            max_memcount = memcount;
        }
    }
#endif

    return str;
}