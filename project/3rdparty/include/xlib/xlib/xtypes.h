#ifndef __X_TYPES_H__
#define __X_TYPES_H__

#include "config.h"
#include "xlibconfig.h"

#include "xmacros.h"
#include "xversionmacros.h"
#include "xlib-visibility.h"

#include <time.h>

X_BEGIN_DECLS

typedef int xint;
typedef char xchar;
typedef long xlong;
typedef short xshort;
typedef xint xboolean;

typedef unsigned int xuint;
typedef unsigned char xuchar;
typedef unsigned long xulong;
typedef unsigned short xushort;

typedef float xfloat;
typedef double xdouble;

#define X_MININT8                           ((xint8)(-X_MAXINT8 - 1))
#define X_MAXINT8                           ((xint8)0x7f)
#define X_MAXUINT8                          ((xuint8)0xff)

#define X_MININT16                          ((xint16)(-X_MAXINT16 - 1))
#define X_MAXINT16                          ((xint16)0x7fff)
#define X_MAXUINT16                         ((xuint16)0xffff)

#define X_MININT32                          ((xint32)(-X_MAXINT32 - 1))
#define X_MAXINT32                          ((xint32)0x7fffffff)
#define X_MAXUINT32                         ((xuint32)0xffffffff)

#define X_MININT64                          ((xint64)(-X_MAXINT64 - X_XINT64_CONSTANT(1)))
#define X_MAXINT64                          X_XINT64_CONSTANT(0x7fffffffffffffff)
#define X_MAXUINT64                         X_XUINT64_CONSTANT(0xffffffffffffffff)

typedef void* xpointer;
typedef const void *xconstpointer;

typedef xint (*XCompareFunc)(xconstpointer a, xconstpointer b);
typedef xint (*XCompareDataFunc)(xconstpointer a, xconstpointer b, xpointer user_data);
typedef xboolean (*XEqualFunc)(xconstpointer a, xconstpointer b);

typedef xboolean (*XEqualFuncFull)(xconstpointer a, xconstpointer b, xpointer user_data);

typedef void (*XDestroyNotify)(xpointer data);
typedef void (*XFunc)(xpointer data, xpointer user_data);

typedef xuint (*XHashFunc)(xconstpointer key);
typedef void (*XHFunc)(xpointer key, xpointer value, xpointer user_data);

typedef void (*XFreeFunc)(xpointer data);
typedef xpointer (*XCopyFunc)(xconstpointer src, xpointer data);
typedef const xchar *(*XTranslateFunc)(const xchar *str, xpointer data);

#define X_E                                 2.7182818284590452353602874713526624977572470937000
#define X_LN2                               0.69314718055994530941723212145817656807550013436026
#define X_LN10                              2.3025850929940456840179914546843642076011014886288
#define X_PI                                3.1415926535897932384626433832795028841971693993751
#define X_PI_2                              1.5707963267948966192313216916397514420985846996876
#define X_PI_4                              0.78539816339744830961566084581987572104929234984378
#define X_SQRT2                             1.4142135623730950488016887242096980785696718753769

#define X_LITTLE_ENDIAN                     1234
#define X_BIG_ENDIAN                        4321
#define X_PDP_ENDIAN                        3412

#define XUINT16_SWAP_LE_BE_CONSTANT(val)    \
    ((xuint16)(                             \
    (xuint16)((xuint16)(val) >> 8) |        \
    (xuint16)((xuint16)(val) << 8)))

#define XUINT32_SWAP_LE_BE_CONSTANT(val)              \
    ((xuint32)(                                       \
    (((xuint32)(val) & (xuint32)0x000000ffU) << 24) | \
    (((xuint32)(val) & (xuint32)0x0000ff00U) <<  8) | \
    (((xuint32)(val) & (xuint32)0x00ff0000U) >>  8) | \
    (((xuint32)(val) & (xuint32)0xff000000U) >> 24)))

