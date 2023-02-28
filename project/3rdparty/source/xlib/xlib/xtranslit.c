#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib.h>
#include <xlib/xlib/xstrfuncs.h>

struct mapping_entry {
    xuint16 src;
    xuint16 ascii;
};

struct mapping_range {
    xuint16 start;
    xuint16 length;
};

struct locale_entry {
    xuint8 name_offset;
    xuint8 item_id;
};

#include "xtranslit-data.h"

#define get_src_char(array, encoded, index)     ((encoded & 0x8000) ? (array)[((encoded) & 0xfff) + index] : encoded)
#define get_length(encoded)                     ((encoded & 0x8000) ? ((encoded & 0x7000) >> 12) : 1)

#if X_BYTE_ORDER == X_BIG_ENDIAN
#define get_ascii_item(array, encoded)          ((encoded & 0x8000) ? &(array)[(encoded) & 0xfff] : (xpointer)(((char *)&(encoded)) + 1))
#else
#define get_ascii_item(array, encoded)          ((encoded & 0x8000) ? &(array)[(encoded) & 0xfff] : (xpointer)&(encoded))
#endif

static const xchar *lookup_in_item(xuint item_id, const xunichar *key, xint *result_len, xint *key_consumed);

static xint compare_mapping_entry(xconstpointer user_data, xconstpointer data)
{
    xunichar src_0;
    const xunichar *key = (const xunichar *)user_data;
    const struct mapping_entry *entry = (const struct mapping_entry *)data;

    X_STATIC_ASSERT(MAX_KEY_SIZE == 2);

    src_0 = get_src_char(src_table, entry->src, 0);
    if (key[0] > src_0) {
        return 1;
    } else if (key[0] < src_0) {
        return -1;
    }

    if (get_length(entry->src) > 1) {
        xunichar src_1;

        src_1 = get_src_char(src_table, entry->src, 1);
        if (key[1] > src_1) {
            return 1;
        } else if (key[1] < src_1) {
            return -1;
        }
    } else if (key[1]) {
        return 1;
    }

    return 0;
}

static const xchar *lookup_in_mapping(const struct mapping_entry *mapping, xint mapping_size, const xunichar *key, xint *result_len, xint *key_consumed)
{
    const struct mapping_entry *hit;

    hit = (const struct mapping_entry *)bsearch(key, mapping, mapping_size, sizeof (struct mapping_entry), compare_mapping_entry);
    if (hit == NULL) {
        return NULL;
    }

    *key_consumed = get_length(hit->src);
    *result_len = get_length(hit->ascii);

    return (const xchar *)get_ascii_item(ascii_table, hit->ascii);
}

static const xchar *lookup_in_chain(const xuint8 *chain, const xunichar *key, xint *result_len, xint *key_consumed)
{
    const xchar *result;

    while (*chain != 0xff) {
        result = lookup_in_item(*chain, key, result_len, key_consumed);
        if (result) {
            return result;
        }

        chain++;
    }

    return NULL;
}

static const xchar *lookup_in_item(xuint item_id, const xunichar *key, xint *result_len, xint *key_consumed)
{
    if (item_id & 0x80) {
        const xuint8 *chain = chains_table + chain_starts[item_id & 0x7f];
        return lookup_in_chain(chain, key, result_len, key_consumed);
    } else {
        const struct mapping_range *range = &mapping_ranges[item_id];
        return lookup_in_mapping(mappings_table + range->start, range->length, key, result_len, key_consumed);
    }
}

static xint compare_locale_entry(xconstpointer user_data, xconstpointer data)
{
    const xchar *key = (const xchar *)user_data;
    const struct locale_entry *entry = (const struct locale_entry *)data;

    return strcmp(key, &locale_names[entry->name_offset]);
}

static xboolean lookup_item_id_for_one_locale(const xchar *key, xuint *item_id)
{
    const struct locale_entry *hit;

    hit = (const struct locale_entry *)bsearch(key, locale_index, X_N_ELEMENTS(locale_index), sizeof(struct locale_entry), compare_locale_entry);
    if (hit == NULL) {
        return FALSE;
    }

    *item_id = hit->item_id;
    return TRUE;
}

