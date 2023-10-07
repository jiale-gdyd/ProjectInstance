#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <langinfo.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xcharset.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xlibcharset.h>
#include <xlib/xlib/xthreadprivate.h>
#include <xlib/xlib/xcharsetprivate.h>

X_LOCK_DEFINE_STATIC(aliases);

static XHashTable *get_alias_hash(void)
{
    const char *aliases;
    static XHashTable *alias_hash = NULL;

    X_LOCK(aliases);

    if (!alias_hash) {
        alias_hash = x_hash_table_new(x_str_hash, x_str_equal);

        aliases = _x_locale_get_charset_aliases();
        while (*aliases != '\0') {
            int count = 0;
            const char *alias;
            const char *canonical;
            const char **alias_array;

            alias = aliases;
            aliases += strlen(aliases) + 1;
            canonical = aliases;
            aliases += strlen(aliases) + 1;

            alias_array = (const char **)x_hash_table_lookup(alias_hash, canonical);
            if (alias_array) {
                while (alias_array[count]) {
                    count++;
                }
            }

            alias_array = x_renew(const char *, alias_array, count + 2);
            alias_array[count] = alias;
            alias_array[count + 1] = NULL;

            x_hash_table_insert(alias_hash, (char *)canonical, alias_array);
        }
    }

    X_UNLOCK(aliases);

    return alias_hash;
}

const char **_x_charset_get_aliases(const char *canonical_name)
{
    XHashTable *alias_hash = (XHashTable *)get_alias_hash();
    return (const char **)x_hash_table_lookup(alias_hash, canonical_name);
}

