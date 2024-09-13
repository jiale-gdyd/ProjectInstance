#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "mdc.h"
#include "conf.h"
#include "rule.h"
#include "fmacros.h"
#include "logc_defs.h"
#include "record_table.h"
#include "category_table.h"

#include <liblog/liblog.h>

#define LIBLOG_VERSION          "1.2.14"

struct liblog_conf *liblog_env_conf;

#if defined(CONFIG_LIBLOG)
static int liblog_env_is_init = 0;
static int liblog_env_init_version = 0;
static pthread_key_t liblog_thread_key;
static size_t liblog_env_reload_conf_count;
static struct logc_hashtable *liblog_env_records;
static struct logc_hashtable *liblog_env_categories;
static struct liblog_category *liblog_default_category;
static struct liblog_category *liblog_category_hanlde = NULL;
static pthread_rwlock_t liblog_env_lock = PTHREAD_RWLOCK_INITIALIZER;

static void liblog_finish_inner(void)
{
    if (liblog_env_categories) {
        liblog_category_table_del(liblog_env_categories);
    }

    liblog_env_categories = NULL;
    liblog_default_category = NULL;

    if (liblog_env_records) {
        liblog_record_table_del(liblog_env_records);
    }
    liblog_env_records = NULL;

    if (liblog_env_conf) {
        liblog_conf_del(liblog_env_conf);
    }
    liblog_env_conf = NULL;

    liblog_category_hanlde = NULL;
}

static void liblog_clean_rest_thread(void)
{
    struct liblog_thread *a_thread;

    a_thread = (struct liblog_thread *)pthread_getspecific(liblog_thread_key);
    if (!a_thread) {
        return;
    }

    liblog_thread_del(a_thread);
}

static int liblog_init_inner_from_string(const char *config_string)
{
    int rc = 0;

    if (liblog_env_init_version == 0) {
        rc = pthread_key_create(&liblog_thread_key, (void (*)(void *))liblog_thread_del);
        if (rc) {
            logc_error("pthread_key_create fail, rc[%d]", rc);
            goto err;
        }

        rc = atexit(liblog_clean_rest_thread);
        if (rc) {
            logc_error("atexit fail, rc:[%d]", rc);
            goto err;
        }

        liblog_env_init_version++;
    }

    liblog_env_conf = liblog_conf_new_from_string(config_string);
    if (!liblog_env_conf) {
        logc_error("liblog_conf_new:[%s] fail", config_string);
        goto err;
    }

    liblog_env_categories = liblog_category_table_new();
    if (!liblog_env_categories) {
        logc_error("liblog_category_table_new fail");
        goto err;
    }

    liblog_env_records = liblog_record_table_new();
    if (!liblog_env_records) {
        logc_error("liblog_record_table_new fail");
        goto err;
    }

    return 0;

err:
    liblog_finish_inner();
    return -1;
}

static int liblog_init_inner(const char *confpath)
{
    int rc = 0;

    if (liblog_env_init_version == 0) {
        rc = pthread_key_create(&liblog_thread_key, (void (*)(void *))liblog_thread_del);
        if (rc) {
            logc_error("pthread_key_create fail, rc[%d]", rc);
            goto err;
        }

        rc = atexit(liblog_clean_rest_thread);
        if (rc) {
            logc_error("atexit fail, rc[%d]", rc);
            goto err;
        }

        liblog_env_init_version++;
    }

    liblog_env_conf = liblog_conf_new(confpath);
    if (!liblog_env_conf) {
        logc_error("liblog_conf_new[%s] fail", confpath);
        goto err;
    }

    liblog_env_categories = liblog_category_table_new();
    if (!liblog_env_categories) {
        logc_error("liblog_category_table_new fail");
        goto err;
    }

    liblog_env_records = liblog_record_table_new();
    if (!liblog_env_records) {
        logc_error("liblog_env_records fail");
        goto err;
    }

    return 0;

err:
    liblog_finish_inner();
    return -1;
}

