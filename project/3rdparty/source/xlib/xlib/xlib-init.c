#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xmacros.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xconstructor.h>

typedef struct { char a; XFunc b; } XFuncAlign;
typedef struct { char a; xpointer b; } xpointerAlign;
typedef struct { char a; XCompareDataFunc b; } XCompareDataFuncAlign;

X_STATIC_ASSERT(X_SIGNEDNESS_OF(int) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(unsigned int) == 0);

X_STATIC_ASSERT(CHAR_BIT == 8);

X_STATIC_ASSERT(sizeof(xpointer) == sizeof(XFunc));
// X_STATIC_ASSERT(X_ALIGNOF(xpointer) == X_ALIGNOF(XFunc));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xpointerAlign, b) == X_STRUCT_OFFSET(XFuncAlign, b));

X_STATIC_ASSERT(sizeof(XFunc) == sizeof(XCompareDataFunc));
// X_STATIC_ASSERT(X_ALIGNOF(XFunc) == X_ALIGNOF(XCompareDataFunc));
X_STATIC_ASSERT(X_STRUCT_OFFSET(XFuncAlign, b) == X_STRUCT_OFFSET(XCompareDataFuncAlign, b));

typedef enum {
    TEST_CHAR_0 = 0
} TestChar;

typedef enum {
    TEST_SHORT_0   = 0,
    TEST_SHORT_256 = 256
} TestShort;

typedef enum {
    TEST_INT32_MIN = X_MININT32,
    TEST_INT32_MAX = X_MAXINT32
} TestInt;

typedef struct { char a; int b; } intAlign;
typedef struct { char a; TestInt b; } TestIntAlign;
typedef struct { char a; TestChar b; } TestCharAlign;
typedef struct { char a; TestShort b; } TestShortAlign;

X_STATIC_ASSERT(sizeof(TestChar) == sizeof(int));
X_STATIC_ASSERT(sizeof(TestShort) == sizeof(int));
X_STATIC_ASSERT(sizeof(TestInt) == sizeof(int));
// X_STATIC_ASSERT(X_ALIGNOF(TestChar) == X_ALIGNOF(int));
// X_STATIC_ASSERT(X_ALIGNOF(TestShort) == X_ALIGNOF(int));
// X_STATIC_ASSERT(X_ALIGNOF(TestInt) == X_ALIGNOF(int));
X_STATIC_ASSERT(X_STRUCT_OFFSET(TestCharAlign, b) == X_STRUCT_OFFSET(intAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(TestShortAlign, b) == X_STRUCT_OFFSET(intAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(TestIntAlign, b) == X_STRUCT_OFFSET(intAlign, b));

X_STATIC_ASSERT(sizeof(xchar) == 1);
X_STATIC_ASSERT(sizeof(xuchar) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xuchar) == 0);

X_STATIC_ASSERT(sizeof(xint8) * CHAR_BIT == 8);
X_STATIC_ASSERT(sizeof(xuint8) * CHAR_BIT == 8);
X_STATIC_ASSERT(sizeof(xint16) * CHAR_BIT == 16);
X_STATIC_ASSERT(sizeof(xuint16) * CHAR_BIT == 16);
X_STATIC_ASSERT(sizeof(xint32) * CHAR_BIT == 32);
X_STATIC_ASSERT(sizeof(xuint32) * CHAR_BIT == 32);
X_STATIC_ASSERT(sizeof(xint64) * CHAR_BIT == 64);
X_STATIC_ASSERT(sizeof(xuint64) * CHAR_BIT == 64);

X_STATIC_ASSERT(sizeof(void *) == XLIB_SIZEOF_VOID_P);
X_STATIC_ASSERT(sizeof(xintptr) == sizeof(void *));
X_STATIC_ASSERT(sizeof(xuintptr) == sizeof(void *));

X_STATIC_ASSERT(sizeof(short) == sizeof(xshort));
X_STATIC_ASSERT(X_MINSHORT == SHRT_MIN);
X_STATIC_ASSERT(X_MAXSHORT == SHRT_MAX);
X_STATIC_ASSERT(sizeof(unsigned short) == sizeof(xushort));
X_STATIC_ASSERT(X_MAXUSHORT == USHRT_MAX);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xshort) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xushort) == 0);