static xuint lookup_item_id_for_locale(const xchar *locale)
{
    xuint id;
    xuint language_len;
    const xchar *language;
    xuint modifier_len = 0;
    const xchar *next_char;
    xuint territory_len = 0;
    const xchar *modifier = NULL;
    const xchar *territory = NULL;
    xchar key[MAX_LOCALE_NAME + 1];

    language = locale;
    language_len = strcspn(language, "_.@");
    next_char = language + language_len;

    if (*next_char == '_') {
        territory = next_char;
        territory_len = strcspn(territory + 1, "_.@") + 1;
        next_char = territory + territory_len;
    }

    if (*next_char == '.') {
        const xchar *codeset;
        xuint codeset_len;

        codeset = next_char;
        codeset_len = strcspn(codeset + 1, "_.@") + 1;
        next_char = codeset + codeset_len;
    }

    if (*next_char == '@') {
        modifier = next_char;
        modifier_len = strcspn(modifier + 1, "_.@") + 1;
        next_char = modifier + modifier_len;
    }

    if (language_len == 0 || *next_char) {
        return default_item_id;
    }

    if (modifier_len && language_len + modifier_len <= MAX_LOCALE_NAME) {
        memcpy(key, language, language_len);
        memcpy(key + language_len, modifier, modifier_len);
        key[language_len + modifier_len] = '\0';

        if (lookup_item_id_for_one_locale(key, &id)) {
            return id;
        }
    }

    if (territory_len && language_len + territory_len <= MAX_LOCALE_NAME) {
        memcpy(key, language, language_len);
        memcpy(key + language_len, territory, territory_len);
        key[language_len + territory_len] = '\0';

        if (lookup_item_id_for_one_locale(key, &id)) {
            return id;
        }
    }

    if (language_len <= MAX_LOCALE_NAME) {
        memcpy(key, language, language_len);
        key[language_len] = '\0';

        if (lookup_item_id_for_one_locale(key, &id)) {
            return id;
        }
    }

    return default_item_id;
}

static xuint get_default_item_id(void)
{
    static xuint item_id;
    static xboolean done;

    if (!done) {
        const xchar *locale;

        locale = setlocale(LC_CTYPE, NULL);
        item_id = lookup_item_id_for_locale(locale);
        done = TRUE;
    }

    return item_id;
}

xchar *x_str_to_ascii(const xchar *str, const xchar *from_locale)
{
    xuint item_id;
    XString *result;

    x_return_val_if_fail(str != NULL, NULL);

    if (x_str_is_ascii(str)) {
        return x_strdup(str);
    }

    if (from_locale) {
        item_id = lookup_item_id_for_locale(from_locale);
    } else {
        item_id = get_default_item_id();
    }

    result = x_string_sized_new(strlen(str));
    while (*str) {
        if (*str & 0x80) {
            xint r_len;
            xunichar c;
            xint consumed;
            const xchar *r;
            xunichar key[MAX_KEY_SIZE];

            X_STATIC_ASSERT(MAX_KEY_SIZE == 2);

            c = x_utf8_get_char(str);
            str = x_utf8_next_char(str);

            key[0] = c;
            if (*str & 0x80) {
                key[1] = x_utf8_get_char(str);
            } else {
                key[1] = 0;
            }

            r = lookup_in_item(item_id, key, &r_len, &consumed);
            if (r == NULL && key[1]) {
                key[1] = 0;
                r = lookup_in_item(item_id, key, &r_len, &consumed);
            }

            if (r != NULL) {
                x_string_append_len(result, r, r_len);
                if (consumed == 2) {
                    str = x_utf8_next_char(str);
                }
            } else {
                x_string_append_c(result, '?');
            }
        } else {
            x_string_append_c(result, *str++);
        }
    }

    return x_string_free(result, FALSE);
}