static void liblog_env_setting_inner(void)
{
    setenv("LOG_PROFILE_ERROR", "/dev/stderr", 1);
    setenv("LOG_CHECK_FORMAT_RULE", "1", 1);
}
#endif

struct liblog_category *liblog_get_category_handle(void)
{
#if defined(CONFIG_LIBLOG)
    return liblog_category_hanlde;
#else
    return NULL;
#endif
}

#if defined(CONFIG_LIBLOG)
void liblog_set_category_handle(struct liblog_category *handle)
{
    liblog_category_hanlde = handle;
}
#endif

int liblog_init(const char *config_file)
{
#if defined(CONFIG_LIBLOG)
    int rc;

    logc_debug("------liblog_init start------");
    logc_debug("------[%s %s]------", __DATE__, __TIME__);

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return -1;
    }

    liblog_env_setting_inner();

    if (liblog_env_is_init) {
        logc_error("already init, use liblog_reload please");
        goto err;
    }

    if (liblog_init_inner(config_file)) {
        logc_error("liblog_init_inner[%s] fail", config_file);
        goto err;
    }

    liblog_env_is_init = 1;
    liblog_env_init_version++;

    logc_debug("------liblog_init success end------");

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }

    return 0;

err:
    logc_error("------liblog_init fail end------");
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
    }
#endif

    return -1;
}

int liblog_init_from_string(const char *config_string)
{
#if defined(CONFIG_LIBLOG)
    int rc;

    logc_debug("------liblog_init_from_string start------");
    logc_debug("------compile time:[%s %s], version:[%s]------", __DATE__, __TIME__, LIBLOG_VERSION);
    logc_debug("------compile time:[%s %s], version:[%s]------", __DATE__, __TIME__, LIBLOG_VERSION);

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc:[%d]", rc);
        return -1;
    }

    if (liblog_env_is_init) {
        logc_error("already init, use liblog_reload pls");
        goto err;
    }

    if (liblog_init_inner_from_string(config_string)) {
        logc_error("liblog_init_inner_from_string:[%s] fail", config_string);
        goto err;
    }

    liblog_env_is_init = 1;
    liblog_env_init_version++;

    logc_debug("------liblog_init_from_string success end------");

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc:[%d]", rc);
        return -1;
    }

    return 0;

err:
    logc_error("------liblog_init_from_string fail end------");
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc:[%d]", rc);
        return -1;
    }
#endif

    return -1;
}

int liblog_default_init(const char *config_file, const char *cname)
{
#if defined(CONFIG_LIBLOG)
    int rc;

    logc_debug("------liblog_default_init start------");
    logc_debug("------compile time[%s %s] version[%s]------", __DATE__, __TIME__, LIBLOG_VERSION);

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return -1;
    }

    if (liblog_env_is_init) {
        logc_error("already init, use liblog_reload please");
        goto err;
    }

    if (liblog_init_inner(config_file)) {
        logc_error("liblog_init_inner[%s] fail", config_file);
        goto err;
    }

    liblog_default_category = liblog_category_table_fetch_category(liblog_env_categories, cname, liblog_env_conf->rules);
    if (!liblog_default_category) {
        logc_error("liblog_category_table_fetch_category[%s] fail", cname);
        goto err;
    }

    liblog_env_is_init = 1;
    liblog_env_init_version++;

    logc_debug("------liblog_default_init success end------");

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }

    return 0;

err:
    logc_error("------liblog_default_init fail end------");
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
    }
#endif

    return -1;
}

