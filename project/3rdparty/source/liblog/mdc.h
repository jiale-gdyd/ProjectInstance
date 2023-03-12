#ifndef MDC_H
#define MDC_H

#include "logc_defs.h"

struct liblog_mdc {
    struct logc_hashtable *tab;
};

struct liblog_mdc_kv {
    char   key[LOG_MAXLEN_PATH + 1];
    char   value[LOG_MAXLEN_PATH + 1];
    size_t value_len;
};

struct liblog_mdc *liblog_mdc_new(void);
void liblog_mdc_del(struct liblog_mdc *a_mdc);
void liblog_mdc_profile(struct liblog_mdc *a_mdc, int flag);

void liblog_mdc_clean(struct liblog_mdc *a_mdc);
char *liblog_mdc_get(struct liblog_mdc *a_mdc, const char *key);
void liblog_mdc_remove(struct liblog_mdc *a_mdc, const char *key);
int liblog_mdc_put(struct liblog_mdc *a_mdc, const char *key, const char *value);

struct liblog_mdc_kv *liblog_mdc_get_kv(struct liblog_mdc *a_mdc, const char *key);

#endif
