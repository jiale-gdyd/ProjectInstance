#ifndef __XLIBCONFIG_H__
#define __XLIBCONFIG_H__

#include "xmacros.h"

#include <float.h>
#include <limits.h>

#define XLIB_HAVE_ALLOCA_H
#define XLIB_USING_SYSTEM_PRINTF

X_BEGIN_DECLS

#define X_MINFLOAT                              FLT_MIN
#define X_MAXFLOAT                              FLT_MAX
#define X_MINDOUBLE                             DBL_MIN
#define X_MAXDOUBLE                             DBL_MAX
#define X_MINSHORT                              SHRT_MIN
#define X_MAXSHORT                              SHRT_MAX
#define X_MAXUSHORT                             USHRT_MAX
#define X_MININT                                INT_MIN
#define X_MAXINT                                INT_MAX
#define X_MAXUINT                               UINT_MAX
#define X_MINLONG                               LONG_MIN
#define X_MAXLONG                               LONG_MAX
#define X_MAXULONG                              ULONG_MAX

typedef signed char xint8;
typedef unsigned char xuint8;

typedef signed short xint16;
typedef unsigned short xuint16;

#define X_XINT16_MODIFIER                       "h"
#define X_XINT16_FORMAT                         "hi"
#define X_XUINT16_FORMAT                        "hu"

typedef signed int xint32;
typedef unsigned int xuint32;

#define X_XINT32_MODIFIER                       ""
#define X_XINT32_FORMAT                         "i"
#define X_XUINT32_FORMAT                        "u"

#define X_HAVE_XINT64                           1

#if defined(__x86_64__) || defined(__aarch64__)
typedef signed long xint64;
typedef unsigned long xuint64;

#define X_XINT64_CONSTANT(val)                  (val##L)
#define X_XUINT64_CONSTANT(val)                 (val##UL)

#define X_XINT64_MODIFIER                       "l"
#define X_XINT64_FORMAT                         "li"
#define X_XUINT64_FORMAT                        "lu"

#define XLIB_SIZEOF_VOID_P                      8
#define XLIB_SIZEOF_LONG                        8
#define XLIB_SIZEOF_SIZE_T                      8
#define XLIB_SIZEOF_SSIZE_T                     8

typedef signed long xssize;
typedef unsigned long xsize;

#define X_XSIZE_MODIFIER                        "l"
#define X_XSSIZE_MODIFIER                       "l"
#define X_XSIZE_FORMAT                          "lu"
#define X_XSSIZE_FORMAT                         "li"

#define X_MAXSIZE                               X_MAXULONG
#define X_MINSSIZE                              X_MINLONG
#define X_MAXSSIZE                              X_MAXLONG

#else

typedef signed long long xint64;
typedef unsigned long long xuint64;

#define X_XINT64_CONSTANT(val)                  (val##LL)
#define X_XUINT64_CONSTANT(val)                 (val##ULL)

#define X_XINT64_MODIFIER                       "ll"
#define X_XINT64_FORMAT                         "lli"
#define X_XUINT64_FORMAT                        "llu"

#define XLIB_SIZEOF_VOID_P                      4
#define XLIB_SIZEOF_LONG                        4
#define XLIB_SIZEOF_SIZE_T                      4
#define XLIB_SIZEOF_SSIZE_T                     4

typedef signed int xssize;
typedef unsigned int xsize;

#define X_XSIZE_MODIFIER                        ""
#define X_XSSIZE_MODIFIER                       ""
#define X_XSIZE_FORMAT                          "u"
#define X_XSSIZE_FORMAT                         "i"

#define X_MAXSIZE                               X_MAXUINT
#define X_MINSSIZE                              X_MININT
#define X_MAXSSIZE                              X_MAXINT
#endif

typedef xint64 xoffset;
#define X_MINOFFSET                             X_MININT64
#define X_MAXOFFSET                             X_MAXINT64

#define X_XOFFSET_MODIFIER                      X_XINT64_MODIFIER
#define X_XOFFSET_FORMAT                        X_XINT64_FORMAT
#define X_XOFFSET_CONSTANT(val)                 X_XINT64_CONSTANT(val)

#define X_POLLFD_FORMAT                         "%d"

#define XPOINTER_TO_INT(p)                      ((xint)(xlong)(p))
#define XPOINTER_TO_UINT(p)                     ((xuint)(xulong)(p))

#define XINT_TO_POINTER(i)                      ((xpointer)(xlong)(i))
#define XUINT_TO_POINTER(u)                     ((xpointer)(xulong)(u))

#if defined(__x86_64__) || defined(__aarch64__)
typedef signed long xintptr;
typedef unsigned long xuintptr;

#define X_XINTPTR_MODIFIER                      "l"
#define X_XINTPTR_FORMAT                        "li"
#define X_XUINTPTR_FORMAT                       "lu"

#else

typedef signed int xintptr;
typedef unsigned int xuintptr;