int liblog_reload(const char *config_file)
{
#if defined(CONFIG_LIBLOG)
    struct liblog_rule *a_rule;
    int i = 0, rc = 0, c_up = 0;
    struct liblog_conf *new_conf = NULL;

    logc_debug("------liblog_reload start------");

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return -1;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto quit;
    }

    if (config_file == NULL) {
        config_file = liblog_env_conf->file;
    }

    if (config_file == (char *)-1) {
        if (liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period) {
            config_file = liblog_env_conf->file;
        } else {
            goto quit;
        }
    }

    liblog_env_reload_conf_count = 0;

    new_conf = liblog_conf_new(config_file);
    if (!new_conf) {
        logc_error("liblog_conf_new fail");
        goto err;
    }

    logc_arraylist_foreach(new_conf->rules, i, a_rule) {
        liblog_rule_set_record((struct liblog_rule *)a_rule, liblog_env_records);
    }

    if (liblog_category_table_update_rules(liblog_env_categories, new_conf->rules)) {
        c_up = 0;
        logc_error("liblog_category_table_update_rules fail");

        goto err;
    } else {
        c_up = 1;
    }

    liblog_env_init_version++;

    if (c_up) {
        liblog_category_table_commit_rules(liblog_env_categories);
    }

    liblog_conf_del(liblog_env_conf);
    liblog_env_conf = new_conf;

    logc_debug("------liblog_reload success, totoal init version[%d]------", liblog_env_init_version);

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }

    return 0;

err:
    logc_warn("liblog_reload fail, use old conf file, still working");

    if (new_conf) {
        liblog_conf_del(new_conf);
    }

    if (c_up) {
        liblog_category_table_rollback_rules(liblog_env_categories);
    }

    logc_error("------liblog_reload fail, totoal init version[%d]------", liblog_env_init_version);

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }

    return -1;

quit:
    logc_debug("------liblog_reload do nothing------");
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }
#endif

    return 0;
}

int liblog_reload_from_string(const char *conf_string)
{
#if defined(CONFIG_LIBLOG)

    int i = 0;
    int rc = 0;
    int c_up = 0;
    struct liblog_rule *a_rule;
    struct liblog_conf *new_conf = NULL;

    logc_debug("------liblog_reload_from_string start------");

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc:[%d]", rc);
        return -1;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto quit;
    }

    if (conf_string == NULL) {
        goto quit;
    }

    new_conf = liblog_conf_new_from_string(conf_string);
    if (!new_conf) {
        logc_error("liblog_conf_new_from_string fail");
        goto err;
    }

    logc_arraylist_foreach(new_conf->rules, i, a_rule) {
        liblog_rule_set_record(a_rule, liblog_env_records);
    }

    if (liblog_category_table_update_rules(liblog_env_categories, new_conf->rules)) {
        c_up = 0;
        logc_error("liblog_category_table_update fail");
        goto err;
    } else {
        c_up = 1;
    }

    liblog_env_init_version++;

    if (c_up) {
        liblog_category_table_commit_rules(liblog_env_categories);
    }

    liblog_conf_del(liblog_env_conf);
    liblog_env_conf = new_conf;

    logc_debug("------liblog_reload_from_string success, total init verison:[%d] ------", liblog_env_init_version);

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc:[%d]", rc);
        return -1;
    }

    return 0;

err:
    logc_warn("liblog_reload_from_string fail, use old conf file, still working");

    if (new_conf) {
        liblog_conf_del(new_conf);
    }

    if (c_up) {
        liblog_category_table_rollback_rules(liblog_env_categories);
    }

    logc_error("------zlog_reloliblog_reload_from_stringad fail, total init version:[%d] ------", liblog_env_init_version);

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc:[%d]", rc);
        return -1;
    }

    return -1;

quit:
    logc_debug("------zlog_reloliblog_reload_from_stringad do nothing------");

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc:[%d]", rc);
        return -1;
    }

    return 0;
#else
    return -1;
#endif
}

void liblog_exit(void)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;

    logc_debug("------liblog_exit start------");

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return;
    }

    if (!liblog_env_is_init) {
        logc_error("before exit, must liblog_init() or liblog_default_init() first");
        goto exit;
    }

    liblog_finish_inner();
    liblog_env_is_init = 0;

exit:
    logc_debug("------liblog_exit end------");
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return;
    }
#endif
}

struct liblog_category *liblog_get_category(const char *cname)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;
    struct liblog_category *a_category = NULL;

    logc_assert(cname, NULL);
    logc_debug("------liblog_get_category[%s] start------", cname);

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return NULL;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        a_category = NULL;

        goto err;
    }

    a_category = liblog_category_table_fetch_category(liblog_env_categories, cname, liblog_env_conf->rules);
    if (!a_category) {
        logc_error("liblog_category_table_fetch_category[%s] fail", cname);
        goto err;
    }

    logc_debug("------liblog_get_category[%s] success, end------", cname);

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return NULL;
    }

    return a_category;