X_STATIC_ASSERT(sizeof(int) == sizeof(xint));
X_STATIC_ASSERT(X_MININT == INT_MIN);
X_STATIC_ASSERT(X_MAXINT == INT_MAX);
X_STATIC_ASSERT(sizeof(unsigned int) == sizeof(xuint));
X_STATIC_ASSERT(X_MAXUINT == UINT_MAX);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xint) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xuint) == 0);

X_STATIC_ASSERT(sizeof(long) == XLIB_SIZEOF_LONG);
X_STATIC_ASSERT(sizeof(long) == sizeof(xlong));
X_STATIC_ASSERT(X_MINLONG == LONG_MIN);
X_STATIC_ASSERT(X_MAXLONG == LONG_MAX);
X_STATIC_ASSERT(sizeof(unsigned long) == sizeof(xulong));
X_STATIC_ASSERT(X_MAXULONG == ULONG_MAX);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xlong) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xulong) == 0);

X_STATIC_ASSERT(X_HAVE_XINT64 == 1);

X_STATIC_ASSERT(sizeof(size_t) == XLIB_SIZEOF_SIZE_T);

X_STATIC_ASSERT(sizeof(size_t) == XLIB_SIZEOF_SSIZE_T);
X_STATIC_ASSERT(sizeof(xsize) == XLIB_SIZEOF_SSIZE_T);
X_STATIC_ASSERT(sizeof(xsize) == sizeof(size_t));

X_STATIC_ASSERT(X_SIGNEDNESS_OF(size_t) == 0);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xsize) == 0);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xssize) == 1);

typedef struct { char a; xsize b; } xsizeAlign;
typedef struct { char a; xssize b; } xssizeAlign;
typedef struct { char a; size_t b; } size_tAlign;
typedef struct { char a; uintptr_t b; } uintptr_tAlign;

X_STATIC_ASSERT(sizeof(xssize) == sizeof(size_t));
// X_STATIC_ASSERT(X_ALIGNOF(xsize) == X_ALIGNOF(size_t));
// X_STATIC_ASSERT(X_ALIGNOF(xssize) == X_ALIGNOF(size_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xsizeAlign, b) == X_STRUCT_OFFSET(size_tAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xssizeAlign, b) == X_STRUCT_OFFSET(size_tAlign, b));

typedef struct { char a; xint64 b; } xint64Align;
typedef struct { char a; xoffset b; } xoffsetAlign;

#ifndef X_ENABLE_EXPERIMENTAL_ABI_COMPILATION
X_STATIC_ASSERT(sizeof(size_t) == sizeof(uintptr_t));
// X_STATIC_ASSERT(X_ALIGNOF(size_t) == X_ALIGNOF(uintptr_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(size_tAlign, b) == X_STRUCT_OFFSET(uintptr_tAlign, b));
#endif

X_STATIC_ASSERT(X_SIGNEDNESS_OF(xoffset) == 1);

X_STATIC_ASSERT(sizeof(xoffset) == sizeof(xint64));
// X_STATIC_ASSERT(X_ALIGNOF(xoffset) == X_ALIGNOF(xint64));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xoffsetAlign, b) == X_STRUCT_OFFSET(xint64Align, b));

typedef struct { char a; float b; } floatAlign;
typedef struct { char a; double b; } doubleAlign;
typedef struct { char a; xfloat b; } xfloatAlign;
typedef struct { char a; xdouble b; } xdoubleAlign;

X_STATIC_ASSERT(sizeof(xfloat) == sizeof(float));
// X_STATIC_ASSERT(X_ALIGNOF(xfloat) == X_ALIGNOF(float));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xfloatAlign, b) == X_STRUCT_OFFSET(floatAlign, b));

X_STATIC_ASSERT(sizeof(xdouble) == sizeof(double));
// X_STATIC_ASSERT(X_ALIGNOF(xdouble) == X_ALIGNOF(double));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xdoubleAlign, b) == X_STRUCT_OFFSET(doubleAlign, b));

typedef struct { char a; intptr_t b; } intptr_tAlign;
typedef struct { char a; xintptr b; } xintptrAlign;
typedef struct { char a; xuintptr b; } xuintptrAlign;

