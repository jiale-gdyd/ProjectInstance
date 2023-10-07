#ifndef __X_TEST_UTILS_H__
#define __X_TEST_UTILS_H__

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "xerror.h"
#include "xslist.h"
#include "xstring.h"
#include "xmessages.h"

X_BEGIN_DECLS

typedef struct XTestCase XTestCase;
typedef struct XTestSuite XTestSuite;

typedef void (*XTestFunc)(void);
typedef void (*XTestDataFunc)(xconstpointer user_data);
typedef void (*XTestFixtureFunc)(xpointer fixture, xconstpointer user_data);

#define x_assert_cmpstr(s1, cmp, s2)                                                                                                                            \
    X_STMT_START {                                                                                                                                              \
        const char *__s1 = (s1), *__s2 = (s2);                                                                                                                  \
        if (x_strcmp0(__s1, __s2) cmp 0) {                                                                                                                      \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpstr(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #s1 " " #cmp " " #s2, __s1, #cmp, __s2);                                    \
        }                                                                                                                                                       \
    } X_STMT_END

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_78
#define x_assert_cmpint(n1, cmp, n2)                                                                                                                            \
    X_STMT_START {                                                                                                                                              \
        xint64 __n1 = (n1), __n2 = (n2);                                                                                                                        \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpint(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, (xuint64)__n1, #cmp, (xuint64)__n2, 'i');             \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_cmpuint(n1, cmp, n2)                                                                                                                           \
    X_STMT_START {                                                                                                                                              \
        xuint64 __n1 = (n1), __n2 = (n2);                                                                                                                       \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpint(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, __n1, #cmp, __n2, 'u');                               \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_cmphex(n1, cmp, n2)                                                                                                                            \
    X_STMT_START {                                                                                                                                              \
        xuint64 __n1 = (n1), __n2 = (n2);                                                                                                                       \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpint(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, __n1, #cmp, __n2, 'x');                               \
        }                                                                                                                                                       \
    } X_STMT_END
#else
#define x_assert_cmpint(n1, cmp, n2)                                                                                                                            \
    X_STMT_START {                                                                                                                                              \
        xint64 __n1 = (n1), __n2 = (n2);                                                                                                                        \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpnum(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, (long double)__n1, #cmp, (long double)__n2, 'i');     \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_cmpuint(n1, cmp, n2)                                                                                                                           \
    X_STMT_START {                                                                                                                                              \
        xuint64 __n1 = (n1), __n2 = (n2);                                                                                                                       \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpnum(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, (long double)__n1, #cmp, (long double)__n2, 'i');     \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_cmphex(n1, cmp, n2)                                                                                                                            \
    X_STMT_START {                                                                                                                                              \
        xuint64 __n1 = (n1), __n2 = (n2);                                                                                                                       \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpnum(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, (long double)__n1, #cmp, (long double)__n2, 'x');     \
        }                                                                                                                                                       \
    } X_STMT_END
#endif

#define x_assert_cmpfloat(n1, cmp, n2)                                                                                                                          \
    X_STMT_START {                                                                                                                                              \
        long double __n1 = (long double)(n1), __n2 = (long double)(n2);                                                                                         \
        if (__n1 cmp __n2) {                                                                                                                                    \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpnum(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " " #cmp " " #n2, (long double)__n1, #cmp, (long double)__n2, 'f');     \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_cmpfloat_with_epsilon(n1, n2, epsilon)                                                                                                         \
    X_STMT_START {                                                                                                                                              \
        double __n1 = (n1), __n2 = (n2), __epsilon = (epsilon);                                                                                                 \
        if (X_APPROX_VALUE(__n1,  __n2, __epsilon)) {                                                                                                           \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message_cmpnum(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #n1 " == " #n2 " (+/- " #epsilon ")", __n1, "==", __n2, 'f');               \
        }                                                                                                                                                       \
    } X_STMT_END

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_78
#define x_assert_cmpmem(m1, l1, m2, l2)                                                                                                                         \
    X_STMT_START {                                                                                                                                              \
        xconstpointer __m1 = m1, __m2 = m2;                                                                                                                     \
        size_t __l1 = (size_t)l1, __l2 = (size_t)l2;                                                                                                            \
        if (__l1 != 0 && __m1 == NULL) {                                                                                                                        \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #l1 " == 0 || " #m1 " != NULL)");                             \
        } else if (__l2 != 0 && __m2 == NULL) {                                                                                                                 \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #l2 " == 0 || " #m2 " != NULL)");                             \
        } else if (__l1 != __l2) {                                                                                                                              \
            x_assertion_message_cmpint(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #l1 " (len(" #m1 ")) == " #l2 " (len(" #m2 "))", __l1, "==", __l2, 'u');    \
        } else if (__l1 != 0 && __m2 != NULL && memcmp(__m1, __m2, __l1) != 0) {                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #m1 " == " #m2 ")");                                          \
        }                                                                                                                                                       \
    } X_STMT_END
#else
#define x_assert_cmpmem(m1, l1, m2, l2)                                                                                                                         \
    X_STMT_START {                                                                                                                                              \
        xconstpointer __m1 = m1, __m2 = m2;                                                                                                                     \
        size_t __l1 = (size_t)l1, __l2 = (size_t)l2;                                                                                                            \
        if (__l1 != 0 && __m1 == NULL) {                                                                                                                        \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #l1 " == 0 || " #m1 " != NULL)");                             \
        } else if (__l2 != 0 && __m2 == NULL) {                                                                                                                 \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #l2 " == 0 || " #m2 " != NULL)");                             \
        } else if (__l1 != __l2) {                                                                                                                              \
            x_assertion_message_cmpnum(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #l1 " (len(" #m1 ")) == " #l2 " (len(" #m2 "))", (long double)__l1, "==", (long double)__l2, 'i'); \
        } else if (__l1 != 0 && __m2 != NULL && memcmp(__m1, __m2, __l1) != 0) {                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #m1 " == " #m2 ")");                                          \
        }                                                                                                                                                       \
    } X_STMT_END
#endif

#define x_assert_cmpvariant(v1, v2)                                                                                                                             \
    X_STMT_START {                                                                                                                                              \
        XVariant *__v1 = (v1), *__v2 = (v2);                                                                                                                    \
        if (!x_variant_equal(__v1, __v2)) {                                                                                                                     \
            xchar *__s1, *__s2, *__msg;                                                                                                                         \
            __s1 = x_variant_print(__v1, TRUE);                                                                                                                 \
            __s2 = x_variant_print(__v2, TRUE);                                                                                                                 \
            __msg = x_strdup_printf("assertion failed (" #v1 " == " #v2 "): %s does not equal %s", __s1, __s2);                                                 \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, __msg);                                                                            \
            x_free(__s1);                                                                                                                                       \
            x_free(__s2);                                                                                                                                       \
            x_free(__msg);                                                                                                                                      \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_cmpstrv(strv1, strv2)                                                                                                                          \
    X_STMT_START {                                                                                                                                              \
        const char *const *__strv1 = (const char *const *) (strv1);                                                                                             \
        const char *const *__strv2 = (const char *const *) (strv2);                                                                                             \
        if (!__strv1 || !__strv2) {                                                                                                                             \
            if (__strv1) {                                                                                                                                      \
                x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #strv1 " == " #strv2 "): " #strv2 " is NULL, but " #strv1 " is not"); \
            } else if (__strv2) {                                                                                                                               \
                x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "assertion failed (" #strv1 " == " #strv2 "): " #strv1 " is NULL, but " #strv2 " is not"); \
            }                                                                                                                                                   \
        } else {                                                                                                                                                \
            xuint __l1 = x_strv_length((char **) __strv1);                                                                                                      \
            xuint __l2 = x_strv_length((char **) __strv2);                                                                                                      \
            if (__l1 != __l2) {                                                                                                                                 \
                char *__msg;                                                                                                                                    \
                __msg = x_strdup_printf("assertion failed (" #strv1 " == " #strv2 "): length %u does not equal length %u", __l1, __l2);                         \
                x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, __msg);                                                                        \
                x_free(__msg);                                                                                                                                  \
            } else {                                                                                                                                            \
                xuint __i;                                                                                                                                      \
                for (__i = 0; __i < __l1; __i++) {                                                                                                              \
                    if (x_strcmp0(__strv1[__i], __strv2[__i]) != 0) {                                                                                           \
                        x_assertion_message_cmpstrv(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #strv1 " == " #strv2, __strv1, __strv2, __i);                  \
                    }                                                                                                                                           \
                }                                                                                                                                               \
            }                                                                                                                                                   \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_no_errno(expr)                                                                                                                                 \
    X_STMT_START {                                                                                                                                              \
        int __ret, __errsv;                                                                                                                                     \
        errno = 0;                                                                                                                                              \
        __ret = expr;                                                                                                                                           \
        __errsv = errno;                                                                                                                                        \
        if (__ret < 0) {                                                                                                                                        \
            xchar *__msg;                                                                                                                                       \
            __msg = x_strdup_printf("assertion failed (" #expr " >= 0): errno %i: %s", __errsv, x_strerror(__errsv));                                           \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, __msg);                                                                            \
            x_free(__msg);                                                                                                                                      \
        }                                                                                                                                                       \
    } X_STMT_END                                                                                                                                                \
    XLIB_AVAILABLE_MACRO_IN_2_66

#define x_assert_no_error(err)                                                                                                                                  \
    X_STMT_START {                                                                                                                                              \
        if (err) {                                                                                                                                              \
            x_assertion_message_error(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #err, err, 0, 0);                                                            \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_error(err, dom, c)                                                                                                                             \
    X_STMT_START {                                                                                                                                              \
        if (!err || (err)->domain != dom || (err)->code != c) {                                                                                                 \
            x_assertion_message_error(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #err, err, dom, c);                                                          \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_true(expr)                                                                                                                                     \
    X_STMT_START {                                                                                                                                              \
        if X_LIKELY(expr) {                                                                                                                                     \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "'" #expr "' should be TRUE");                                                     \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_false(expr)                                                                                                                                    \
    X_STMT_START {                                                                                                                                              \
        if X_LIKELY(!(expr)) {                                                                                                                                  \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "'" #expr "' should be FALSE");                                                    \
        }                                                                                                                                                       \
    } X_STMT_END

#if X_CXX_STD_CHECK_VERSION(11)
#define x_assert_null(expr)                                                                                                                                     \
    X_STMT_START {                                                                                                                                              \
        if X_LIKELY((expr) == nullptr) {                                                                                                                        \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "'" #expr "' should be nullptr");                                                  \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_nonnull(expr)                                                                                                                                  \
    X_STMT_START {                                                                                                                                              \
        if X_LIKELY ((expr) != nullptr) {                                                                                                                       \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "'" #expr "' should not be nullptr");                                              \
        }                                                                                                                                                       \
    } X_STMT_END

#else

#define x_assert_null(expr)                                                                                                                                     \
    X_STMT_START {                                                                                                                                              \
        if X_LIKELY((expr) == NULL) {                                                                                                                           \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "'" #expr "' should be NULL");                                                     \
        }                                                                                                                                                       \
    } X_STMT_END

#define x_assert_nonnull(expr)                                                                                                                                  \
    X_STMT_START {                                                                                                                                              \
        if X_LIKELY((expr) != NULL) {                                                                                                                           \
            ;                                                                                                                                                   \
        } else {                                                                                                                                                \
            x_assertion_message(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, "'" #expr "' should not be NULL");                                                 \
        }                                                                                                                                                       \
    } X_STMT_END
#endif

#ifdef X_DISABLE_ASSERT
#if __GNUC__ >= 5 || x_macro__has_builtin(__builtin_unreachable)
#define x_assert_not_reached()          X_STMT_START { (void) 0; __builtin_unreachable (); } X_STMT_END
#else
#define x_assert_not_reached()          X_STMT_START { (void) 0; } X_STMT_END
#endif

#define x_assert(expr)                  X_STMT_START { (void) 0; } X_STMT_END
#else
#define x_assert_not_reached()          X_STMT_START { x_assertion_message_expr(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, NULL); } X_STMT_END

#define x_assert(expr)                                                                                                                                        \
    X_STMT_START {                                                                                                                                            \
        if X_LIKELY (expr) {                                                                                                                                  \
            ;                                                                                                                                                 \
        } else {                                                                                                                                              \
            x_assertion_message_expr(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, #expr);                                                                     \
        }                                                                                                                                                     \
    } X_STMT_END
#endif

XLIB_AVAILABLE_IN_ALL
int x_strcmp0(const char *str1, const char *str2);

XLIB_AVAILABLE_IN_ALL
void x_test_minimized_result(double minimized_quantity, const char *format, ...) X_GNUC_PRINTF(2, 3);

XLIB_AVAILABLE_IN_ALL
void x_test_maximized_result(double maximized_quantity, const char *format, ...) X_GNUC_PRINTF (2, 3);

XLIB_AVAILABLE_IN_ALL
void x_test_init(int *argc, char ***argv, ...) X_GNUC_NULL_TERMINATED;

#define X_TEST_OPTION_ISOLATE_DIRS                  "isolate_dirs"

#ifdef X_DISABLE_ASSERT
#if defined(X_HAVE_ISO_VARARGS)
#define x_test_init(argc, argv, ...)                                                                            \
    X_STMT_START {                                                                                              \
        x_printerr("Tests were compiled with X_DISABLE_ASSERT and are likely no-ops. Aborting.\n");             \
        exit(1);                                                                                                \
    } X_STMT_END
#elif defined(X_HAVE_GNUC_VARARGS)
#define x_test_init(argc, argv...)                                                                              \
    X_STMT_START {                                                                                              \
        x_printerr("Tests were compiled with X_DISABLE_ASSERT and are likely no-ops. Aborting.\n");             \
        exit(1);                                                                                                \
    } X_STMT_END
#else
#endif
#endif

#define x_test_initialized()            (x_test_config_vars->test_initialized)
#define x_test_quick()                  (x_test_config_vars->test_quick)
#define x_test_slow()                   (!x_test_config_vars->test_quick)
#define x_test_thorough()               (!x_test_config_vars->test_quick)
#define x_test_perf()                   (x_test_config_vars->test_perf)
#define x_test_verbose()                (x_test_config_vars->test_verbose)
#define x_test_quiet()                  (x_test_config_vars->test_quiet)
#define x_test_undefined()              (x_test_config_vars->test_undefined)

XLIB_AVAILABLE_IN_2_38
xboolean x_test_subprocess(void);

XLIB_AVAILABLE_IN_ALL
int x_test_run(void);

XLIB_AVAILABLE_IN_ALL
void x_test_add_func(const char *testpath, XTestFunc test_func);

XLIB_AVAILABLE_IN_ALL
void x_test_add_data_func(const char *testpath, xconstpointer test_data, XTestDataFunc test_func);

XLIB_AVAILABLE_IN_2_34
void x_test_add_data_func_full(const char *testpath, xpointer test_data, XTestDataFunc test_func, XDestroyNotify data_free_func);

XLIB_AVAILABLE_IN_2_68
const char *x_test_get_path(void);

XLIB_AVAILABLE_IN_2_30
void x_test_fail(void);

XLIB_AVAILABLE_IN_2_70
void x_test_fail_printf(const char *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_2_38
void x_test_incomplete(const xchar *msg);

XLIB_AVAILABLE_IN_2_70
void x_test_incomplete_printf(const char *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_2_38
void x_test_skip(const xchar *msg);

XLIB_AVAILABLE_IN_2_70
void x_test_skip_printf(const char *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_2_38
xboolean x_test_failed(void);

XLIB_AVAILABLE_IN_2_38
void x_test_set_nonfatal_assertions(void);

XLIB_AVAILABLE_IN_2_78
void x_test_disable_crash_reporting(void);

#define x_test_add(testpath, Fixture, tdata, fsetup, ftest, fteardown)  \
    X_STMT_START {                                                      \
        void (*add_vtable) (const char *,                               \
                xsize,                                                  \
                xconstpointer,                                          \
                void (*)(Fixture *, xconstpointer),                     \
                void (*)(Fixture *, xconstpointer),                     \
                void (*)(Fixture *, xconstpointer)) =  (void (*)(const xchar *, xsize, xconstpointer, void (*)(Fixture *, xconstpointer), void (*)(Fixture *, xconstpointer), void (*)(Fixture *, xconstpointer))) x_test_add_vtable; \
        add_vtable                                                      \
        (testpath, sizeof (Fixture), tdata, fsetup, ftest, fteardown);  \
    } X_STMT_END

XLIB_AVAILABLE_IN_ALL
void x_test_message(const char *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_ALL
void x_test_bug_base(const char *uri_pattern);

XLIB_AVAILABLE_IN_ALL
void x_test_bug(const char *bug_uri_snippet);

XLIB_AVAILABLE_IN_2_62
void x_test_summary(const char *summary);

XLIB_AVAILABLE_IN_ALL
void x_test_timer_start(void);

XLIB_AVAILABLE_IN_ALL
double x_test_timer_elapsed(void);

XLIB_AVAILABLE_IN_ALL
double x_test_timer_last(void);

XLIB_AVAILABLE_IN_ALL
void x_test_queue_free(xpointer gfree_pointer);

XLIB_AVAILABLE_IN_ALL
void x_test_queue_destroy(XDestroyNotify destroy_func, xpointer destroy_data);

#define x_test_queue_unref(gobject)     x_test_queue_destroy(x_object_unref, gobject)

typedef enum {
    X_TEST_TRAP_DEFAULT XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_TEST_TRAP_SILENCE_STDOUT = 1 << 7,
    X_TEST_TRAP_SILENCE_STDERR = 1 << 8,
    X_TEST_TRAP_INHERIT_STDIN  = 1 << 9
} XTestTrapFlags XLIB_DEPRECATED_TYPE_IN_2_38_FOR(XTestSubprocessFlags);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XLIB_DEPRECATED_IN_2_38_FOR(x_test_trap_subprocess)
xboolean x_test_trap_fork(xuint64 usec_timeout, XTestTrapFlags test_trap_flags);

X_GNUC_END_IGNORE_DEPRECATIONS

typedef enum {
    X_TEST_SUBPROCESS_DEFAULT XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_TEST_SUBPROCESS_INHERIT_STDIN  = 1 << 0,
    X_TEST_SUBPROCESS_INHERIT_STDOUT = 1 << 1,
    X_TEST_SUBPROCESS_INHERIT_STDERR = 1 << 2
} XTestSubprocessFlags;

XLIB_AVAILABLE_IN_2_38
void x_test_trap_subprocess(const char *test_path, xuint64 usec_timeout, XTestSubprocessFlags test_flags);

XLIB_AVAILABLE_IN_ALL
xboolean x_test_trap_has_passed(void);

XLIB_AVAILABLE_IN_ALL
xboolean x_test_trap_reached_timeout(void);

#define x_test_trap_assert_passed()                         x_test_trap_assertions(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, 0, 0)
#define x_test_trap_assert_failed()                         x_test_trap_assertions(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, 1, 0)
#define x_test_trap_assert_stdout(soutpattern)              x_test_trap_assertions(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, 2, soutpattern)
#define x_test_trap_assert_stdout_unmatched(soutpattern)    x_test_trap_assertions(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, 3, soutpattern)
#define x_test_trap_assert_stderr(serrpattern)              x_test_trap_assertions(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, 4, serrpattern)
#define x_test_trap_assert_stderr_unmatched(serrpattern)    x_test_trap_assertions(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC, 5, serrpattern)

#define x_test_rand_bit()                                   (0 != (x_test_rand_int() & (1 << 15)))

XLIB_AVAILABLE_IN_ALL
xint32 x_test_rand_int(void);

XLIB_AVAILABLE_IN_ALL
xint32 x_test_rand_int_range(xint32 begin, xint32 end);

XLIB_AVAILABLE_IN_ALL
double x_test_rand_double(void);

XLIB_AVAILABLE_IN_ALL
double x_test_rand_double_range(double range_start, double range_end);

XLIB_AVAILABLE_IN_ALL
XTestCase *x_test_create_case(const char *test_name, xsize data_size, xconstpointer test_data, XTestFixtureFunc data_setup, XTestFixtureFunc data_test, XTestFixtureFunc data_teardown);

XLIB_AVAILABLE_IN_ALL
XTestSuite *x_test_create_suite(const char *suite_name);

XLIB_AVAILABLE_IN_ALL
XTestSuite *x_test_get_root(void);

XLIB_AVAILABLE_IN_ALL
void x_test_suite_add(XTestSuite *suite, XTestCase *test_case);

XLIB_AVAILABLE_IN_ALL
void x_test_suite_add_suite(XTestSuite *suite, XTestSuite *nestedsuite);

XLIB_AVAILABLE_IN_ALL
int x_test_run_suite(XTestSuite *suite);

XLIB_AVAILABLE_IN_2_70
void x_test_case_free(XTestCase *test_case);

XLIB_AVAILABLE_IN_2_70
void x_test_suite_free(XTestSuite *suite);

XLIB_AVAILABLE_IN_ALL
void x_test_trap_assertions(const char *domain, const char *file, int line, const char *func, xuint64 assertion_flags, const char *pattern);

XLIB_AVAILABLE_IN_ALL
void x_assertion_message(const char *domain, const char *file, int line, const char *func, const char *message) X_ANALYZER_NORETURN;

X_NORETURN
XLIB_AVAILABLE_IN_ALL
void x_assertion_message_expr(const char *domain, const char *file, int line, const char *func, const char *expr);

XLIB_AVAILABLE_IN_ALL
void x_assertion_message_cmpstr(const char *domain, const char *file, int line, const char *func, const char *expr, const char *arg1, const char *cmp, const char *arg2) X_ANALYZER_NORETURN;

XLIB_AVAILABLE_IN_2_68
void x_assertion_message_cmpstrv(const char *domain, const char *file, int line, const char *func, const char *expr, const char *const *arg1, const char *const *arg2, xsize first_wrong_idx) X_ANALYZER_NORETURN;

XLIB_AVAILABLE_IN_2_78
void x_assertion_message_cmpint(const char *domain, const char *file, int line, const char *func, const char *expr, xuint64 arg1, const char *cmp, xuint64 arg2, char numtype) X_ANALYZER_NORETURN;

XLIB_AVAILABLE_IN_ALL
void x_assertion_message_cmpnum(const char *domain, const char *file, int line, const char *func, const char *expr, long double arg1, const char *cmp, long double arg2, char numtype) X_ANALYZER_NORETURN;

XLIB_AVAILABLE_IN_ALL
void x_assertion_message_error(const char *domain, const char *file, int line, const char *func, const char *expr, const XError *error, XQuark error_domain, int error_code) X_ANALYZER_NORETURN;

XLIB_AVAILABLE_IN_ALL
void x_test_add_vtable(const char *testpath, xsize data_size, xconstpointer test_data, XTestFixtureFunc data_setup, XTestFixtureFunc data_test, XTestFixtureFunc data_teardown);

typedef struct {
    xboolean test_initialized;
    xboolean test_quick;
    xboolean test_perf;
    xboolean test_verbose;
    xboolean test_quiet;
    xboolean test_undefined;
} XTestConfig;
XLIB_VAR const XTestConfig *const x_test_config_vars;

typedef enum {
    X_TEST_RUN_SUCCESS,
    X_TEST_RUN_SKIPPED,
    X_TEST_RUN_FAILURE,
    X_TEST_RUN_INCOMPLETE
} XTestResult;

typedef enum {
    X_TEST_LOG_NONE,
    X_TEST_LOG_ERROR,
    X_TEST_LOG_START_BINARY,
    X_TEST_LOG_LIST_CASE,
    X_TEST_LOG_SKIP_CASE, 
    X_TEST_LOG_START_CASE,
    X_TEST_LOG_STOP_CASE,
    X_TEST_LOG_MIN_RESULT,
    X_TEST_LOG_MAX_RESULT,
    X_TEST_LOG_MESSAGE,
    X_TEST_LOG_START_SUITE,
    X_TEST_LOG_STOP_SUITE
} XTestLogType;

typedef struct {
    XTestLogType log_type;
    xuint        n_strings;
    xchar        **strings;
    xuint        n_nums;
    long double  *nums;
} XTestLogMsg;

typedef struct {
    XString *data;
    XSList  *msgs;
} XTestLogBuffer;

XLIB_AVAILABLE_IN_ALL
const char *x_test_log_type_name(XTestLogType log_type);

XLIB_AVAILABLE_IN_ALL
XTestLogBuffer *x_test_log_buffer_new(void);

XLIB_AVAILABLE_IN_ALL
void x_test_log_buffer_free(XTestLogBuffer *tbuffer);

XLIB_AVAILABLE_IN_ALL
void x_test_log_buffer_push(XTestLogBuffer *tbuffer, xuint n_bytes, const xuint8 *bytes);

XLIB_AVAILABLE_IN_ALL
XTestLogMsg *x_test_log_buffer_pop(XTestLogBuffer *tbuffer);

XLIB_AVAILABLE_IN_ALL
void x_test_log_msg_free(XTestLogMsg *tmsg);

typedef xboolean (*XTestLogFatalFunc)(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_test_log_set_fatal_handler(XTestLogFatalFunc log_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_34
void x_test_expect_message(const xchar *log_domain, XLogLevelFlags log_level, const xchar *pattern);

XLIB_AVAILABLE_IN_2_34
void x_test_assert_expected_messages_internal(const char *domain, const char *file, int line, const char *func);

typedef enum {
    X_TEST_DIST,
    X_TEST_BUILT
} XTestFileType;

XLIB_AVAILABLE_IN_2_38
xchar *x_test_build_filename(XTestFileType file_type, const xchar *first_path, ...) X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_2_38
const xchar *x_test_get_dir(XTestFileType file_type);

XLIB_AVAILABLE_IN_2_38
const xchar *x_test_get_filename(XTestFileType file_type, const xchar *first_path, ...) X_GNUC_NULL_TERMINATED;

#define x_test_assert_expected_messages()       x_test_assert_expected_messages_internal(X_LOG_DOMAIN, __FILE__, __LINE__, X_STRFUNC)

X_END_DECLS

#endif