err:
    logc_error("------liblog_get_category[%s] fail, end------ ", cname);
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
    }
#endif

    return NULL;
}

int liblog_default_set_category(const char *cname)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;

    logc_assert(cname, -1);

    logc_debug("------liblog_default_set_category[%s] start------", cname);

    rc = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return -1;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto err;
    }

    liblog_default_category = liblog_category_table_fetch_category(liblog_env_categories, cname, liblog_env_conf->rules);
    if (!liblog_default_category) {
        logc_error("liblog_category_table_fetch_category[%s] fail", cname);
        goto err;
    }

    logc_debug("------liblog_default_set_category[%s] end, success------", cname);

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }

    return 0;

err:
    logc_error("------liblog_default_set_category[%s] end, fail------", cname);
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
    }
#endif

    return -1;
}

#if defined(CONFIG_LIBLOG)
#define liblog_fetch_thread(a_thread, fail_goto) do {                       \
    int rd = 0;                                                             \
    a_thread = (struct liblog_thread *)pthread_getspecific(liblog_thread_key);   \
    if (!a_thread) {                                                        \
        a_thread = liblog_thread_new(liblog_env_init_version, liblog_env_conf->buf_size_min, liblog_env_conf->buf_size_max, liblog_env_conf->time_cache_count); \
        if (!a_thread) {                                                    \
            logc_error("liblog_thread_new fail");                           \
            goto fail_goto;                                                 \
        }                                                                   \
                                                                            \
        rd = pthread_setspecific(liblog_thread_key, a_thread);              \
        if (rd) {                                                           \
            liblog_thread_del(a_thread);                                    \
            logc_error("pthread_setspecific fail, rd[%d]", rd);             \
            goto fail_goto;                                                 \
        }                                                                   \
    }                                                                       \
                                                                            \
    if (a_thread->init_version != liblog_env_init_version) {                \
        rd = liblog_thread_rebuild_msg_buf(a_thread, liblog_env_conf->buf_size_min, liblog_env_conf->buf_size_max);  \
        if (rd) {                                                           \
            logc_error("liblog_thread_resize_msg_buf fail, rd[%d]", rd);    \
            goto fail_goto;                                                 \
        }                                                                   \
                                                                            \
        rd = liblog_thread_rebuild_event(a_thread, liblog_env_conf->time_cache_count);                                \
        if (rd) {                                                           \
            logc_error("liblog_thread_resize_msg_buf fail, rd[%d]", rd);    \
            goto fail_goto;                                                 \
        }                                                                   \
        a_thread->init_version = liblog_env_init_version;                   \
    }                                                                       \
} while (0)
#endif

int liblog_put_mdc(const char *key, const char *value)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;
    struct liblog_thread *a_thread;

    logc_assert(key, -1);
    logc_assert(value, -1);

    rc = pthread_rwlock_rdlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return -1;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto err;
    }

    liblog_fetch_thread(a_thread, err);

    if (liblog_mdc_put(a_thread->mdc, key, value)) {
        logc_error("liblog_mdc_put fail, key[%s], value[%s]", key, value);
        goto err;
    }

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return -1;
    }

    return 0;

err:
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
    }
#endif

    return -1;
}

char *liblog_get_mdc(const char *key)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;
    char *value = NULL;
    struct liblog_thread *a_thread;

    logc_assert(key, NULL);

    rc = pthread_rwlock_rdlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_rdlock fail, rc[%d]", rc);
        return NULL;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto err;
    }

    a_thread = (struct liblog_thread *)pthread_getspecific(liblog_thread_key);
    if (!a_thread) {
        logc_error("thread not found, maybe not use liblog_put_mdc before");
        goto err;
    }

    value = liblog_mdc_get(a_thread->mdc, key);
    if (!value) {
        logc_error("key[%s] not found in mdc", key);
        goto err;
    }

    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return NULL;
    }

    return value;