#define X_XINTPTR_MODIFIER                      ""
#define X_XINTPTR_FORMAT                        "i"
#define X_XUINTPTR_FORMAT                       "u"
#endif

#define XLIB_MAJOR_VERSION                      2
#define XLIB_MINOR_VERSION                      83
#define XLIB_MICRO_VERSION                      0

#define X_OS_UNIX

#define X_VA_COPY                               va_copy
#define X_VA_COPY_AS_ARRAY                      1

#ifndef __cplusplus
#define X_HAVE_ISO_VARARGS                      1
#endif

#ifdef __cplusplus
#define X_HAVE_ISO_VARARGS                      1
#endif

#if __GNUC__ == 2 && __GNUC_MINOR__ == 95
#undef X_HAVE_ISO_VARARGS
#endif

#define X_HAVE_GROWING_STACK                    0

#define X_HAVE_GNUC_VARARGS                     1

#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define X_GNUC_INTERNAL                         __attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define X_GNUC_INTERNAL                         __hidden
#elif defined(__GNUC__) && defined(X_HAVE_GNUC_VISIBILITY)
#define X_GNUC_INTERNAL                         __attribute__((visibility("hidden")))
#else
#define X_GNUC_INTERNAL
#endif

#define X_THREADS_ENABLED
#define X_THREADS_IMPL_POSIX

#undef X_ATOMIC_OP_MEMORY_BARRIER_NEEDED
#define X_ATOMIC_LOCK_FREE

#define XINT16_TO_LE(val)                       ((xint16)(val))
#define XUINT16_TO_LE(val)                      ((xuint16)(val))
#define XINT16_TO_BE(val)                       ((xint16)XUINT16_SWAP_LE_BE(val))
#define XUINT16_TO_BE(val)                      (XUINT16_SWAP_LE_BE(val))

#define XINT32_TO_LE(val)                       ((xint32)(val))
#define XUINT32_TO_LE(val)                      ((xuint32)(val))
#define XINT32_TO_BE(val)                       ((xint32)XUINT32_SWAP_LE_BE(val))
#define XUINT32_TO_BE(val)                      (XUINT32_SWAP_LE_BE(val))

#define XINT64_TO_LE(val)                       ((xint64)(val))
#define XUINT64_TO_LE(val)                      ((xuint64)(val))
#define XINT64_TO_BE(val)                       ((xint64)XUINT64_SWAP_LE_BE(val))
#define XUINT64_TO_BE(val)                      (XUINT64_SWAP_LE_BE(val))

#define XLONG_TO_LE(val)                        ((xlong)XINT64_TO_LE(val))
#define XULONG_TO_LE(val)                       ((xulong)XUINT64_TO_LE(val))
#define XLONG_TO_BE(val)                        ((xlong)XINT64_TO_BE(val))
#define XULONG_TO_BE(val)                       ((xulong)XUINT64_TO_BE(val))
#define XINT_TO_LE(val)                         ((xint)XINT32_TO_LE(val))
#define XUINT_TO_LE(val)                        ((xuint)XUINT32_TO_LE(val))
#define XINT_TO_BE(val)                         ((xint)XINT32_TO_BE(val))
#define XUINT_TO_BE(val)                        ((xuint)XUINT32_TO_BE(val))
#define XSIZE_TO_LE(val)                        ((xsize)XUINT64_TO_LE(val))
#define XSSIZE_TO_LE(val)                       ((xssize)XINT64_TO_LE(val))
#define XSIZE_TO_BE(val)                        ((xsize)XUINT64_TO_BE(val))
#define XSSIZE_TO_BE(val)                       ((xssize)XINT64_TO_BE(val))
#define X_BYTE_ORDER                            X_LITTLE_ENDIAN

#define XLIB_SYSDEF_POLLIN                      =1
#define XLIB_SYSDEF_POLLOUT                     =4
#define XLIB_SYSDEF_POLLPRI                     =2
#define XLIB_SYSDEF_POLLHUP                     =16
#define XLIB_SYSDEF_POLLERR                     =8
#define XLIB_SYSDEF_POLLNVAL                    =32

#define X_MODULE_SUFFIX                         "so"

typedef int XPid;
#define X_PID_FORMAT                            "i"

#define XLIB_SYSDEF_AF_UNIX                     1
#define XLIB_SYSDEF_AF_INET                     2
#define XLIB_SYSDEF_AF_INET6                    10

#define XLIB_SYSDEF_MSG_OOB                     1
#define XLIB_SYSDEF_MSG_PEEK                    2
#define XLIB_SYSDEF_MSG_DONTROUTE               4

#define X_DIR_SEPARATOR                         '/'
#define X_DIR_SEPARATOR_S                       "/"
#define X_SEARCHPATH_SEPARATOR                  ':'
#define X_SEARCHPATH_SEPARATOR_S                ":"

X_END_DECLS

#endif
