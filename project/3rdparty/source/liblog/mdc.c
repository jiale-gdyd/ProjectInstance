#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "mdc.h"
#include "logc_defs.h"

void liblog_mdc_profile(struct liblog_mdc *a_mdc, int flag)
{
    struct liblog_mdc_kv *a_mdc_kv;
    struct logc_hashtable_entry *a_entry;

    logc_assert(a_mdc, );
    logc_profile(flag, "---mdc[%p]---", a_mdc);

    logc_hashtable_foreach(a_mdc->tab, a_entry) {
        a_mdc_kv = (struct liblog_mdc_kv *)(a_entry->value);
        logc_profile(flag, "----mdc_kv[%p][%s]-[%s]----", a_mdc_kv, a_mdc_kv->key, a_mdc_kv->value);
    }
}

void liblog_mdc_del(struct liblog_mdc *a_mdc)
{
    logc_assert(a_mdc, );
    if (a_mdc->tab) {
        logc_hashtable_del(a_mdc->tab);
    }

    logc_debug("liblog_mdc_del[%]", a_mdc);
    free(a_mdc);
}

static void liblog_mdc_kv_del(struct liblog_mdc_kv *a_mdc_kv)
{
    logc_debug("liblog_mdc_kv_del[%p]", a_mdc_kv);
    free(a_mdc_kv);
}

static struct liblog_mdc_kv *liblog_mdc_kv_new(const char *key, const char *value)
{
    struct liblog_mdc_kv *a_mdc_kv;

    a_mdc_kv = (struct liblog_mdc_kv *)calloc(1, sizeof(struct liblog_mdc_kv));
    if (!a_mdc_kv) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    snprintf(a_mdc_kv->key, sizeof(a_mdc_kv->key), "%s", key);
    a_mdc_kv->value_len = snprintf(a_mdc_kv->value, sizeof(a_mdc_kv->value), "%s", value);

    return a_mdc_kv;
}

struct liblog_mdc *liblog_mdc_new(void)
{
    struct liblog_mdc *a_mdc;

    a_mdc = (struct liblog_mdc *)calloc(1, sizeof(struct liblog_mdc));
    if (!a_mdc) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    a_mdc->tab = logc_hashtable_new(20, logc_hashtable_str_hash, logc_hashtable_str_equal, NULL, (hashtable_del_func)liblog_mdc_kv_del);
    if (!a_mdc->tab) {
        logc_error("libc_hashtable_new fail");
        goto err;
    }

    return a_mdc;

err:
    liblog_mdc_del(a_mdc);
    return NULL;
}

int liblog_mdc_put(struct liblog_mdc *a_mdc, const char *key, const char *value)
{
    struct liblog_mdc_kv *a_mdc_kv;

    a_mdc_kv = liblog_mdc_kv_new(key, value);
    if (!a_mdc_kv) {
        logc_error("liblog_mdc_kv_new failed");
        return -1;
    }

    if (logc_hashtable_put(a_mdc->tab, a_mdc_kv->key, a_mdc_kv)) {
        logc_error("logc_hashtable_put fail");
        liblog_mdc_kv_del(a_mdc_kv);

        return -1;
    }

    return 0;
}

void liblog_mdc_clean(struct liblog_mdc *a_mdc)
{
    logc_hashtable_clean(a_mdc->tab);
}

char *liblog_mdc_get(struct liblog_mdc *a_mdc, const char *key)
{
    struct liblog_mdc_kv *a_mdc_kv;

    a_mdc_kv = (struct liblog_mdc_kv *)logc_hashtable_get(a_mdc->tab, key);
    if (!a_mdc_kv) {
        logc_error("logc_hashtable_get fail");
        return NULL;
    } else {
        return a_mdc_kv->value;
    }
}

struct liblog_mdc_kv *liblog_mdc_get_kv(struct liblog_mdc *a_mdc, const char *key)
{
    struct liblog_mdc_kv *a_mdc_kv;

    a_mdc_kv = (struct liblog_mdc_kv *)logc_hashtable_get(a_mdc->tab, key);
    if (!a_mdc_kv) {
        logc_error("logc_hashtable_get fail");
        return NULL;
    } else {
        return a_mdc_kv;
    }
}

void liblog_mdc_remove(struct liblog_mdc *a_mdc, const char *key)
{
    logc_hashtable_remove(a_mdc->tab, key);
}