X_STATIC_ASSERT(sizeof(xintptr) == sizeof(intptr_t));
X_STATIC_ASSERT(sizeof(xuintptr) == sizeof(uintptr_t));
// X_STATIC_ASSERT(X_ALIGNOF(xintptr) == X_ALIGNOF(intptr_t));
// X_STATIC_ASSERT(X_ALIGNOF(xuintptr) == X_ALIGNOF(uintptr_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xintptrAlign, b) == X_STRUCT_OFFSET(intptr_tAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xuintptrAlign, b) == X_STRUCT_OFFSET(uintptr_tAlign, b));

X_STATIC_ASSERT(X_SIGNEDNESS_OF(xintptr) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xuintptr) == 0);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(intptr_t) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(uintptr_t) == 0);

typedef struct { char a; int8_t b; } int8_tAlign;
typedef struct { char a; uint8_t b; } uint8_tAlign;
typedef struct { char a; xint8 b; } xint8Align;
typedef struct { char a; xuint8 b; } xuint8Align;

X_STATIC_ASSERT(sizeof(xint8) == sizeof(int8_t));
X_STATIC_ASSERT(sizeof(xuint8) == sizeof(uint8_t));
// X_STATIC_ASSERT(X_ALIGNOF(xint8) == X_ALIGNOF(int8_t));
// X_STATIC_ASSERT(X_ALIGNOF(xuint8) == X_ALIGNOF(uint8_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xint8Align, b) == X_STRUCT_OFFSET(int8_tAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xuint8Align, b) == X_STRUCT_OFFSET(uint8_tAlign, b));

X_STATIC_ASSERT(X_SIGNEDNESS_OF(xint8) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xuint8) == 0);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(int8_t) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(uint8_t) == 0);

typedef struct { char a; int16_t b; } int16_tAlign;
typedef struct { char a; uint16_t b; } uint16_tAlign;
typedef struct { char a; xint16 b; } xint16Align;
typedef struct { char a; xuint16 b; } xuint16Align;

X_STATIC_ASSERT(sizeof(xint16) == sizeof(int16_t));
X_STATIC_ASSERT(sizeof(xuint16) == sizeof(uint16_t));
// X_STATIC_ASSERT(X_ALIGNOF(xint16) == X_ALIGNOF(int16_t));
// X_STATIC_ASSERT(X_ALIGNOF(xuint16) == X_ALIGNOF(uint16_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xint16Align, b) == X_STRUCT_OFFSET(int16_tAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xuint16Align, b) == X_STRUCT_OFFSET(uint16_tAlign, b));

X_STATIC_ASSERT(X_SIGNEDNESS_OF(int16_t) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(uint16_t) == 0);

typedef struct { char a; int32_t b; } int32_tAlign;
typedef struct { char a; uint32_t b; } uint32_tAlign;
typedef struct { char a; xint32 b; } xint32Align;
typedef struct { char a; xuint32 b; } xuint32Align;

X_STATIC_ASSERT(sizeof(xint32) == sizeof(int32_t));
X_STATIC_ASSERT(sizeof(xuint32) == sizeof(uint32_t));
// X_STATIC_ASSERT(X_ALIGNOF(xint32) == X_ALIGNOF(int32_t));
// X_STATIC_ASSERT(X_ALIGNOF(xuint32) == X_ALIGNOF(uint32_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xint32Align, b) == X_STRUCT_OFFSET(int32_tAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xuint32Align, b) == X_STRUCT_OFFSET(uint32_tAlign, b));

X_STATIC_ASSERT(X_SIGNEDNESS_OF(xint32) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xuint32) == 0);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(int32_t) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(uint32_t) == 0);

typedef struct { char a; int64_t b; } int64_tAlign;
typedef struct { char a; uint64_t b; } uint64_tAlign;
typedef struct { char a; xuint64 b; } xuint64Align;

X_STATIC_ASSERT(sizeof(xint64) == sizeof(int64_t));
X_STATIC_ASSERT(sizeof(xuint64) == sizeof(uint64_t));
// X_STATIC_ASSERT(X_ALIGNOF(xint64) == X_ALIGNOF(int64_t));
// X_STATIC_ASSERT(X_ALIGNOF(xuint64) == X_ALIGNOF(uint64_t));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xint64Align, b) == X_STRUCT_OFFSET(int64_tAlign, b));
X_STATIC_ASSERT(X_STRUCT_OFFSET(xuint64Align, b) == X_STRUCT_OFFSET(uint64_tAlign, b));

X_STATIC_ASSERT(X_SIGNEDNESS_OF(xint64) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(xuint64) == 0);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(int64_t) == 1);
X_STATIC_ASSERT(X_SIGNEDNESS_OF(uint64_t) == 0);

