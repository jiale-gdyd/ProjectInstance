#ifndef __X_MACROS_H__
#define __X_MACROS_H__

#include <stddef.h>

#ifdef __GNUC__
#define X_GNUC_CHECK_VERSION(major, minor)                  ((__GNUC__ > (major)) || ((__GNUC__ == (major)) && (__GNUC_MINOR__ >= (minor))))
#else
#define X_GNUC_CHECK_VERSION(major, minor)                  0
#endif

#if X_GNUC_CHECK_VERSION(2, 8)
#define X_GNUC_EXTENSION                                    __extension__
#else
#define X_GNUC_EXTENSION
#endif

#if !defined (__cplusplus)

#undef X_CXX_STD_VERSION
#define X_CXX_STD_CHECK_VERSION(version)                    (0)

#if defined(__STDC_VERSION__)
#define X_C_STD_VERSION                                     __STDC_VERSION__
#else
#define X_C_STD_VERSION                                     199000L
#endif

#define X_C_STD_CHECK_VERSION(version)                      \
    (((version) >= 199000L && (version) <= X_C_STD_VERSION) \
    || ((version) == 89 && X_C_STD_VERSION >= 199000L)      \
    || ((version) == 90 && X_C_STD_VERSION >= 199000L)      \
    || ((version) == 99 && X_C_STD_VERSION >= 199901L)      \
    || ((version) == 11 && X_C_STD_VERSION >= 201112L)      \
    || ((version) == 17 && X_C_STD_VERSION >= 201710L)      \
    || (0))

#else

#undef X_C_STD_VERSION
#define X_C_STD_CHECK_VERSION(version)                      (0)

#define X_CXX_STD_VERSION                                   __cplusplus

# define X_CXX_STD_CHECK_VERSION(version)                       \
    (((version) >= 199711L && (version) <= X_CXX_STD_VERSION)   \
    || ((version) == 98 && X_CXX_STD_VERSION >= 199711L)        \
    || ((version) == 03 && X_CXX_STD_VERSION >= 199711L)        \
    || ((version) == 11 && X_CXX_STD_VERSION >= 201103L)        \
    || ((version) == 14 && X_CXX_STD_VERSION >= 201402L)        \
    || ((version) == 17 && X_CXX_STD_VERSION >= 201703L)        \
    || ((version) == 20 && X_CXX_STD_VERSION >= 202002L)        \
    || (0))

#endif

#define X_CAN_INLINE
#ifdef X_C_STD_VERSION
#if !X_C_STD_CHECK_VERSION(99)
#define X_INLINE_DEFINE_NEEDED
#endif
#endif

#ifdef X_INLINE_DEFINE_NEEDED
#undef inline
#define inline                                              __inline
#endif

#undef X_INLINE_DEFINE_NEEDED

#ifdef X_IMPLEMENT_INLINES
#define X_INLINE_FUNC                                       extern XLIB_DEPRECATED_MACRO_IN_2_48_FOR(static inline)
#undef  X_CAN_INLINE
#else
#define X_INLINE_FUNC                                       static inline XLIB_DEPRECATED_MACRO_IN_2_48_FOR(static inline)
#endif

#ifdef __has_attribute
#define x_macro__has_attribute                              __has_attribute
#else
#define x_macro__has_attribute(x)                           x_macro__has_attribute_##x

