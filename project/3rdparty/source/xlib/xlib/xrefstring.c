#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xrcbox.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xrefstring.h>

X_LOCK_DEFINE_STATIC(interned_ref_strings);
static XHashTable *interned_ref_strings;

char *x_ref_string_new(const char *str)
{
    char *res;
    xsize len;

    x_return_val_if_fail(str != NULL, NULL);
    len = strlen(str);
    res = (char *)x_atomic_rc_box_dup(sizeof(char) * len + 1, str);

    return res;
}

char *x_ref_string_new_len(const char *str, xssize len)
{
    char *res;

    x_return_val_if_fail(str != NULL, NULL);

    if (len < 0) {
        return x_ref_string_new(str);
    }

    res = (char *)x_atomic_rc_box_alloc((xsize) len + 1);
    memcpy(res, str, len);
    res[len] = '\0';

    return res;
}

static xboolean interned_str_equal(xconstpointer v1, xconstpointer v2)
{
    const char *str1 = (const char *)v1;
    const char *str2 = (const char *)v2;

    if (v1 == v2) {
        return TRUE;
    }

    return strcmp(str1, str2) == 0;
}

char *x_ref_string_new_intern(const char *str)
{
    char *res;

    x_return_val_if_fail(str != NULL, NULL);

    X_LOCK(interned_ref_strings);

    if (X_UNLIKELY(interned_ref_strings == NULL)) {
        interned_ref_strings = x_hash_table_new(x_str_hash, interned_str_equal);
    }

    res = (char *)x_hash_table_lookup(interned_ref_strings, str);
    if (res != NULL) {
        x_atomic_rc_box_acquire(res);
        X_UNLOCK(interned_ref_strings);
        return res;
    }

    res = x_ref_string_new(str);
    x_hash_table_add(interned_ref_strings, res);
    X_UNLOCK(interned_ref_strings);

    return res;
}

char *x_ref_string_acquire(char *str)
{
    x_return_val_if_fail(str != NULL, NULL);
    return x_atomic_rc_box_acquire(str);
}

static void remove_if_interned(xpointer data)
{
    char *str = (char *)data;

    X_LOCK(interned_ref_strings);
    if (X_LIKELY(interned_ref_strings != NULL)) {
        x_hash_table_remove(interned_ref_strings, str);

        if (x_hash_table_size(interned_ref_strings) == 0) {
            x_clear_pointer(&interned_ref_strings, x_hash_table_destroy);
        }
    }
    X_UNLOCK(interned_ref_strings);
}

void x_ref_string_release(char *str)
{
    x_return_if_fail(str != NULL);
    x_atomic_rc_box_release_full(str, remove_if_interned);
}

xsize x_ref_string_length(char *str)
{
    x_return_val_if_fail(str != NULL, 0);
    return x_atomic_rc_box_get_size(str) - 1;
}