#define XUINT64_SWAP_LE_BE_CONSTANT(val)                            \
    ((xuint64)(                                                     \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x00000000000000ffU)) << 56) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x000000000000ff00U)) << 40) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x0000000000ff0000U)) << 24) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x00000000ff000000U)) <<  8) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x000000ff00000000U)) >>  8) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x0000ff0000000000U)) >> 24) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0x00ff000000000000U)) >> 40) |       \
    (((xuint64)(val) &                                              \
    (xuint64)X_XINT64_CONSTANT(0xff00000000000000U)) >> 56)))

#if defined(__GNUC__) && (__GNUC__ >= 2) && defined(__OPTIMIZE__)

#if __GNUC__ >= 4 && defined (__GNUC_MINOR__) && __GNUC_MINOR__ >= 3
#define XUINT32_SWAP_LE_BE(val)             ((xuint32) __builtin_bswap32((xuint32)(val)))
#define XUINT64_SWAP_LE_BE(val)             ((xuint64) __builtin_bswap64((xuint64)(val)))
#endif

#if defined (__i386__)
#define XUINT16_SWAP_LE_BE_IA32(val)                        \
    (X_GNUC_EXTENSION                                       \
    ({ xuint16 __v, __x = ((xuint16) (val));                \
    if (__builtin_constant_p (__x))                         \
        __v = XUINT16_SWAP_LE_BE_CONSTANT (__x);            \
    else                                                    \
        __asm__ ("rorw $8, %w0"                             \
            : "=r" (__v)                                    \
            : "0" (__x)                                     \
            : "cc");                                        \
        __v; }))
#if !defined(__i486__) && !defined(__i586__) && !defined(__pentium__) && !defined(__i686__) && !defined(__pentiumpro__) && !defined(__pentium4__)
#define XUINT32_SWAP_LE_BE_IA32(val)                        \
    (X_GNUC_EXTENSION                                       \
    ({ xuint32 __v, __x = ((xuint32) (val));                \
        if (__builtin_constant_p (__x))                     \
        __v = XUINT32_SWAP_LE_BE_CONSTANT (__x);            \
        else                                                \
        __asm__ ("rorw $8, %w0\n\t"                         \
            "rorl $16, %0\n\t"                              \
            "rorw $8, %w0"                                  \
            : "=r" (__v)                                    \
            : "0" (__x)                                     \
            : "cc");                                        \
        __v; }))
#else
#define XUINT32_SWAP_LE_BE_IA32(val)                        \
    (X_GNUC_EXTENSION                                       \
    ({ xuint32 __v, __x = ((xuint32) (val));                \
        if (__builtin_constant_p (__x))                     \
        __v = XUINT32_SWAP_LE_BE_CONSTANT (__x);            \
        else                                                \
        __asm__ ("bswap %0"                                 \
            : "=r" (__v)                                    \
            : "0" (__x));                                   \
        __v; }))
#endif
#define XUINT64_SWAP_LE_BE_IA32(val)                        \
    (X_GNUC_EXTENSION                                       \
    ({ union { xuint64 __ll;                                \
        xuint32 __l[2]; } __w, __r;                         \
    __w.__ll = ((xuint64) (val));                           \
    if (__builtin_constant_p (__w.__ll))                    \
        __r.__ll = XUINT64_SWAP_LE_BE_CONSTANT (__w.__ll);  \
    else                                                    \
        {                                                   \
        __r.__l[0] = XUINT32_SWAP_LE_BE (__w.__l[1]);       \
        __r.__l[1] = XUINT32_SWAP_LE_BE (__w.__l[0]);       \
        }                                                   \
    __r.__ll; }))

#define XUINT16_SWAP_LE_BE(val)             (XUINT16_SWAP_LE_BE_IA32 (val))

#ifndef XUINT32_SWAP_LE_BE
#define XUINT32_SWAP_LE_BE(val)             (XUINT32_SWAP_LE_BE_IA32 (val))
#endif

#ifndef XUINT64_SWAP_LE_BE
#define XUINT64_SWAP_LE_BE(val)             (XUINT64_SWAP_LE_BE_IA32 (val))
#endif