xboolean x_mem_gc_friendly = FALSE;
XLogLevelFlags x_log_always_fatal = (XLogLevelFlags)X_LOG_FATAL_MASK;
XLogLevelFlags x_log_msg_prefix = (XLogLevelFlags)(X_LOG_LEVEL_ERROR | X_LOG_LEVEL_WARNING | X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_DEBUG);

static xboolean debug_key_matches(const xchar *key, const xchar *token, xuint length)
{
    for (; length; length--, key++, token++) {
        char k = (*key   == '_') ? '-' : tolower(*key  );
        char t = (*token == '_') ? '-' : tolower(*token);

        if (k != t) {
            return FALSE;
        }
    }

    return *key == '\0';
}

X_STATIC_ASSERT(sizeof(int) == sizeof(xint32));

xuint x_parse_debug_string(const xchar *string, const XDebugKey *keys, xuint nkeys)
{
    xuint i;
    xuint result = 0;

    if (string == NULL) {
        return 0;
    }

    if (!strcasecmp(string, "help")) {
        fprintf(stderr, "Supported debug values:");
        for (i = 0; i < nkeys; i++) {
            fprintf(stderr, " %s", keys[i].key);
        }
        fprintf(stderr, " all help\n");
    } else {
        const xchar *q;
        const xchar *p = string;
        xboolean invert = FALSE;

        while (*p) {
            q = strpbrk(p, ":;, \t");
            if (!q) {
                q = p + strlen(p);
            }

            if (debug_key_matches("all", p, q - p)) {
                invert = TRUE;
            } else {
                for (i = 0; i < nkeys; i++) {
                    if (debug_key_matches(keys[i].key, p, q - p)) {
                        result |= keys[i].value;
                    }
                }
            }

            p = q;
            if (*p) {
                p++;
            }
        }

        if (invert) {
            xuint all_flags = 0;

            for (i = 0; i < nkeys; i++) {
                all_flags |= keys[i].value;
            }
            result = all_flags & (~result);
        }
    }

    return result;
}

static xuint x_parse_debug_envvar(const xchar *envvar, const XDebugKey *keys, xint n_keys, xuint default_value)
{
    const xchar *value;

    value = getenv(envvar);

    if (value == NULL) {
        return default_value;
    }

    return x_parse_debug_string(value, keys, n_keys);
}

static void x_messages_prefixed_init(void)
{
    const XDebugKey keys[] = {
        { "error",    X_LOG_LEVEL_ERROR },
        { "critical", X_LOG_LEVEL_CRITICAL },
        { "warning",  X_LOG_LEVEL_WARNING },
        { "message",  X_LOG_LEVEL_MESSAGE },
        { "info",     X_LOG_LEVEL_INFO },
        { "debug",    X_LOG_LEVEL_DEBUG }
    };

    x_log_msg_prefix = (XLogLevelFlags)x_parse_debug_envvar("X_MESSAGES_PREFIXED", keys, X_N_ELEMENTS(keys), x_log_msg_prefix);
}

static void x_debug_init(void)
{
    const XDebugKey keys[] = {
        { "gc-friendly",    1 },
        {"fatal-warnings",  X_LOG_LEVEL_WARNING | X_LOG_LEVEL_CRITICAL },
        {"fatal-criticals", X_LOG_LEVEL_CRITICAL }
    };

    XLogLevelFlags flags;
    flags = (XLogLevelFlags)x_parse_debug_envvar("X_DEBUG", keys, X_N_ELEMENTS(keys), 0);

    x_log_always_fatal = (XLogLevelFlags)(x_log_always_fatal | (flags & X_LOG_LEVEL_MASK));
    x_mem_gc_friendly = flags & 1;
}

void xlib_init(void)
{
    static xboolean xlib_inited;

    if (xlib_inited) {
        return;
    }

    xlib_inited = TRUE;

    x_messages_prefixed_init();
    x_debug_init();
    x_quark_init();
    x_error_init();
}

#if defined(X_HAS_CONSTRUCTORS)

#ifdef X_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma X_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(xlib_init_ctor)
#endif

X_DEFINE_CONSTRUCTOR(xlib_init_ctor)

static void xlib_init_ctor(void)
{
    xlib_init();
}

#else
#error Your platform/compiler is missing constructor support
#endif