#define x_macro__has_attribute___alloc_size__               X_GNUC_CHECK_VERSION(4, 3)
#define x_macro__has_attribute___always_inline__            X_GNUC_CHECK_VERSION(2, 0)
#define x_macro__has_attribute___const__                    X_GNUC_CHECK_VERSION(2, 4)
#define x_macro__has_attribute___deprecated__               X_GNUC_CHECK_VERSION(3, 1)
#define x_macro__has_attribute___format__                   X_GNUC_CHECK_VERSION(2, 4)
#define x_macro__has_attribute___format_arg__               X_GNUC_CHECK_VERSION(2, 4)
#define x_macro__has_attribute___malloc__                   X_GNUC_CHECK_VERSION(2, 96)
#define x_macro__has_attribute___no_instrument_function__   X_GNUC_CHECK_VERSION(2, 4)
#define x_macro__has_attribute___noinline__                 X_GNUC_CHECK_VERSION(2, 96)
#define x_macro__has_attribute___noreturn__                 (X_GNUC_CHECK_VERSION(2, 8) || (0x5110 <= __SUNPRO_C))
#define x_macro__has_attribute___pure__                     X_GNUC_CHECK_VERSION(2, 96)
#define x_macro__has_attribute___sentinel__                 X_GNUC_CHECK_VERSION(4, 0)
#define x_macro__has_attribute___unused__                   X_GNUC_CHECK_VERSION(2, 4)
#define x_macro__has_attribute___weak__                     X_GNUC_CHECK_VERSION(2, 8)
#define x_macro__has_attribute_cleanup                      X_GNUC_CHECK_VERSION(3, 3)
#define x_macro__has_attribute_fallthrough                  X_GNUC_CHECK_VERSION(6, 0)
#define x_macro__has_attribute_may_alias                    X_GNUC_CHECK_VERSION(3, 3)
#define x_macro__has_attribute_warn_unused_result           X_GNUC_CHECK_VERSION(3, 4)
#endif

#if x_macro__has_attribute(__pure__)
#define X_GNUC_PURE                                         __attribute__((__pure__))
#else
#define X_GNUC_PURE
#endif

#if x_macro__has_attribute(__malloc__)
#define X_GNUC_MALLOC                                       __attribute__ ((__malloc__))
#else
#define X_GNUC_MALLOC
#endif

#if x_macro__has_attribute(__noinline__)
#define X_GNUC_NO_INLINE __attribute__                      ((__noinline__)) XLIB_AVAILABLE_MACRO_IN_2_58
#else
#define X_GNUC_NO_INLINE                                    XLIB_AVAILABLE_MACRO_IN_2_58
#endif

#if x_macro__has_attribute(__sentinel__)
#define X_GNUC_NULL_TERMINATED                              __attribute__((__sentinel__))
#else
#define X_GNUC_NULL_TERMINATED
#endif

#ifdef __has_feature
#define x_macro__has_feature                                __has_feature
#else
#define x_macro__has_feature(x)                             0
#endif

#ifdef __has_builtin
#define x_macro__has_builtin                                __has_builtin
#else
#define x_macro__has_builtin(x)                             0
#endif

#ifdef __has_extension
#define x_macro__has_extension                              __has_extension
#else
#define x_macro__has_extension(x)                           0
#endif

#if x_macro__has_attribute(__alloc_size__)
#define X_GNUC_ALLOC_SIZE(x)                                __attribute__((__alloc_size__(x)))
#define X_GNUC_ALLOC_SIZE2(x, y)                            __attribute__((__alloc_size__(x,y)))
#else
#define X_GNUC_ALLOC_SIZE(x)
#define X_GNUC_ALLOC_SIZE2(x, y)
#endif

#if x_macro__has_attribute(__format__)

#if !defined (__clang__) && X_GNUC_CHECK_VERSION (4, 4)
#define X_GNUC_PRINTF(format_idx, arg_idx)                  __attribute__((__format__ (gnu_printf, format_idx, arg_idx)))
#define X_GNUC_SCANF(format_idx, arg_idx)                   __attribute__((__format__ (gnu_scanf, format_idx, arg_idx)))
#define X_GNUC_STRFTIME(format_idx)                         __attribute__((__format__ (gnu_strftime, format_idx, 0))) XLIB_AVAILABLE_MACRO_IN_2_60
#else
#define X_GNUC_PRINTF(format_idx, arg_idx)                  __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#define X_GNUC_SCANF(format_idx, arg_idx)                   __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
#define X_GNUC_STRFTIME(format_idx)                         __attribute__((__format__ (__strftime__, format_idx, 0))) XLIB_AVAILABLE_MACRO_IN_2_60
#endif

#else

#define X_GNUC_PRINTF(format_idx, arg_idx)
#define X_GNUC_SCANF(format_idx, arg_idx)
#define X_GNUC_STRFTIME(format_idx)                         XLIB_AVAILABLE_MACRO_IN_2_60