err:
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc=[%d]", rc);
    }
#endif

    return NULL;
}

void liblog_remove_mdc(const char *key)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;
    struct liblog_thread *a_thread;

    logc_assert(key, );

    rc = pthread_rwlock_rdlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_rdlock fail, rc[%d]", rc);
        return;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    a_thread = (struct liblog_thread *)pthread_getspecific(liblog_thread_key);
    if (!a_thread) {
        logc_error("thread not found, maybe not use liblog_put_mdc before");
        goto exit;
    }

    liblog_mdc_remove(a_thread->mdc, key);

exit:
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return;
    }
#endif
}

void liblog_clean_mdc(void)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;
    struct liblog_thread *a_thread;

    rc = pthread_rwlock_rdlock(&liblog_env_lock);
    if (rc) {;
        logc_error("pthread_rwlock_rdlock fail, rc[%d]", rc);
        return;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    a_thread = (struct liblog_thread *)pthread_getspecific(liblog_thread_key);
    if (!a_thread) {
        logc_error("thread not found, maybe not use liblog_put_mdc before");
        goto exit;
    }

    liblog_mdc_clean(a_thread->mdc);

exit:
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return;
    }
#endif
}

int liblog_level_switch(struct liblog_category *category, int level)
{
#if defined(CONFIG_LIBLOG)
    memset(category->level_bitmap, 0x00, sizeof(category->level_bitmap));
    category->level_bitmap[level / 8] |= ~(0xFF << (8 - level % 8));
    memset(category->level_bitmap + level / 8 + 1, 0xFF, sizeof(category->level_bitmap) - level / 8 - 1);
#endif

    return 0;
}