#elif defined (__ia64__)
#define XUINT16_SWAP_LE_BE_IA64(val)                        \
    (x_GNUC_EXTENSION                                       \
    ({ xuint16 __v, __x = ((xuint16) (val));                \
    if (__builtin_constant_p (__x))                         \
        __v = xUINT16_SWAP_LE_BE_CONSTANT (__x);            \
    else                                                    \
        __asm__ __volatile__ ("shl %0 = %1, 48 ;;"          \
                "mux1 %0 = %0, @rev ;;"                     \
                    : "=r" (__v)                            \
                    : "r" (__x));                           \
        __v; }))
#define XUINT32_SWAP_LE_BE_IA64(val)                        \
    (X_GNUC_EXTENSION                                       \
    ({ xuint32 __v, __x = ((xuint32) (val));                \
        if (__builtin_constant_p (__x))                     \
        __v = XUINT32_SWAP_LE_BE_CONSTANT (__x);            \
        else                                                \
        __asm__ __volatile__ ("shl %0 = %1, 32 ;;"          \
                "mux1 %0 = %0, @rev ;;"                     \
                    : "=r" (__v)                            \
                    : "r" (__x));                           \
        __v; }))
#define XUINT64_SWAP_LE_BE_IA64(val)                        \
    (X_GNUC_EXTENSION                                       \
    ({ xuint64 __v, __x = ((xuint64) (val));                \
    if (__builtin_constant_p (__x))                         \
        __v = XUINT64_SWAP_LE_BE_CONSTANT (__x);            \
    else                                                    \
        __asm__ __volatile__ ("mux1 %0 = %1, @rev ;;"       \
                : "=r" (__v)                                \
                : "r" (__x));                               \
    __v; }))

#define XUINT16_SWAP_LE_BE(val)         (XUINT16_SWAP_LE_BE_IA64 (val))

#ifndef XUINT32_SWAP_LE_BE
#define XUINT32_SWAP_LE_BE(val)         (XUINT32_SWAP_LE_BE_IA64 (val))
#endif

#ifndef XUINT64_SWAP_LE_BE
#define XUINT64_SWAP_LE_BE(val)         (XUINT64_SWAP_LE_BE_IA64 (val))
#endif

#elif defined (__x86_64__)
#define XUINT32_SWAP_LE_BE_X86_64(val)                      \
    (X_GNUC_EXTENSION                                       \
    ({ xuint32 __v, __x = ((xuint32) (val));                \
        if (__builtin_constant_p (__x))                     \
        __v = XUINT32_SWAP_LE_BE_CONSTANT (__x);            \
        else                                                \
        __asm__ ("bswapl %0"                                \
            : "=r" (__v)                                    \
            : "0" (__x));                                   \
        __v; }))
#define XUINT64_SWAP_LE_BE_X86_64(val)                      \
    (X_GNUC_EXTENSION                                       \
    ({ xuint64 __v, __x = ((xuint64) (val));                \
    if (__builtin_constant_p (__x))                         \
        __v = XUINT64_SWAP_LE_BE_CONSTANT (__x);            \
    else                                                    \
        __asm__ ("bswapq %0"                                \
            : "=r" (__v)                                    \
            : "0" (__x));                                   \
    __v; }))

#define XUINT16_SWAP_LE_BE(val)         (XUINT16_SWAP_LE_BE_CONSTANT (val))

#ifndef XUINT32_SWAP_LE_BE
#define XUINT32_SWAP_LE_BE(val)         (XUINT32_SWAP_LE_BE_X86_64 (val))
#endif

#ifndef XUINT64_SWAP_LE_BE
#define XUINT64_SWAP_LE_BE(val)         (XUINT64_SWAP_LE_BE_X86_64 (val))
#endif

#else

#define XUINT16_SWAP_LE_BE(val)         (XUINT16_SWAP_LE_BE_CONSTANT (val))

#ifndef XUINT32_SWAP_LE_BE
#define XUINT32_SWAP_LE_BE(val)         (XUINT32_SWAP_LE_BE_CONSTANT (val))
#endif

#ifndef XUINT64_SWAP_LE_BE
#define XUINT64_SWAP_LE_BE(val)         (XUINT64_SWAP_LE_BE_CONSTANT (val))
#endif