#endif

#if x_macro__has_attribute(__format_arg__)
#define X_GNUC_FORMAT(arg_idx)                              __attribute__ ((__format_arg__ (arg_idx)))
#else
#define X_GNUC_FORMAT(arg_idx)
#endif

#if x_macro__has_attribute(__noreturn__)
#define X_GNUC_NORETURN                                     __attribute__ ((__noreturn__))
#else
#define X_GNUC_NORETURN
#endif

#if x_macro__has_attribute(__const__)
#define X_GNUC_CONST                                        __attribute__ ((__const__))
#else
#define X_GNUC_CONST
#endif

#if x_macro__has_attribute(__unused__)
#define X_GNUC_UNUSED                                       __attribute__ ((__unused__))
#else
#define X_GNUC_UNUSED
#endif

#if x_macro__has_attribute(__no_instrument_function__)
#define X_GNUC_NO_INSTRUMENT                                __attribute__ ((__no_instrument_function__))
#else
#define X_GNUC_NO_INSTRUMENT
#endif

#if x_macro__has_attribute(fallthrough)
#define X_GNUC_FALLTHROUGH                                  __attribute__((fallthrough)) XLIB_AVAILABLE_MACRO_IN_2_60
#else
#define X_GNUC_FALLTHROUGH                                  XLIB_AVAILABLE_MACRO_IN_2_60
#endif

#if x_macro__has_attribute(__deprecated__)
#define X_GNUC_DEPRECATED                                   __attribute__((__deprecated__))
#else
#define X_GNUC_DEPRECATED
#endif

#if X_GNUC_CHECK_VERSION(4, 5) || defined(__clang__)
#define X_GNUC_DEPRECATED_FOR(f)                            __attribute__((deprecated("Use " #f " instead"))) XLIB_AVAILABLE_MACRO_IN_2_26
#else
#define X_GNUC_DEPRECATED_FOR(f)                            X_GNUC_DEPRECATED XLIB_AVAILABLE_MACRO_IN_2_26
#endif

#ifdef __ICC
#define X_GNUC_BEGIN_IGNORE_DEPRECATIONS                    \
    _Pragma ("warning (push)")                              \
    _Pragma ("warning (disable:1478)")
#define X_GNUC_END_IGNORE_DEPRECATIONS                      \
    _Pragma ("warning (pop)")