void liblog_vprintf(struct liblog_category *category, const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, va_list args)
{
#if defined(CONFIG_LIBLOG)
    struct liblog_thread *a_thread;

    pthread_rwlock_rdlock(&liblog_env_lock);

    if (liblog_category_needless_level(category, level)) {
        goto exit;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    liblog_fetch_thread(a_thread, exit);

    liblog_event_set_fmt(a_thread->event, category->name, category->name_len, file, filelen, func, funclen, line, level, format, args);

    if (liblog_category_output(category, a_thread)) {
        logc_error("liblog_output fail, srcfile[%s], srcline[%ld]", file, line);
        goto exit;
    }

    if (liblog_env_conf->reload_conf_period && (++liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period)) {
        goto reload;
    }

exit:
    pthread_rwlock_unlock(&liblog_env_lock);
    return;

reload:
    pthread_rwlock_unlock(&liblog_env_lock);
    if (liblog_reload((char *)-1)) {
        logc_error("reach reload-conf-period but liblog_reload fail, liblog-chk-conf [file] see detail");
    }
#endif
}

void liblog_printf_hex(struct liblog_category *category, const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *buf, size_t buflen)
{
#if defined(CONFIG_LIBLOG)
    struct liblog_thread *a_thread;

    pthread_rwlock_rdlock(&liblog_env_lock);

    if (liblog_category_needless_level(category, level)) {
        goto exit;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    liblog_fetch_thread(a_thread, exit);

    liblog_event_set_hex(a_thread->event, category->name, category->name_len, file, filelen, func, funclen, line, level, buf, buflen);

    if (liblog_category_output(category, a_thread)) {
        logc_error("liblog_output fail, srcfile[%s], srcline[%ld]", file, line);
        goto exit;
    }

    if (liblog_env_conf->reload_conf_period && (++liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period)) {
        goto reload;
    }

exit:
    pthread_rwlock_unlock(&liblog_env_lock);
    return;

reload:
    pthread_rwlock_unlock(&liblog_env_lock);
    if (liblog_reload((char *)-1)) {
        logc_error("reach reload-conf-period but liblog_reload fail, liblog-chk-conf [file] see detail");
    }
#endif
}

void liblog_default_vprintf(const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, va_list args)
{
#if defined(CONFIG_LIBLOG)
    struct liblog_thread *a_thread;

    pthread_rwlock_rdlock(&liblog_env_lock);

    if (liblog_category_needless_level(liblog_default_category, level)) {
        goto exit;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    if (!liblog_default_category) {
        logc_error("liblog_default_category is null, liblog_init() or liblog_set_cateogry() is not called above");
        goto exit;
    }

    liblog_fetch_thread(a_thread, exit);

    liblog_event_set_fmt(a_thread->event, liblog_default_category->name, liblog_default_category->name_len, file, filelen, func, funclen, line, level, format, args);

    if (liblog_category_output(liblog_default_category, a_thread)) {
        logc_error("zlog_output fail, srcfile[%s], srcline[%ld]", file, line);
        goto exit;
    }

    if (liblog_env_conf->reload_conf_period && (++liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period)) {
        goto reload;
    }

exit:
    pthread_rwlock_unlock(&liblog_env_lock);
    return;

reload:
    pthread_rwlock_unlock(&liblog_env_lock);
    if (liblog_reload((char *)-1)) {
        logc_error("reach reload-conf-period but liblog_reload fail, liblog-chk-conf [file] see detail");
    }
#endif
}

void liblog_default_printf_hex(const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const void *buf, size_t buflen)
{
#if defined(CONFIG_LIBLOG)
    struct liblog_thread *a_thread;

    pthread_rwlock_rdlock(&liblog_env_lock);

    if (liblog_category_needless_level(liblog_default_category, level)) {
        goto exit;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    if (!liblog_default_category) {
        logc_error("liblog_default_category is null, liblog_default_init() or liblog_set_cateogry() is not called above");
        goto exit;
    }

    liblog_fetch_thread(a_thread, exit);

    liblog_event_set_hex(a_thread->event, liblog_default_category->name, liblog_default_category->name_len, file, filelen, func, funclen, line, level, buf, buflen);

    if (liblog_category_output(liblog_default_category, a_thread)) {
        logc_error("liblog_output fail, srcfile[%s], srcline[%ld]", file, line);
        goto exit;
    }

    if (liblog_env_conf->reload_conf_period && (++liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period)) {
        goto reload;
    }

exit:
    pthread_rwlock_unlock(&liblog_env_lock);
    return;

reload:
    pthread_rwlock_unlock(&liblog_env_lock);
    if (liblog_reload((char *)-1)) {
        logc_error("reach reload-conf-period but liblog_reload fail, liblog-chk-conf [file] see detail");
    }
#endif
}

void liblog_printf(struct liblog_category *category, const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, ...)
{
#if defined(CONFIG_LIBLOG)
    va_list args;
    struct liblog_thread *a_thread;

    pthread_rwlock_rdlock(&liblog_env_lock);

    if (category && liblog_category_needless_level(category, level)) {
        goto exit;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    liblog_fetch_thread(a_thread, exit);

    va_start(args, format);
    liblog_event_set_fmt(a_thread->event, category->name, category->name_len, file, filelen, func, funclen, line, level, format, args);
    if (liblog_category_output(category, a_thread)) {
        logc_error("liblog_output fail, srcfile[%s], srcline[%ld]", file, line);
        va_end(args);

        goto exit;
    }
    va_end(args);

    if (liblog_env_conf->reload_conf_period && (++liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period)) {
        goto reload;
    }

exit:
    pthread_rwlock_unlock(&liblog_env_lock);
    return;

reload:
    pthread_rwlock_unlock(&liblog_env_lock);
    if (liblog_reload((char *)-1)) {
        logc_error("reach reload-conf-period but liblog_reload fail, liblog-chk-conf [file] see detail");
    }
#endif
}

void liblog_default_printf(const char *file, size_t filelen, const char *func, size_t funclen, long line, int level, const char *format, ...)
{
#if defined(CONFIG_LIBLOG)
    va_list args;
    struct liblog_thread *a_thread;

    pthread_rwlock_rdlock(&liblog_env_lock);

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto exit;
    }

    if (!liblog_default_category) {
        logc_error("liblog_default_category is null, liblog_default_init() or liblog_deault_set_cateogry() is not called above");
        goto exit;
    }

    if (liblog_category_needless_level(liblog_default_category, level)) {
        goto exit;
    }

    liblog_fetch_thread(a_thread, exit);

    va_start(args, format);
    liblog_event_set_fmt(a_thread->event, liblog_default_category->name, liblog_default_category->name_len, file, filelen, func, funclen, line, level, format, args);

    if (liblog_category_output(liblog_default_category, a_thread)) {
        logc_error("liblog_output fail, srcfile[%s], srcline[%ld]", file, line);
        va_end(args);

        goto exit;
    }
    va_end(args);

    if (liblog_env_conf->reload_conf_period && (++liblog_env_reload_conf_count > liblog_env_conf->reload_conf_period)) {
        goto reload;
    }

exit:
    pthread_rwlock_unlock(&liblog_env_lock);
    return;

reload:
    pthread_rwlock_unlock(&liblog_env_lock);
    if (liblog_reload((char *)-1)) {
        logc_error("reach reload-conf-period but liblog_reload fail, liblog-chk-conf [file] see detail");
    }
#endif
}

void liblog_profile(void)
{
#if defined(CONFIG_LIBLOG)
    int rc = 0;

    rc = pthread_rwlock_rdlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_wrlock fail, rc[%d]", rc);
        return;
    }

    logc_warn("------liblog_profile start------ ");
    logc_warn("is init:[%d]", liblog_env_is_init);
    logc_warn("init version:[%d]", liblog_env_init_version);

    liblog_conf_profile(liblog_env_conf, LOGC_WARN);
    liblog_record_table_profile(liblog_env_records, LOGC_WARN);
    liblog_category_table_profile(liblog_env_categories, LOGC_WARN);

    if (liblog_default_category) {
        logc_warn("-default_category-");
        liblog_category_profile(liblog_default_category, LOGC_WARN);
    }

    logc_warn("------liblog_profile end------ ");
    rc = pthread_rwlock_unlock(&liblog_env_lock);
    if (rc) {
        logc_error("pthread_rwlock_unlock fail, rc[%d]", rc);
        return;
    }
#endif
}

int liblog_set_record(const char *rname, liblog_record_func record)
{
#if defined(CONFIG_LIBLOG)
    int i = 0;
    int rc = 0;
    int rd = 0;
    struct liblog_rule *a_rule;
    struct liblog_record *a_record;

    logc_assert(rname, -1);
    logc_assert(record, -1);

    rd = pthread_rwlock_wrlock(&liblog_env_lock);
    if (rd) {
        logc_error("pthread_rwlock_rdlock fail, rd[%d]", rd);
        return -1;
    }

    if (!liblog_env_is_init) {
        logc_error("never call liblog_init() or liblog_default_init() before");
        goto liblog_set_record_exit;
    }

    a_record = liblog_record_new(rname, record);
    if (!a_record) {
        rc = -1;
        logc_error("zlog_record_new fail");

        goto liblog_set_record_exit;
    }

    rc = logc_hashtable_put(liblog_env_records, a_record->name, a_record);
    if (rc) {
        liblog_record_del(a_record);
        logc_error("libc_hashtable_put fail");

        goto liblog_set_record_exit;
    }

    logc_arraylist_foreach(liblog_env_conf->rules, i, a_rule) {
        liblog_rule_set_record((struct liblog_rule *)a_rule, liblog_env_records);
    }

liblog_set_record_exit:
    rd = pthread_rwlock_unlock(&liblog_env_lock);
    if (rd) {
        logc_error("pthread_rwlock_unlock fail, rd[%d]", rd);
        return -1;
    }

    return rc;
#else
    return 0;
#endif
}

int liblog_level_enabled(struct liblog_category *category, int level)
{
#if defined(CONFIG_LIBLOG)
    int enable = 0;

    pthread_rwlock_rdlock(&liblog_env_lock);
    enable = category && ((liblog_category_needless_level(category, level) == 0));
    pthread_rwlock_unlock(&liblog_env_lock);

    return enable;
#else
    return 0;
#endif
}

const char *liblog_get_version(void)
{
    return LIBLOG_VERSION;
}