#endif
#else
#define XUINT16_SWAP_LE_BE(val)         (XUINT16_SWAP_LE_BE_CONSTANT (val))
#define XUINT32_SWAP_LE_BE(val)         (XUINT32_SWAP_LE_BE_CONSTANT (val))
#define XUINT64_SWAP_LE_BE(val)         (XUINT64_SWAP_LE_BE_CONSTANT (val))
#endif

#define XUINT16_SWAP_LE_PDP(val)        ((xuint16) (val))
#define XUINT16_SWAP_BE_PDP(val)        (XUINT16_SWAP_LE_BE (val))
#define XUINT32_SWAP_LE_PDP(val)        ((xuint32) ( \
    (((xuint32) (val) & (xuint32) 0x0000ffffU) << 16) | \
    (((xuint32) (val) & (xuint32) 0xffff0000U) >> 16)))

#define XUINT32_SWAP_BE_PDP(val)        ((xuint32) ( \
    (((xuint32) (val) & (xuint32) 0x00ff00ffU) << 8) | \
    (((xuint32) (val) & (xuint32) 0xff00ff00U) >> 8)))

#define XINT16_FROM_LE(val)             (XINT16_TO_LE (val))
#define XUINT16_FROM_LE(val)            (XUINT16_TO_LE (val))
#define XINT16_FROM_BE(val)             (XINT16_TO_BE (val))
#define XUINT16_FROM_BE(val)            (XUINT16_TO_BE (val))
#define XINT32_FROM_LE(val)             (XINT32_TO_LE (val))
#define XUINT32_FROM_LE(val)            (XUINT32_TO_LE (val))
#define XINT32_FROM_BE(val)             (XINT32_TO_BE (val))
#define XUINT32_FROM_BE(val)            (XUINT32_TO_BE (val))

#define XINT64_FROM_LE(val)             (XINT64_TO_LE (val))
#define XUINT64_FROM_LE(val)            (XUINT64_TO_LE (val))
#define XINT64_FROM_BE(val)             (XINT64_TO_BE (val))
#define XUINT64_FROM_BE(val)            (XUINT64_TO_BE (val))

#define XLONG_FROM_LE(val)              (XLONG_TO_LE (val))
#define XULONG_FROM_LE(val)             (XULONG_TO_LE (val))
#define XLONG_FROM_BE(val)              (XLONG_TO_BE (val))
#define XULONG_FROM_BE(val)             (XULONG_TO_BE (val))

#define XINT_FROM_LE(val)               (XINT_TO_LE (val))
#define XUINT_FROM_LE(val)              (XUINT_TO_LE (val))
#define XINT_FROM_BE(val)               (XINT_TO_BE (val))
#define XUINT_FROM_BE(val)              (XUINT_TO_BE (val))

#define XSIZE_FROM_LE(val)              (XSIZE_TO_LE (val))
#define XSSIZE_FROM_LE(val)             (XSSIZE_TO_LE (val))
#define XSIZE_FROM_BE(val)              (XSIZE_TO_BE (val))
#define XSSIZE_FROM_BE(val)             (XSSIZE_TO_BE (val))

#define x_ntohl(val)                    (XUINT32_FROM_BE (val))
#define x_ntohs(val)                    (XUINT16_FROM_BE (val))
#define x_htonl(val)                    (XUINT32_TO_BE (val))
#define x_htons(val)                    (XUINT16_TO_BE (val))

#ifndef _XLIB_TEST_OVERFLOW_FALLBACK
#if __GNUC__ >= 5 && !defined(__INTEL_COMPILER)
#define _XLIB_HAVE_BUILTIN_OVERFLOW_CHECKS
#elif x_macro__has_builtin(__builtin_add_overflow)
#define _XLIB_HAVE_BUILTIN_OVERFLOW_CHECKS
#endif
#endif

#ifdef _XLIB_HAVE_BUILTIN_OVERFLOW_CHECKS

#define x_uint_checked_add(dest, a, b)      (!__builtin_add_overflow(a, b, dest))
#define x_uint_checked_mul(dest, a, b)      (!__builtin_mul_overflow(a, b, dest))