static xboolean x_utf8_get_charset_internal(const char *raw_data, const char **a)
{
    const char *charset = x_getenv("CHARSET");

    if (charset && *charset) {
        *a = charset;
        if (charset && strstr(charset, "UTF-8")) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    X_LOCK(aliases);
    charset = _x_locale_charset_unalias(raw_data);
    X_UNLOCK(aliases);

    if (charset && *charset) {
        *a = charset;
        if (charset && strstr(charset, "UTF-8")) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    *a = "US-ASCII";

    return FALSE;
}

typedef struct _XCharsetCache XCharsetCache;

struct _XCharsetCache {
    xboolean is_utf8;
    xchar    *raw;
    xchar    *charset;
};

static void charset_cache_free(xpointer data)
{
    XCharsetCache *cache = (XCharsetCache *)data;
    x_free(cache->raw);
    x_free(cache->charset);
    x_free(cache);
}

xboolean x_get_charset(const char **charset)
{
    const xchar *raw;
    static XPrivate cache_private = X_PRIVATE_INIT(charset_cache_free);
    XCharsetCache *cache = (XCharsetCache *)x_private_get(&cache_private);

    if (!cache) {
        cache = (XCharsetCache *)x_private_set_alloc0(&cache_private, sizeof(XCharsetCache));
    }

    X_LOCK(aliases);
    raw = _x_locale_charset_raw();
    X_UNLOCK(aliases);

    if (cache->raw == NULL || strcmp(cache->raw, raw) != 0) {
        const xchar *new_charset;

        x_free(cache->raw);
        x_free(cache->charset);
        cache->raw = x_strdup(raw);
        cache->is_utf8 = x_utf8_get_charset_internal(raw, &new_charset);
        cache->charset = x_strdup(new_charset);
    }

    if (charset) {
        *charset = cache->charset;
    }

    return cache->is_utf8;
}

xboolean _x_get_time_charset(const char **charset)
{
    const xchar *raw;
    static XPrivate cache_private = X_PRIVATE_INIT(charset_cache_free);
    XCharsetCache *cache = (XCharsetCache *)x_private_get(&cache_private);

    if (!cache) {
        cache = (XCharsetCache *)x_private_set_alloc0(&cache_private, sizeof(XCharsetCache));
    }

#ifdef HAVE_LANGINFO_TIME_CODESET
    raw = nl_langinfo(_NL_TIME_CODESET);
#else
    X_LOCK(aliases);
    raw = _x_locale_charset_raw();
    X_UNLOCK(aliases);
#endif

    if (cache->raw == NULL || strcmp(cache->raw, raw) != 0) {
        const xchar *new_charset;

        x_free(cache->raw);
        x_free(cache->charset);
        cache->raw = x_strdup(raw);
        cache->is_utf8 = x_utf8_get_charset_internal(raw, &new_charset);
        cache->charset = x_strdup(new_charset);
    }

    if (charset) {
        *charset = cache->charset;
    }

    return cache->is_utf8;
}

xboolean _x_get_ctype_charset(const char **charset)
{
    const xchar *raw;
    static XPrivate cache_private = X_PRIVATE_INIT(charset_cache_free);
    XCharsetCache *cache = (XCharsetCache *)x_private_get(&cache_private);

    if (!cache) {
        cache = (XCharsetCache *)x_private_set_alloc0(&cache_private, sizeof(XCharsetCache));
    }

#ifdef HAVE_LANGINFO_CODESET
    raw = nl_langinfo (CODESET);
#else
    X_LOCK(aliases);
    raw = _x_locale_charset_raw();
    X_UNLOCK(aliases);
#endif

    if (cache->raw == NULL || strcmp(cache->raw, raw) != 0) {
        const xchar *new_charset;

        x_free(cache->raw);
        x_free(cache->charset);
        cache->raw = x_strdup(raw);
        cache->is_utf8 = x_utf8_get_charset_internal(raw, &new_charset);
        cache->charset = x_strdup(new_charset);
    }

    if (charset) {
        *charset = cache->charset;
    }

    return cache->is_utf8;
}

xchar *x_get_codeset(void)
{
    const xchar *charset;

    x_get_charset(&charset);
    return x_strdup(charset);
}

xboolean x_get_console_charset(const char **charset)
{
    return x_get_charset(charset);
}

static void read_aliases(const xchar *file, XHashTable *alias_table)
{
    FILE *fp;
    char buf[256];

    fp = fopen(file, "re");
    if (!fp) {
        return;
    }

    while (fgets(buf, 256, fp)) {
        char *p, *q;

        x_strstrip(buf);
        if ((buf[0] == '#') || (buf[0] == '\0')) {
            continue;
        }

        for (p = buf, q = NULL; *p; p++) {
            if ((*p == '\t') || (*p == ' ') || (*p == ':')) {
                *p = '\0';
                q = p + 1;
                while ((*q == '\t') || (*q == ' ')) { 
                    q++;
                }
    
                break;
            }
        }

        if (!q || *q == '\0') {
            continue;
        }

        for (p = q; *p; p++) {
            if ((*p == '\t') || (*p == ' ')) {
                *p = '\0';
                break;
            }
        }

        if (!x_hash_table_lookup(alias_table, buf)) {
            x_hash_table_insert(alias_table, x_strdup(buf), x_strdup(q));
        }
    }

    fclose(fp);
}

static char *unalias_lang(char *lang)
{
    int i;
    char *p;
    static XHashTable *alias_table = NULL;

    if (x_once_init_enter_pointer(&alias_table)) {
        XHashTable *table = x_hash_table_new(x_str_hash, x_str_equal);
        read_aliases("/usr/share/locale/locale.alias", table);
        x_once_init_leave_pointer(&alias_table, table);
    }

    i = 0;
    while ((p = (char *)x_hash_table_lookup(alias_table, lang)) && (strcmp(p, lang) != 0)) {
        lang = p;
        if (i++ == 30) {
            static xboolean said_before = FALSE;
            if (!said_before) {
                x_warning("Too many alias levels for a locale, may indicate a loop");
            }

            said_before = TRUE;
            return lang;
        }
    }

    return lang;
}

enum {
    COMPONENT_CODESET   =   1 << 0,
    COMPONENT_TERRITORY = 1 << 1,
    COMPONENT_MODIFIER  =  1 << 2
};

static xuint explode_locale(const xchar *locale, xchar **language, xchar **territory, xchar **codeset, xchar **modifier)
{
    xuint mask = 0;
    const xchar *at_pos;
    const xchar *dot_pos;
    const xchar *uscore_pos;

    uscore_pos = strchr(locale, '_');
    dot_pos = strchr(uscore_pos ? uscore_pos : locale, '.');
    at_pos = strchr(dot_pos ? dot_pos : (uscore_pos ? uscore_pos : locale), '@');

    if (at_pos) {
        mask |= COMPONENT_MODIFIER;
        *modifier = x_strdup(at_pos);
    } else {
        at_pos = locale + strlen(locale);
    }

    if (dot_pos) {
        mask |= COMPONENT_CODESET;
        *codeset = x_strndup(dot_pos, at_pos - dot_pos);
    } else {
        dot_pos = at_pos;
    }

    if (uscore_pos) {
        mask |= COMPONENT_TERRITORY;
        *territory = x_strndup(uscore_pos, dot_pos - uscore_pos);
    } else {
        uscore_pos = dot_pos;
    }
    *language = x_strndup(locale, uscore_pos - locale);

    return mask;
}

static void append_locale_variants(XPtrArray *array, const xchar *locale)
{
    xuint mask;
    xuint i, j;
    xchar *codeset = NULL;
    xchar *language = NULL;
    xchar *modifier = NULL;
    xchar *territory = NULL;

    x_return_if_fail(locale != NULL);

    mask = explode_locale(locale, &language, &territory, &codeset, &modifier);

    for (j = 0; j <= mask; ++j) {
        i = mask - j;
        if ((i & ~mask) == 0) {
            xchar *val = x_strconcat(language, (i & COMPONENT_TERRITORY) ? territory : "", (i & COMPONENT_CODESET) ? codeset : "", (i & COMPONENT_MODIFIER) ? modifier : "", NULL);
            x_ptr_array_add(array, val);
        }
    }

    x_free(language);
    if (mask & COMPONENT_CODESET) {
        x_free(codeset);
    }

    if (mask & COMPONENT_TERRITORY) {
        x_free(territory);
    }

    if (mask & COMPONENT_MODIFIER) {
        x_free(modifier);
    }
}

xchar **x_get_locale_variants(const xchar *locale)
{
    XPtrArray *array;

    x_return_val_if_fail(locale != NULL, NULL);

    array = x_ptr_array_sized_new(8);
    append_locale_variants(array, locale);
    x_ptr_array_add(array, NULL);

    return (xchar **)x_ptr_array_free(array, FALSE);
}

static const xchar *guess_category_value(const xchar *category_name)
{
    const xchar *retval;

    retval = x_getenv("LANGUAGE");
    if ((retval != NULL) && (retval[0] != '\0')) {
        return retval;
    }

    retval = x_getenv("LC_ALL");
    if ((retval != NULL) && (retval[0] != '\0')) {
        return retval;
    }

    retval = x_getenv(category_name);
    if ((retval != NULL) && (retval[0] != '\0')) {
        return retval;
    }

    retval = x_getenv("LANG");
    if ((retval != NULL) && (retval[0] != '\0')) {
        return retval;
    }

    return NULL;
}

typedef struct _XLanguageNamesCache XLanguageNamesCache;

struct _XLanguageNamesCache {
    xchar *languages;
    xchar **language_names;
};

static void language_names_cache_free(xpointer data)
{
    XLanguageNamesCache *cache = (XLanguageNamesCache *)data;

    x_free(cache->languages);
    x_strfreev(cache->language_names);
    x_free(cache);
}

const xchar *const *x_get_language_names(void)
{
    return x_get_language_names_with_category("LC_MESSAGES");
}

const xchar *const *x_get_language_names_with_category(const xchar *category_name)
{
    const xchar *languages;
    XLanguageNamesCache *name_cache;
    static XPrivate cache_private = X_PRIVATE_INIT((void (*)(xpointer))x_hash_table_unref);
    XHashTable *cache = (XHashTable *)x_private_get(&cache_private);

    x_return_val_if_fail(category_name != NULL, NULL);

    if (!cache) {
        cache = x_hash_table_new_full(x_str_hash, x_str_equal, x_free, language_names_cache_free);
        x_private_set(&cache_private, cache);
    }

    languages = guess_category_value(category_name);
    if (!languages) {
        languages = "C";
    }

    name_cache = (XLanguageNamesCache *)x_hash_table_lookup(cache, category_name);
    if (!(name_cache && name_cache->languages && strcmp(name_cache->languages, languages) == 0)) {
        XPtrArray *array;
        xchar **alist, **a;

        x_hash_table_remove(cache, category_name);

        array = x_ptr_array_sized_new(8);

        alist = x_strsplit(languages, ":", 0);
        for (a = alist; *a; a++) {
            append_locale_variants(array, unalias_lang(*a));
        }

        x_strfreev(alist);
        x_ptr_array_add(array, x_strdup("C"));
        x_ptr_array_add(array, NULL);

        name_cache = x_new0(XLanguageNamesCache, 1);
        name_cache->languages = x_strdup(languages);
        name_cache->language_names = (xchar **)x_ptr_array_free(array, FALSE);
        x_hash_table_insert(cache, x_strdup(category_name), name_cache);
    }

    return (const xchar *const *)name_cache->language_names;
}
