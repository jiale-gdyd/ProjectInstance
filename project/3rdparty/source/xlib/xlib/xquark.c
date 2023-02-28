#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xquark.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xlib_trace.h>

#define QUARK_BLOCK_SIZE                    2048
#define QUARK_STRING_BLOCK_SIZE             (4096 - sizeof (xsize))

static inline XQuark quark_new(xchar *string);

X_LOCK_DEFINE_STATIC(quark_global);

static xchar **quarks = NULL;
static xint quark_seq_id = 0;
static xchar *quark_block = NULL;
static XHashTable *quark_ht = NULL;
static xint quark_block_offset = 0;

void x_quark_init(void)
{
    x_assert(quark_seq_id == 0);

    quark_ht = x_hash_table_new(x_str_hash, x_str_equal);
    quarks = x_new(xchar *, QUARK_BLOCK_SIZE);
    quarks[0] = NULL;
    quark_seq_id = 1;
}

XQuark x_quark_try_string(const xchar *string)
{
    XQuark quark = 0;

    if (string == NULL) {
        return 0;
    }

    X_LOCK(quark_global);
    quark = XPOINTER_TO_UINT(x_hash_table_lookup(quark_ht, string));
    X_UNLOCK(quark_global);

    return quark;
}

static char *quark_strdup(const xchar *string)
{
    xsize len;
    xchar *copy;

    len = strlen(string) + 1;
    if (len > QUARK_STRING_BLOCK_SIZE / 2) {
        return x_strdup(string);
    }

    if (quark_block == NULL || QUARK_STRING_BLOCK_SIZE - quark_block_offset < len) {
        quark_block = (xchar *)x_malloc(QUARK_STRING_BLOCK_SIZE);
        quark_block_offset = 0;
    }

    copy = quark_block + quark_block_offset;
    memcpy(copy, string, len);
    quark_block_offset += len;

    return copy;
}

static inline XQuark quark_from_string(const xchar *string, xboolean duplicate)
{
    XQuark quark = 0;

    quark = XPOINTER_TO_UINT(x_hash_table_lookup(quark_ht, string));
    if (!quark) {
        quark = quark_new(duplicate ? quark_strdup(string) : (xchar *)string);
        TRACE(XLIB_QUARK_NEW(string, quark));
    }

    return quark;
}

static inline XQuark quark_from_string_locked(const xchar *string, xboolean duplicate)
{
    XQuark quark = 0;

    if (!string) {
        return 0;
    }

    X_LOCK(quark_global);
    quark = quark_from_string(string, duplicate);
    X_UNLOCK(quark_global);

    return quark;
}

XQuark x_quark_from_string(const xchar *string)
{
  return quark_from_string_locked(string, TRUE);
}

XQuark x_quark_from_static_string(const xchar *string)
{
    return quark_from_string_locked(string, FALSE);
}

const xchar *x_quark_to_string(XQuark quark)
{
    xuint seq_id;
    xchar **strings;
    xchar *result = NULL;

    seq_id = (xuint)x_atomic_int_get(&quark_seq_id);
    strings = x_atomic_pointer_get(&quarks);

    if (quark < seq_id) {
        result = strings[quark];
    }

    return result;
}

static inline XQuark quark_new(xchar *string)
{
    XQuark quark;
    xchar **quarks_new;

    if (quark_seq_id % QUARK_BLOCK_SIZE == 0) {
        quarks_new = x_new(xchar *, quark_seq_id + QUARK_BLOCK_SIZE);
        if (quark_seq_id != 0) {
            memcpy(quarks_new, quarks, sizeof(char *) * quark_seq_id);
        }

        memset(quarks_new + quark_seq_id, 0, sizeof (char *) * QUARK_BLOCK_SIZE);
        x_atomic_pointer_set(&quarks, quarks_new);
    }

    quark = quark_seq_id;
    x_atomic_pointer_set(&quarks[quark], string);
    x_hash_table_insert(quark_ht, string, XUINT_TO_POINTER(quark));
    x_atomic_int_inc(&quark_seq_id);

    return quark;
}

static inline const xchar *quark_intern_string_locked(const xchar *string, xboolean duplicate)
{
    XQuark quark;
    const xchar *result;

    if (!string) {
        return NULL;
    }

    X_LOCK(quark_global);
    quark = quark_from_string(string, duplicate);
    result = quarks[quark];
    X_UNLOCK(quark_global);

    return result;
}

const xchar *x_intern_string(const xchar *string)
{
    return quark_intern_string_locked(string, TRUE);
}

const xchar *x_intern_static_string(const xchar *string)
{
    return quark_intern_string_locked(string, FALSE);
}