#define x_uint64_checked_add(dest, a, b)    (!__builtin_add_overflow(a, b, dest))
#define x_uint64_checked_mul(dest, a, b)    (!__builtin_mul_overflow(a, b, dest))

#define x_size_checked_add(dest, a, b)      (!__builtin_add_overflow(a, b, dest))
#define x_size_checked_mul(dest, a, b)      (!__builtin_mul_overflow(a, b, dest))

#else

static inline xboolean _XLIB_CHECKED_ADD_UINT(xuint *dest, xuint a, xuint b)
{
    *dest = a + b;
    return *dest >= a;
}

static inline xboolean _XLIB_CHECKED_MUL_UINT(xuint *dest, xuint a, xuint b)
{
    *dest = a * b;
    return !a || *dest / a == b;
}

static inline xboolean _XLIB_CHECKED_ADD_UINT64(xuint64 *dest, xuint64 a, xuint64 b)
{
    *dest = a + b;
    return *dest >= a;
}

static inline xboolean _XLIB_CHECKED_MUL_UINT64(xuint64 *dest, xuint64 a, xuint64 b)
{
    *dest = a * b;
    return !a || *dest / a == b;
}

static inline xboolean _XLIB_CHECKED_ADD_SIZE(xsize *dest, xsize a, xsize b)
{
    *dest = a + b;
    return *dest >= a;
}

static inline xboolean _XLIB_CHECKED_MUL_SIZE(xsize *dest, xsize a, xsize b)
{
    *dest = a * b;
    return !a || *dest / a == b;
}

#define x_uint_checked_add(dest, a, b)      _XLIB_CHECKED_ADD_UINT(dest, a, b)
#define x_uint_checked_mul(dest, a, b)      _XLIB_CHECKED_MUL_UINT(dest, a, b)

#define x_uint64_checked_add(dest, a, b)    _XLIB_CHECKED_ADD_UINT64(dest, a, b)
#define x_uint64_checked_mul(dest, a, b)    _XLIB_CHECKED_MUL_UINT64(dest, a, b)

#define x_size_checked_add(dest, a, b)      _XLIB_CHECKED_ADD_SIZE(dest, a, b)
#define x_size_checked_mul(dest, a, b)      _XLIB_CHECKED_MUL_SIZE(dest, a, b)

#endif

typedef union _XDoubleIEEE754 XDoubleIEEE754;
typedef union _XFloatIEEE754 XFloatIEEE754;

#define X_IEEE754_FLOAT_BIAS                (127)
#define X_IEEE754_DOUBLE_BIAS               (1023)

#define X_LOG_2_BASE_10                     (0.30102999566398119521)

#if x_BYTE_ORDER == x_LITTLE_ENDIAN
union _XFloatIEEE754 {
    xfloat    v_float;
    struct {
        xuint mantissa : 23;
        xuint biased_exponent : 8;
        xuint sign : 1;
    } mpn;
};

union _XDoubleIEEE754 {
    xdouble   v_double;
    struct {
        xuint mantissa_low : 32;
        xuint mantissa_high : 20;
        xuint biased_exponent : 11;
        xuint sign : 1;
    } mpn;
};
#elif X_BYTE_ORDER == X_BIG_ENDIAN
union _XFloatIEEE754 {
    xfloat    v_float;
    struct {
        xuint sign : 1;
        xuint biased_exponent : 8;
        xuint mantissa : 23;
    } mpn;
};
union _XDoubleIEEE754 {
    xdouble   v_double;
    struct {
        xuint sign : 1;
        xuint biased_exponent : 11;
        xuint mantissa_high : 20;
        xuint mantissa_low : 32;
    } mpn;
};
#else
#error unknown ENDIAN type
#endif

typedef struct _XTimeVal XTimeVal XLIB_DEPRECATED_TYPE_IN_2_62_FOR(XDateTime);

struct _XTimeVal {
    xlong tv_sec;
    xlong tv_usec;
} XLIB_DEPRECATED_TYPE_IN_2_62_FOR(XDateTime);

typedef xint xrefcount;
typedef xint xatomicrefcount;

X_END_DECLS

#ifndef XLIB_VAR
#define XLIB_VAR                            _XLIB_EXTERN
#endif

#endif
