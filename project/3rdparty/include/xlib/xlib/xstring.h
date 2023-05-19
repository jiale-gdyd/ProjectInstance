#ifndef __X_STRING_H__
#define __X_STRING_H__

#include <string.h>

#include "xtypes.h"
#include "xunicode.h"
#include "xbytes.h"
#include "xstrfuncs.h"
#include "xutils.h"

X_BEGIN_DECLS

typedef struct _XString XString;

struct _XString {
    xchar *str;
    xsize len;
    xsize allocated_len;
};

XLIB_AVAILABLE_IN_ALL
XString *x_string_new(const xchar *init);

XLIB_AVAILABLE_IN_2_78
XString *x_string_new_take(xchar *init);

XLIB_AVAILABLE_IN_ALL
XString *x_string_new_len(const xchar *init, xssize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_sized_new(xsize dfl_size);

XLIB_AVAILABLE_IN_ALL
xchar *(x_string_free)(XString *string, xboolean free_segment);

XLIB_AVAILABLE_IN_2_76
xchar *x_string_free_and_steal(XString *string) X_GNUC_WARN_UNUSED_RESULT;

#if X_GNUC_CHECK_VERSION (2, 0) && (XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_76)

#define x_string_free(str, free_segment)        \
    (__builtin_constant_p(free_segment) ? ((free_segment) ? (x_string_free)((str), (free_segment)) : x_string_free_and_steal(str)) : (x_string_free)((str), (free_segment)))

#endif

XLIB_AVAILABLE_IN_2_34
XBytes *x_string_free_to_bytes(XString *string);

XLIB_AVAILABLE_IN_ALL
xboolean x_string_equal(const XString *v, const XString *v2);

XLIB_AVAILABLE_IN_ALL
xuint x_string_hash(const XString *str);

XLIB_AVAILABLE_IN_ALL
XString *x_string_assign(XString *string, const xchar *rval);

XLIB_AVAILABLE_IN_ALL
XString *x_string_truncate(XString *string, xsize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_set_size(XString *string, xsize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_insert_len(XString *string, xssize pos, const xchar *val, xssize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_append(XString *string, const xchar *val);

XLIB_AVAILABLE_IN_ALL
XString *x_string_append_len(XString *string, const xchar *val, xssize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_append_c(XString *string, xchar c);

XLIB_AVAILABLE_IN_ALL
XString *x_string_append_unichar(XString *string, xunichar wc);

XLIB_AVAILABLE_IN_ALL
XString *x_string_prepend(XString *string, const xchar *val);

XLIB_AVAILABLE_IN_ALL
XString *x_string_prepend_c(XString *string, xchar c);

XLIB_AVAILABLE_IN_ALL
XString *x_string_prepend_unichar(XString *string, xunichar wc);

XLIB_AVAILABLE_IN_ALL
XString *x_string_prepend_len(XString *string, const xchar *val, xssize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_insert(XString *string, xssize pos, const xchar *val);

XLIB_AVAILABLE_IN_ALL
XString *x_string_insert_c(XString *string, xssize pos, xchar c);

XLIB_AVAILABLE_IN_ALL
XString *x_string_insert_unichar(XString *string, xssize pos, xunichar wc);

XLIB_AVAILABLE_IN_ALL
XString *x_string_overwrite(XString *string, xsize pos, const xchar *val);

XLIB_AVAILABLE_IN_ALL
XString *x_string_overwrite_len(XString *string, xsize pos, const xchar *val, xssize len);

XLIB_AVAILABLE_IN_ALL
XString *x_string_erase(XString *string, xssize pos, xssize len);

XLIB_AVAILABLE_IN_2_68
xuint x_string_replace(XString *string, const xchar *find, const xchar *replace, xuint limit);

XLIB_AVAILABLE_IN_ALL
XString *x_string_ascii_down(XString *string);

XLIB_AVAILABLE_IN_ALL
XString *x_string_ascii_up(XString *string);

XLIB_AVAILABLE_IN_ALL
void x_string_vprintf(XString *string, const xchar *format, va_list args) X_GNUC_PRINTF(2, 0);

XLIB_AVAILABLE_IN_ALL
void x_string_printf(XString *string, const xchar *format, ...) X_GNUC_PRINTF (2, 3);

XLIB_AVAILABLE_IN_ALL
void x_string_append_vprintf(XString *string, const xchar *format, va_list args) X_GNUC_PRINTF(2, 0);

XLIB_AVAILABLE_IN_ALL
void x_string_append_printf(XString *string, const xchar *format, ...) X_GNUC_PRINTF (2, 3);

XLIB_AVAILABLE_IN_ALL
XString *x_string_append_uri_escaped(XString *string, const xchar *unescaped, const xchar *reserved_chars_allowed, xboolean allow_utf8);

#ifdef X_CAN_INLINE
X_ALWAYS_INLINE
static inline XString *x_string_append_c_inline(XString *gstring, xchar c)
{
    if (X_LIKELY((gstring != NULL) && ((gstring->len + 1) < gstring->allocated_len))) {
        gstring->str[gstring->len++] = c;
        gstring->str[gstring->len] = 0;
    } else {
        x_string_insert_c(gstring, -1, c);
    }

    return gstring;
}

#define x_string_append_c(gstr, c)       x_string_append_c_inline(gstr, c)

X_ALWAYS_INLINE
static inline XString *x_string_append_len_inline(XString *gstring, const char *val, xssize len)
{
    xsize len_unsigned;

    if X_UNLIKELY(gstring == NULL) {
        return x_string_append_len(gstring, val, len);
    }

    if X_UNLIKELY(val == NULL) {
        return (len != 0) ? x_string_append_len(gstring, val, len) : gstring;
    }

    if (len < 0) {
        len_unsigned = strlen(val);
    } else {
        len_unsigned = (xsize)len;
    }

    if (X_LIKELY((gstring->len + len_unsigned) < gstring->allocated_len)) {
        char *end = gstring->str + gstring->len;
        if (X_LIKELY(((val + len_unsigned) <= end) || (val > (end + len_unsigned)))) {
            memcpy(end, val, len_unsigned);
        } else {
            memmove(end, val, len_unsigned);
        }

        gstring->len += len_unsigned;
        gstring->str[gstring->len] = 0;
        return gstring;
    } else {
        return x_string_insert_len(gstring, -1, val, len);
    }
}

#define x_string_append_len(gstr, val, len)     x_string_append_len_inline(gstr, val, len)

X_ALWAYS_INLINE
static inline XString *x_string_truncate_inline(XString *gstring, xsize len)
{
    gstring->len = MIN(len, gstring->len);
    gstring->str[gstring->len] = '\0';
    return gstring;
}

#define x_string_truncate(gstr, len)            x_string_truncate_inline(gstr, len)

#if X_GNUC_CHECK_VERSION(2, 0)
#define x_string_append(gstr, val)                  \
  (__builtin_constant_p(val) ?                      \
    X_GNUC_EXTENSION ({                             \
      const char *const __val = (val);              \
      x_string_append_len(gstr, __val,              \
        X_LIKELY(__val != NULL) ?                   \
          (xssize)strlen(_X_STR_NONNULL(__val))     \
        : (xssize)-1);                              \
    })                                              \
    :                                               \
    x_string_append_len(gstr, val, (xssize)-1))
#endif

#endif

XLIB_DEPRECATED
XString *x_string_down(XString *string);

XLIB_DEPRECATED
XString *x_string_up(XString *string);

#define x_string_sprintf                x_string_printf XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_string_printf)
#define x_string_sprintfa               x_string_append_printf XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_string_append_printf)

X_END_DECLS

#endif
