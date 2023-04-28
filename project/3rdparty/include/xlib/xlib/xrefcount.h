#ifndef __X_REFCOUNT_H__
#define __X_REFCOUNT_H__

#include "xatomic.h"
#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_2_58
void x_ref_count_init(xrefcount *rc);

XLIB_AVAILABLE_IN_2_58
void x_ref_count_inc(xrefcount *rc);

XLIB_AVAILABLE_IN_2_58
xboolean x_ref_count_dec(xrefcount *rc);

XLIB_AVAILABLE_IN_2_58
xboolean x_ref_count_compare(xrefcount *rc, xint val);

XLIB_AVAILABLE_IN_2_58
void x_atomic_ref_count_init(xatomicrefcount *arc);

XLIB_AVAILABLE_IN_2_58
void x_atomic_ref_count_inc(xatomicrefcount *arc);

XLIB_AVAILABLE_IN_2_58
xboolean x_atomic_ref_count_dec(xatomicrefcount *arc);

XLIB_AVAILABLE_IN_2_58
xboolean x_atomic_ref_count_compare(xatomicrefcount *arc, xint val);

#define X_REF_COUNT_INIT          -1 XLIB_AVAILABLE_MACRO_IN_2_78
#define X_ATOMIC_REF_COUNT_INIT   1  XLIB_AVAILABLE_MACRO_IN_2_78

#if defined(__GNUC__) && defined(X_DISABLE_CHECKS)

#define x_ref_count_init(rc)                                                                \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xrefcount));                                 \
        (void)(0 ? *(rc) ^ *(rc) : 1);                                                      \
        *(rc) = -1;                                                                         \
    }))

#define x_ref_count_inc(rc)                                                                 \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xrefcount));                                 \
        (void)(0 ? *(rc) ^ *(rc) : 1);                                                      \
        if (*(rc) == X_MININT) {                                                            \
            ;                                                                               \
        } else {                                                                            \
            *(rc) -= 1;                                                                     \
        }                                                                                   \
    }))

#define x_ref_count_dec(rc)                                                                 \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xrefcount));                                 \
        xrefcount __rc = *(rc);                                                             \
        __rc += 1;                                                                          \
        if (__rc == 0) {                                                                    \
            ;                                                                               \
        } else {                                                                            \
            *(rc) = __rc;                                                                   \
        }                                                                                   \
        (xboolean)(__rc == 0);                                                              \
    }))

#define x_ref_count_compare(rc, val)                                                        \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xrefcount));                                 \
        (void)(0 ? *(rc) ^ (val) : 1);                                                      \
        (xboolean)(*(rc) == -(val));                                                        \
    }))

#define x_atomic_ref_count_init(rc)                                                         \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xatomicrefcount));                           \
        (void)(0 ? *(rc) ^ *(rc) : 1);                                                      \
        *(rc) = 1;                                                                          \
    }))

#define x_atomic_ref_count_inc(rc)                                                          \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xatomicrefcount));                           \
        (void)(0 ? *(rc) ^ *(rc) : 1);                                                      \
        (void)(x_atomic_int_get(rc) == X_MAXINT ? 0 : x_atomic_int_inc((rc)));              \
    }))

#define x_atomic_ref_count_dec(rc)                                                          \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xatomicrefcount));                           \
        (void)(0 ? *(rc) ^ *(rc) : 1);                                                      \
        x_atomic_int_dec_and_test((rc));                                                    \
    }))

#define x_atomic_ref_count_compare(rc, val)                                                 \
    (X_GNUC_EXTENSION ({                                                                    \
        X_STATIC_ASSERT(sizeof *(rc) == sizeof(xatomicrefcount));                           \
        (void)(0 ? *(rc) ^ (val) : 1);                                                      \
        (xboolean)(x_atomic_int_get(rc) == (val));                                          \
    }))

#endif

X_END_DECLS

#endif