#elif X_GNUC_CHECK_VERSION(4, 6)
#define X_GNUC_BEGIN_IGNORE_DEPRECATIONS                    \
    _Pragma ("GCC diagnostic push")                         \
    _Pragma ("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define X_GNUC_END_IGNORE_DEPRECATIONS                      \
    _Pragma ("GCC diagnostic pop")
#elif defined (__clang__)
#define X_GNUC_BEGIN_IGNORE_DEPRECATIONS                    \
    _Pragma("clang diagnostic push")                        \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define X_GNUC_END_IGNORE_DEPRECATIONS                      \
    _Pragma("clang diagnostic pop")
#else
#define X_GNUC_BEGIN_IGNORE_DEPRECATIONS
#define X_GNUC_END_IGNORE_DEPRECATIONS
#define XLIB_CANNOT_IGNORE_DEPRECATIONS
#endif

#if x_macro__has_attribute(may_alias)
#define X_GNUC_MAY_ALIAS                                    __attribute__((may_alias))
#else
#define X_GNUC_MAY_ALIAS
#endif

#if x_macro__has_attribute(warn_unused_result)
#define X_GNUC_WARN_UNUSED_RESULT                           __attribute__((warn_unused_result))
#else
#define X_GNUC_WARN_UNUSED_RESULT
#endif

#if defined (__GNUC__) && (__GNUC__ < 3)
#define X_GNUC_FUNCTION                                     __FUNCTION__ XLIB_DEPRECATED_MACRO_IN_2_26_FOR(X_STRFUNC)
#define X_GNUC_PRETTY_FUNCTION                              __PRETTY_FUNCTION__ XLIB_DEPRECATED_MACRO_IN_2_26_FOR(X_STRFUNC)
#else
#define X_GNUC_FUNCTION                                     "" XLIB_DEPRECATED_MACRO_IN_2_26_FOR(X_STRFUNC)
#define X_GNUC_PRETTY_FUNCTION                              "" XLIB_DEPRECATED_MACRO_IN_2_26_FOR(X_STRFUNC)
#endif

#if x_macro__has_feature(attribute_analyzer_noreturn) && defined(__clang_analyzer__)
#define X_ANALYZER_ANALYZING                                1
#define X_ANALYZER_NORETURN                                 __attribute__((analyzer_noreturn))
#elif defined(__COVERITY__)
#define X_ANALYZER_ANALYZING                                1
#define X_ANALYZER_NORETURN                                 __attribute__((noreturn))
#else
#define X_ANALYZER_ANALYZING                                0
#define X_ANALYZER_NORETURN
#endif

#define X_STRINGIFY(macro_or_string)                        X_STRINGIFY_ARG (macro_or_string)
#define X_STRINGIFY_ARG(contents)                           #contents

#ifndef __GI_SCANNER__
#define X_PASTE_ARGS(identifier1, identifier2)              identifier1 ## identifier2
#define X_PASTE(identifier1, identifier2)                   X_PASTE_ARGS(identifier1, identifier2)

#if X_CXX_STD_CHECK_VERSION(11)
#define X_STATIC_ASSERT(expr)                               static_assert(expr, "Expression evaluates to false")
#elif (X_C_STD_CHECK_VERSION(11) || x_macro__has_feature(c_static_assert) || x_macro__has_extension(c_static_assert))
#define X_STATIC_ASSERT(expr)                               _Static_assert(expr, "Expression evaluates to false")
#else
#ifdef __COUNTER__
#define X_STATIC_ASSERT(expr)                               typedef char X_PASTE(_XStaticAssertCompileTimeAssertion_, __COUNTER__)[(expr) ? 1 : -1] X_GNUC_UNUSED
#else
#define X_STATIC_ASSERT(expr)                               typedef char X_PASTE(_XStaticAssertCompileTimeAssertion_, __LINE__)[(expr) ? 1 : -1] X_GNUC_UNUSED
#endif
#endif

#define X_STATIC_ASSERT_EXPR(expr)                          ((void)sizeof(char[(expr) ? 1 : -1]))
#endif

#if defined(__GNUC__) && (__GNUC__ < 3) && !defined(X_CXX_STD_VERSION)
#define X_STRLOC                                            __FILE__ ":" X_STRINGIFY (__LINE__) ":" __PRETTY_FUNCTION__ "()"
#else
#define X_STRLOC                                            __FILE__ ":" X_STRINGIFY (__LINE__)
#endif

#if defined (__GNUC__) && defined(X_CXX_STD_VERSION)
#define X_STRFUNC                                           ((const char *)(__PRETTY_FUNCTION__))
#elif X_C_STD_CHECK_VERSION(99)
#define X_STRFUNC                                           ((const char *)(__func__))
#elif defined (__GNUC__)
#define X_STRFUNC                                           ((const char *)(__FUNCTION__))
#else
#define X_STRFUNC                                           ((const char *)("???"))
#endif

#ifdef X_CXX_STD_VERSION
#define X_BEGIN_DECLS                                       extern "C" {
#define X_END_DECLS                                         }
#else
#define X_BEGIN_DECLS
#define X_END_DECLS
#endif

#ifndef NULL
#if X_CXX_STD_CHECK_VERSION(11)
#define NULL                                                (nullptr)
#elif defined(X_CXX_STD_VERSION)
#define NULL                                                (0L)
#else
#define NULL                                                ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE                                               (0)
#endif

#ifndef TRUE
#define TRUE                                                (!FALSE)
#endif

#undef MAX
#define MAX(a, b)                                           (((a) > (b)) ? (a) : (b))

#undef MIN
#define MIN(a, b)                                           (((a) < (b)) ? (a) : (b))

#undef ABS
#define ABS(a)                                              (((a) < 0) ? -(a) : (a))

#undef CLAMP
#define CLAMP(x, low, high)                                 (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define X_APPROX_VALUE(a, b, epsilon)                       (((a) > (b) ? (a) - (b) : (b) - (a)) < (epsilon))

#define X_N_ELEMENTS(arr)                                   (sizeof(arr) / sizeof((arr)[0]))

#define XPOINTER_TO_SIZE(p)                                 ((xsize)(p))
#define XSIZE_TO_POINTER(s)                                 ((xpointer)(xuintptr)(xsize)(s))

#if X_GNUC_CHECK_VERSION(4, 0)
#define X_OFFSETOF(st_type, mem_name)                       (xsize)&(((st_type *)0)->mem_name)

#define X_STRUCT_OFFSET(struct_type, member)                ((xlong)offsetof(struct_type, member))
//#define X_STRUCT_OFFSET(struct_type, member)                ((xlong)X_OFFSETOF(struct_type, member))
#else
#define X_STRUCT_OFFSET(struct_type, member)                ((xlong)((xuint8 *)&((struct_type *)0)->member))
#endif

#define X_STRUCT_MEMBER_P(struct_p, struct_offset)              ((xpointer)((xuint8 *)(struct_p) + (xlong)(struct_offset)))
#define X_STRUCT_MEMBER(member_type, struct_p, struct_offset)   (*(member_type *)X_STRUCT_MEMBER_P((struct_p), (struct_offset)))

#if !(defined(X_STMT_START) && defined(X_STMT_END))
#define X_STMT_START                                        do
#define X_STMT_END                                          while (0)
#endif

#if X_C_STD_CHECK_VERSION(11)
#define X_ALIGNOF(type)                                     _Alignof(type) XLIB_AVAILABLE_MACRO_IN_2_60
#else
#define X_ALIGNOF(type)                                     (X_STRUCT_OFFSET(struct { char a; type b; }, b)) XLIB_AVAILABLE_MACRO_IN_2_60
#endif

#ifdef X_DISABLE_CONST_RETURNS
#define X_CONST_RETURN                                      XLIB_DEPRECATED_MACRO_IN_2_30_FOR(const)
#else
#define X_CONST_RETURN                                      const XLIB_DEPRECATED_MACRO_IN_2_30_FOR(const)
#endif

#if X_CXX_STD_CHECK_VERSION(11)
#define X_NORETURN                                          [[noreturn]]
#elif x_macro__has_attribute(__noreturn__)
#define X_NORETURN                                          __attribute__ ((__noreturn__))
#elif X_C_STD_CHECK_VERSION(11)
#define X_NORETURN                                          _Noreturn
#else
#define X_NORETURN
#endif

#if x_macro__has_attribute(__noreturn__)
#define X_NORETURN_FUNCPTR                                  __attribute__ ((__noreturn__)) XLIB_AVAILABLE_MACRO_IN_2_68
#else
#define X_NORETURN_FUNCPTR                                  XLIB_AVAILABLE_MACRO_IN_2_68
#endif

#if x_macro__has_attribute(__always_inline__)
#if X_CXX_STD_CHECK_VERSION(11)
#define X_ALWAYS_INLINE                                     [[gnu::always_inline]]
#else
#define X_ALWAYS_INLINE                                     __attribute__ ((__always_inline__))
#endif
#else
#define X_ALWAYS_INLINE
#endif

#if x_macro__has_attribute(__noinline__)
#if X_CXX_STD_CHECK_VERSION(11)
#if defined(__GNUC__)
#define X_NO_INLINE                                         [[gnu::noinline]]
#endif
#else
#define X_NO_INLINE                                         __attribute__ ((__noinline__))
#endif
#else
#define X_NO_INLINE
#endif

#if X_GNUC_CHECK_VERSION(2, 0) && defined(__OPTIMIZE__)
#define _X_BOOLEAN_EXPR_IMPL(uniq, expr)                    \
    X_GNUC_EXTENSION ({                                     \
        int X_PASTE(_x_boolean_var_, uniq);                 \
        if (expr) {                                         \
            X_PASTE(_x_boolean_var_, uniq) = 1;             \
        } else {                                            \
            X_PASTE(_x_boolean_var_, uniq) = 0;             \
        }                                                   \
        X_PASTE(_x_boolean_var_, uniq);                     \
    })

#define _X_BOOLEAN_EXPR(expr)                               _X_BOOLEAN_EXPR_IMPL(__COUNTER__, expr)

#define X_LIKELY(expr)                                      (__builtin_expect(_X_BOOLEAN_EXPR(expr), 1))
#define X_UNLIKELY(expr)                                    (__builtin_expect(_X_BOOLEAN_EXPR(expr), 0))
#else
#define X_LIKELY(expr)                                      (expr)
#define X_UNLIKELY(expr)                                    (expr)
#endif

#if __GNUC__ >= 4
#define X_HAVE_GNUC_VISIBILITY                              1
#endif

#if defined(XLIB_CANNOT_IGNORE_DEPRECATIONS)
#define X_DEPRECATED
#elif X_GNUC_CHECK_VERSION(3, 1) || defined(__clang__)
#define X_DEPRECATED                                        __attribute__((__deprecated__))
#else
#define X_DEPRECATED
#endif

#if defined(XLIB_CANNOT_IGNORE_DEPRECATIONS)
#define X_DEPRECATED_FOR(f)                                 X_DEPRECATED
#elif X_GNUC_CHECK_VERSION(4, 5) || defined(__clang__)
#define X_DEPRECATED_FOR(f)                                 __attribute__((__deprecated__("Use '" #f "' instead")))
#else
#define X_DEPRECATED_FOR(f)                                 X_DEPRECATED
#endif

#if X_GNUC_CHECK_VERSION(4, 5) || defined(__clang__)
#define X_UNAVAILABLE(maj, min)                             __attribute__((deprecated("Not available before " #maj "." #min)))
#else
#define X_UNAVAILABLE(maj, min)                             X_DEPRECATED
#endif

#ifndef _XLIB_EXTERN
#define _XLIB_EXTERN                                        extern
#endif

#ifdef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DEPRECATED                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_FOR(f)                              _XLIB_EXTERN
#define XLIB_UNAVAILABLE(maj, min)                          _XLIB_EXTERN
#define XLIB_UNAVAILABLE_STATIC_INLINE(maj, min)
#else
#define XLIB_DEPRECATED                                     X_DEPRECATED _XLIB_EXTERN
#define XLIB_DEPRECATED_FOR(f)                              X_DEPRECATED_FOR(f) _XLIB_EXTERN
#define XLIB_UNAVAILABLE(maj, min)                          X_UNAVAILABLE(maj, min) _XLIB_EXTERN
#define XLIB_UNAVAILABLE_STATIC_INLINE(maj, min)            X_UNAVAILABLE(maj, min)
#endif

#if !defined(XLIB_DISABLE_DEPRECATION_WARNINGS) && (X_GNUC_CHECK_VERSION(4, 6) || __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 4))
#define _XLIB_GNUC_DO_PRAGMA(x)                             _Pragma(X_STRINGIFY(x))
#define XLIB_DEPRECATED_MACRO _                             XLIB_GNUC_DO_PRAGMA(GCC warning "Deprecated pre-processor symbol")
#define XLIB_DEPRECATED_MACRO_FOR(f)                        _XLIB_GNUC_DO_PRAGMA(GCC warning X_STRINGIFY(Deprecated pre-processor symbol: replace with #f))
#define XLIB_UNAVAILABLE_MACRO(maj, min)                    _XLIB_GNUC_DO_PRAGMA(GCC warning X_STRINGIFY(Not available before maj.min))
#else
#define XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_UNAVAILABLE_MACRO(maj, min)
#endif

#if !defined(XLIB_DISABLE_DEPRECATION_WARNINGS) && (X_GNUC_CHECK_VERSION(6, 1) || (defined (__clang_major__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 0))))
#define XLIB_DEPRECATED_ENUMERATOR                          X_DEPRECATED
#define XLIB_DEPRECATED_ENUMERATOR_FOR(f)                   X_DEPRECATED_FOR(f)
#define XLIB_UNAVAILABLE_ENUMERATOR(maj, min)               X_UNAVAILABLE(maj, min)
#else
#define XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_UNAVAILABLE_ENUMERATOR(maj, min)
#endif

#if !defined(XLIB_DISABLE_DEPRECATION_WARNINGS) && (X_GNUC_CHECK_VERSION(3, 1) || (defined (__clang_major__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 0))))
#define XLIB_DEPRECATED_TYPE                                X_DEPRECATED
#define XLIB_DEPRECATED_TYPE_FOR(f)                         X_DEPRECATED_FOR(f)
#define XLIB_UNAVAILABLE_TYPE(maj, min)                     X_UNAVAILABLE(maj, min)
#else
#define XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_FOR(f)
#define XLIB_UNAVAILABLE_TYPE(maj, min)
#endif

#ifndef __GI_SCANNER__

#if x_macro__has_attribute(cleanup)

#define _XLIB_AUTOPTR_FUNC_NAME(TypeName)                   xlib_autoptr_cleanup_##TypeName
#define _XLIB_AUTOPTR_CLEAR_FUNC_NAME(TypeName)             xlib_autoptr_clear_##TypeName
#define _XLIB_AUTOPTR_TYPENAME(TypeName)                    TypeName##_autoptr
#define _XLIB_AUTOPTR_LIST_FUNC_NAME(TypeName)              xlib_listautoptr_cleanup_##TypeName
#define _XLIB_AUTOPTR_LIST_TYPENAME(TypeName)               TypeName##_listautoptr
#define _XLIB_AUTOPTR_SLIST_FUNC_NAME(TypeName)             xlib_slistautoptr_cleanup_##TypeName
#define _XLIB_AUTOPTR_SLIST_TYPENAME(TypeName)              TypeName##_slistautoptr
#define _XLIB_AUTOPTR_QUEUE_FUNC_NAME(TypeName)             xlib_queueautoptr_cleanup_##TypeName
#define _XLIB_AUTOPTR_QUEUE_TYPENAME(TypeName)              TypeName##_queueautoptr
#define _XLIB_AUTO_FUNC_NAME(TypeName)                      xlib_auto_cleanup_##TypeName
#define _XLIB_CLEANUP(func)                                 __attribute__((cleanup(func)))

#define _XLIB_DEFINE_AUTOPTR_CLEANUP_FUNCS(TypeName, ParentName, cleanup)                                                       \
    typedef TypeName *_XLIB_AUTOPTR_TYPENAME(TypeName);                                                                         \
    typedef XList *_XLIB_AUTOPTR_LIST_TYPENAME(TypeName);                                                                       \
    typedef XSList *_XLIB_AUTOPTR_SLIST_TYPENAME(TypeName);                                                                     \
    typedef XQueue *_XLIB_AUTOPTR_QUEUE_TYPENAME(TypeName);                                                                     \
                                                                                                                                \
    X_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                                            \
    static X_GNUC_UNUSED inline void _XLIB_AUTOPTR_CLEAR_FUNC_NAME(TypeName)(TypeName *_ptr)                                    \
    {                                                                                                                           \
        if (_ptr) {                                                                                                             \
            (cleanup)((ParentName *)_ptr);                                                                                      \
        }                                                                                                                       \
    }                                                                                                                           \
    static X_GNUC_UNUSED inline void _XLIB_AUTOPTR_FUNC_NAME(TypeName)(TypeName **_ptr)                                         \
    {                                                                                                                           \
        _XLIB_AUTOPTR_CLEAR_FUNC_NAME(TypeName)(*_ptr);                                                                         \
    }                                                                                                                           \
    static X_GNUC_UNUSED inline void _XLIB_AUTOPTR_LIST_FUNC_NAME(TypeName)(XList **_l)                                         \
    {                                                                                                                           \
        x_list_free_full(*_l, (XDestroyNotify)(void(*)(void))cleanup);                                                          \
    }                                                                                                                           \
    static X_GNUC_UNUSED inline void _XLIB_AUTOPTR_SLIST_FUNC_NAME(TypeName)(XSList **_l)                                       \
    {                                                                                                                           \
        x_slist_free_full(*_l, (XDestroyNotify)(void(*)(void))cleanup);                                                         \
    }                                                                                                                           \
    static X_GNUC_UNUSED inline void _XLIB_AUTOPTR_QUEUE_FUNC_NAME(TypeName)(XQueue **_q)                                       \
    {                                                                                                                           \
        if (*_q) {                                                                                                              \
            x_queue_free_full(*_q, (XDestroyNotify)(void(*)(void))cleanup);                                                     \
        }                                                                                                                       \
    }                                                                                                                           \
    X_GNUC_END_IGNORE_DEPRECATIONS

#define _XLIB_DEFINE_AUTOPTR_CHAINUP(ModuleObjName, ParentName) \
    _XLIB_DEFINE_AUTOPTR_CLEANUP_FUNCS(ModuleObjName, ParentName, _XLIB_AUTOPTR_CLEAR_FUNC_NAME(ParentName))

#define X_DEFINE_AUTOPTR_CLEANUP_FUNC(TypeName, func) \
    _XLIB_DEFINE_AUTOPTR_CLEANUP_FUNCS(TypeName, TypeName, func)

#define X_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(TypeName, func)                                                                        \
    X_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                                            \
    static X_GNUC_UNUSED inline void _XLIB_AUTO_FUNC_NAME(TypeName)(TypeName *_ptr)                                             \
    {                                                                                                                           \
        (func)(_ptr);                                                                                                           \
    }                                                                                                                           \
    X_GNUC_END_IGNORE_DEPRECATIONS

#define X_DEFINE_AUTO_CLEANUP_FREE_FUNC(TypeName, func, none)                                                                   \
    X_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                                            \
    static X_GNUC_UNUSED inline void _XLIB_AUTO_FUNC_NAME(TypeName)(TypeName *_ptr)                                             \
    {                                                                                                                           \
        if (*_ptr != none) {                                                                                                    \
            (func)(*_ptr);                                                                                                      \
        }                                                                                                                       \
    }                                                                                                                           \
    X_GNUC_END_IGNORE_DEPRECATIONS

#define x_autoptr(TypeName)                                 _XLIB_CLEANUP(_XLIB_AUTOPTR_FUNC_NAME(TypeName)) _XLIB_AUTOPTR_TYPENAME(TypeName)
#define x_autolist(TypeName)                                _XLIB_CLEANUP(_XLIB_AUTOPTR_LIST_FUNC_NAME(TypeName)) _XLIB_AUTOPTR_LIST_TYPENAME(TypeName)
#define x_autoslist(TypeName)                               _XLIB_CLEANUP(_XLIB_AUTOPTR_SLIST_FUNC_NAME(TypeName)) _XLIB_AUTOPTR_SLIST_TYPENAME(TypeName)
#define x_autoqueue(TypeName)                               _XLIB_CLEANUP(_XLIB_AUTOPTR_QUEUE_FUNC_NAME(TypeName)) _XLIB_AUTOPTR_QUEUE_TYPENAME(TypeName)
#define x_auto(TypeName)                                    _XLIB_CLEANUP(_XLIB_AUTO_FUNC_NAME(TypeName)) TypeName
#define x_autofree                                          _XLIB_CLEANUP(x_autoptr_cleanup_generic_xfree)

#else
#define _XLIB_DEFINE_AUTOPTR_CHAINUP(ModuleObjName, ParentName)

#define X_DEFINE_AUTOPTR_CLEANUP_FUNC(TypeName, func)
#define X_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(TypeName, func)
#define X_DEFINE_AUTO_CLEANUP_FREE_FUNC(TypeName, func, none)

#endif
#else

#define _XLIB_DEFINE_AUTOPTR_CHAINUP(ModuleObjName, ParentName)

#define X_DEFINE_AUTOPTR_CLEANUP_FUNC(TypeName, func)
#define X_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(TypeName, func)
#define X_DEFINE_AUTO_CLEANUP_FREE_FUNC(TypeName, func, none)
#endif

#define X_SIZEOF_MEMBER(struct_type, member)                XLIB_AVAILABLE_MACRO_IN_2_64 sizeof(((struct_type *)0)->member)

#endif
