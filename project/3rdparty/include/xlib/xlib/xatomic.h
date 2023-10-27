#ifndef __X_ATOMIC_H__
#define __X_ATOMIC_H__

#include "xtypes.h"
#include "xlib-typeof.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
xint x_atomic_int_get(const volatile xint *atomic);

XLIB_AVAILABLE_IN_ALL
void x_atomic_int_set(volatile xint *atomic, xint newval);

XLIB_AVAILABLE_IN_ALL
void x_atomic_int_inc(volatile xint *atomic);

XLIB_AVAILABLE_IN_ALL
xboolean x_atomic_int_dec_and_test(volatile xint *atomic);

XLIB_AVAILABLE_IN_ALL
xboolean x_atomic_int_compare_and_exchange(volatile xint *atomic, xint oldval, xint newval);

XLIB_AVAILABLE_IN_2_74
xboolean x_atomic_int_compare_and_exchange_full(xint *atomic, xint oldval, xint newval, xint *preval);

XLIB_AVAILABLE_IN_2_74
xint x_atomic_int_exchange(xint *atomic, xint newval);

XLIB_AVAILABLE_IN_ALL
xint x_atomic_int_add(volatile xint *atomic, xint val);

XLIB_AVAILABLE_IN_2_30
xuint x_atomic_int_and(volatile xuint *atomic, xuint val);

XLIB_AVAILABLE_IN_2_30
xuint x_atomic_int_or(volatile xuint *atomic, xuint val);

XLIB_AVAILABLE_IN_ALL
xuint x_atomic_int_xor(volatile xuint *atomic, xuint val);

XLIB_AVAILABLE_IN_ALL
xpointer x_atomic_pointer_get(const volatile void *atomic);

XLIB_AVAILABLE_IN_ALL
void x_atomic_pointer_set(volatile void  *atomic, xpointer newval);

XLIB_AVAILABLE_IN_ALL
xboolean x_atomic_pointer_compare_and_exchange(volatile void  *atomic, xpointer oldval, xpointer newval);

XLIB_AVAILABLE_IN_2_74
xboolean x_atomic_pointer_compare_and_exchange_full(void *atomic, xpointer oldval, xpointer newval, void *preval);

XLIB_AVAILABLE_IN_2_74
xpointer x_atomic_pointer_exchange(void *atomic, xpointer newval);

XLIB_AVAILABLE_IN_ALL
xintptr x_atomic_pointer_add(volatile void *atomic, xssize val);

XLIB_AVAILABLE_IN_2_30
xuintptr x_atomic_pointer_and(volatile void *atomic, xsize val);

XLIB_AVAILABLE_IN_2_30
xuintptr x_atomic_pointer_or(volatile void *atomic, xsize val);

XLIB_AVAILABLE_IN_ALL
xuintptr x_atomic_pointer_xor(volatile void *atomic, xsize val);

XLIB_DEPRECATED_IN_2_30_FOR(x_atomic_int_add)
xint x_atomic_int_exchange_and_add(volatile xint *atomic, xint val);

X_END_DECLS

#if defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

#if defined(__ATOMIC_SEQ_CST)

#define x_atomic_int_get(atomic) \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        xint gaig_temp;                                                                         \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                                                  \
        __atomic_load((xint *)(atomic), &gaig_temp, __ATOMIC_SEQ_CST);                          \
        (xint)gaig_temp;                                                                        \
    }))

#define x_atomic_int_set(atomic, newval) \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        xint gais_temp = (xint)(newval);                                                        \
        (void)(0 ? *(atomic) ^ (newval) : 1);                                                   \
        __atomic_store((xint *)(atomic), &gais_temp, __ATOMIC_SEQ_CST);                         \
    }))

#if defined(xlib_typeof)
#define x_atomic_pointer_get(atomic)                                                            \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        xlib_typeof(*(atomic)) gapg_temp_newval;                                                \
        xlib_typeof((atomic)) gapg_temp_atomic = (atomic);                                      \
        __atomic_load(gapg_temp_atomic, &gapg_temp_newval, __ATOMIC_SEQ_CST);                   \
        gapg_temp_newval;                                                                       \
    }))

#define x_atomic_pointer_set(atomic, newval)                                                    \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        xlib_typeof((atomic)) gaps_temp_atomic = (atomic);                                      \
        xlib_typeof(*(atomic)) gaps_temp_newval = (newval);                                     \
        (void) (0 ? (xpointer) * (atomic) : NULL);                                              \
        __atomic_store(gaps_temp_atomic, &gaps_temp_newval, __ATOMIC_SEQ_CST);                  \
    }))

#else

#define x_atomic_pointer_get(atomic)                                                            \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        xpointer gapg_temp_newval;                                                              \
        xpointer *gapg_temp_atomic = (xpointer *)(atomic);                                      \
        __atomic_load(gapg_temp_atomic, &gapg_temp_newval, __ATOMIC_SEQ_CST);                   \
        gapg_temp_newval;                                                                       \
    }))

#define x_atomic_pointer_set(atomic, newval)                                                    \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        xpointer *gaps_temp_atomic = (xpointer *)(atomic);                                      \
        xpointer gaps_temp_newval = (xpointer)(newval);                                         \
        (void) (0 ? (xpointer) *(atomic) : NULL);                                               \
        __atomic_store(gaps_temp_atomic, &gaps_temp_newval, __ATOMIC_SEQ_CST);                  \
    }))
#endif

#define x_atomic_int_inc(atomic) \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT (sizeof *(atomic) == sizeof(xint));                                     \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                                                  \
        (void)__atomic_fetch_add((atomic), 1, __ATOMIC_SEQ_CST);                                \
    }))

#define x_atomic_int_dec_and_test(atomic) \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                                                  \
        __atomic_fetch_sub((atomic), 1, __ATOMIC_SEQ_CST) == 1;                                 \
    }))

#if defined(xlib_typeof) && defined(X_CXX_STD_VERSION)
#define x_atomic_int_compare_and_exchange(atomic, oldval, newval)                               \
    (X_GNUC_EXTENSION ({                                                                        \
        xlib_typeof(*(atomic)) gaicae_oldval = (oldval);                                        \
        X_STATIC_ASSERT (sizeof *(atomic) == sizeof(xint));                                     \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) : 1);                                        \
        __atomic_compare_exchange_n((atomic), (void *)&gaicae_oldval, (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
    }))

#else

#define x_atomic_int_compare_and_exchange(atomic, oldval, newval)                               \
    (X_GNUC_EXTENSION ({                                                                        \
        xint gaicae_oldval = (oldval);                                                          \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) : 1);                                        \
        __atomic_compare_exchange_n((atomic), (void *)(&(gaicae_oldval)), (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
    }))
#endif

#define x_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval)                  \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        X_STATIC_ASSERT(sizeof *(preval) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) ^ *(preval) : 1);                            \
        *(preval) = (oldval);                                                                   \
        __atomic_compare_exchange_n((atomic), (preval), (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
    }))

#define x_atomic_int_exchange(atomic, newval)                                                   \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) : 1);                                                   \
        (xint)__atomic_exchange_n((atomic), (newval), __ATOMIC_SEQ_CST);                        \
    }))

#define x_atomic_int_add(atomic, val)                                                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xint)__atomic_fetch_add((atomic), (val), __ATOMIC_SEQ_CST);                            \
    }))

#define x_atomic_int_and(atomic, val)                                                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xuint)__atomic_fetch_and((atomic), (val), __ATOMIC_SEQ_CST);                           \
    }))

#define x_atomic_int_or(atomic, val)                                                            \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xuint)__atomic_fetch_or((atomic), (val), __ATOMIC_SEQ_CST);                            \
    }))

#define x_atomic_int_xor(atomic, val)                                                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT (sizeof *(atomic) == sizeof(xint));                                     \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xuint)__atomic_fetch_xor((atomic), (val), __ATOMIC_SEQ_CST);                           \
    }))

#if defined(xlib_typeof) && defined(X_CXX_STD_VERSION)
#define x_atomic_pointer_compare_and_exchange(atomic, oldval, newval)                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof (oldval) == sizeof(xpointer));                                   \
        xlib_typeof(*(atomic)) gapcae_oldval = (oldval);                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        __atomic_compare_exchange_n((atomic), &gapcae_oldval, (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
    }))

#else

#define x_atomic_pointer_compare_and_exchange(atomic, oldval, newval)                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof (oldval) == sizeof(xpointer));                                   \
        xpointer gapcae_oldval = (xpointer)(oldval);                                            \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        __atomic_compare_exchange_n((atomic), (void *)(&(gapcae_oldval)), (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
    }))
#endif

#define x_atomic_pointer_compare_and_exchange_full(atomic, oldval, newval, preval)              \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        X_STATIC_ASSERT(sizeof *(preval) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (xpointer) *(preval) : NULL);                                                \
        *(preval) = (oldval);                                                                   \
        __atomic_compare_exchange_n((atomic), (preval), (newval), FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE; \
    }))

#define x_atomic_pointer_exchange(atomic, newval)                                               \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (xpointer)__atomic_exchange_n((atomic), (newval), __ATOMIC_SEQ_CST);                    \
    }))

#define x_atomic_pointer_add(atomic, val)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xintptr)__atomic_fetch_add((atomic), (val), __ATOMIC_SEQ_CST);                         \
    }))

#define x_atomic_pointer_and(atomic, val)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        xuintptr *gapa_atomic = (xuintptr *)(atomic);                                           \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xuintptr));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xuintptr)__atomic_fetch_and(gapa_atomic, (val), __ATOMIC_SEQ_CST);                     \
    }))

#define x_atomic_pointer_or(atomic, val)                                                        \
    (X_GNUC_EXTENSION ({                                                                        \
        xuintptr *gapo_atomic = (xuintptr *)(atomic);                                           \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xuintptr));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xuintptr)__atomic_fetch_or(gapo_atomic, (val), __ATOMIC_SEQ_CST);                      \
    }))

#define x_atomic_pointer_xor(atomic, val)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        xuintptr *gapx_atomic = (xuintptr *)(atomic);                                           \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xuintptr));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xuintptr)__atomic_fetch_xor(gapx_atomic, (val), __ATOMIC_SEQ_CST);                     \
    }))

#else

#define x_atomic_int_get(atomic)                                                                \
    (X_GNUC_EXTENSION ({                                                                        \
        xint gaig_result;                                                                       \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                                                  \
        gaig_result = (xint) *(atomic);                                                         \
        __sync_synchronize();                                                                   \
        __asm__ __volatile__ ("" : : : "memory");                                               \
        gaig_result;                                                                            \
    }))

#define x_atomic_int_set(atomic, newval)                                                        \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) : 1);                                                   \
        __sync_synchronize();                                                                   \
        __asm__ __volatile__ ("" : : : "memory");                                               \
        *(atomic) = (newval);                                                                   \
    }))

#define x_atomic_pointer_get(atomic)                                                            \
    (X_GNUC_EXTENSION ({                                                                        \
        xpointer gapg_result;                                                                   \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        gapg_result = (xpointer) *(atomic);                                                     \
        __sync_synchronize();                                                                   \
        __asm__ __volatile__ ("" : : : "memory");                                               \
        gapg_result;                                                                            \
    }))

#if defined(xlib_typeof)
#define x_atomic_pointer_set(atomic, newval)                                                    \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        __sync_synchronize();                                                                   \
        __asm__ __volatile__ ("" : : : "memory");                                               \
        *(atomic) = (xlib_typeof(*(atomic)))(xuintptr)(newval);                                 \
    }))

#else

#define x_atomic_pointer_set(atomic, newval)                                                    \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        __sync_synchronize();                                                                   \
        __asm__ __volatile__ ("" : : : "memory");                                               \
        *(atomic) = (xpointer)(xuintptr)(newval);                                               \
    }))
#endif

#define x_atomic_int_inc(atomic)                                                                \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                                                  \
        (void)__sync_fetch_and_add((atomic), 1);                                                \
    }))

#define x_atomic_int_dec_and_test(atomic)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                                                  \
        __sync_fetch_and_sub((atomic), 1) == 1;                                                 \
    }))

#define x_atomic_int_compare_and_exchange(atomic, oldval, newval)                               \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) : 1);                                        \
        __sync_bool_compare_and_swap((atomic), (oldval), (newval)) ? TRUE : FALSE;              \
    }))

#define x_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval)                  \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        X_STATIC_ASSERT(sizeof *(preval) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) ^ *(preval) : 1);                            \
        *(preval) = __sync_val_compare_and_swap((atomic), (oldval), (newval));                  \
        (*(preval) == (oldval)) ? TRUE : FALSE;                                                 \
    }))

#if defined(_XLIB_GCC_HAVE_SYNC_SWAP)
#define x_atomic_int_exchange(atomic, newval)                                                   \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) : 1);                                                   \
        (xint)__sync_swap((atomic), (newval));                                                  \
    }))

#else

#define x_atomic_int_exchange(atomic, newval)                                                   \
    (X_GNUC_EXTENSION ({                                                                        \
        xint oldval;                                                                            \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (newval) : 1);                                                   \
        do {                                                                                    \
            oldval = *atomic;                                                                   \
        } while (!__sync_bool_compare_and_swap(atomic, oldval, newval));                        \
        oldval;                                                                                 \
    }))
#endif

#define x_atomic_int_add(atomic, val)                                                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xint)__sync_fetch_and_add((atomic), (val));                                            \
    }))

#define x_atomic_int_and(atomic, val)                                                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xuint)__sync_fetch_and_and((atomic), (val));                                           \
    }))

#define x_atomic_int_or(atomic, val)                                                            \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xuint)__sync_fetch_and_or((atomic), (val));                                            \
    }))

#define x_atomic_int_xor(atomic, val)                                                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xint));                                      \
        (void)(0 ? *(atomic) ^ (val) : 1);                                                      \
        (xuint)__sync_fetch_and_xor((atomic), (val));                                           \
    }))

#define x_atomic_pointer_compare_and_exchange(atomic, oldval, newval)                           \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        __sync_bool_compare_and_swap((atomic), (oldval), (newval)) ? TRUE : FALSE;              \
    }))

#define x_atomic_pointer_compare_and_exchange_full(atomic, oldval, newval, preval)              \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        X_STATIC_ASSERT(sizeof *(preval) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (xpointer) *(preval) : NULL);                                                \
        *(preval) = __sync_val_compare_and_swap((atomic), (oldval), (newval));                  \
        (*(preval) == (oldval)) ? TRUE : FALSE;                                                 \
    }))

#if defined(_XLIB_GCC_HAVE_SYNC_SWAP)
#define x_atomic_pointer_exchange(atomic, newval)                                               \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (xpointer)__sync_swap((atomic), (newval));                                              \
    }))

#else

#define x_atomic_pointer_exchange(atomic, newval)                                               \
    (X_GNUC_EXTENSION ({                                                                        \
        xpointer oldval;                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        do {                                                                                    \
            oldval = (xpointer) *atomic;                                                        \
        } while (!__sync_bool_compare_and_swap(atomic, oldval, newval));                        \
        oldval;                                                                                 \
    }))
#endif

#define x_atomic_pointer_add(atomic, val)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xintptr)__sync_fetch_and_add((atomic), (val));                                         \
    }))

#define x_atomic_pointer_and(atomic, val)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xuintptr)__sync_fetch_and_and((atomic), (val));                                        \
    }))

#define x_atomic_pointer_or(atomic, val)                                                        \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xuintptr)__sync_fetch_and_or((atomic), (val));                                         \
    }))

#define x_atomic_pointer_xor(atomic, val)                                                       \
    (X_GNUC_EXTENSION ({                                                                        \
        X_STATIC_ASSERT(sizeof *(atomic) == sizeof(xpointer));                                  \
        (void)(0 ? (xpointer) *(atomic) : NULL);                                                \
        (void)(0 ? (val) ^ (val) : 1);                                                          \
        (xuintptr)__sync_fetch_and_xor((atomic), (val));                                        \
    }))
#endif

#else

#define x_atomic_int_get(atomic)                                                                \
    (x_atomic_int_get((xint *)(atomic)))

#define x_atomic_int_set(atomic, newval)                                                        \
    (x_atomic_int_set((xint *)(atomic), (xint)(newval)))

#define x_atomic_int_compare_and_exchange(atomic, oldval, newval)                               \
    (x_atomic_int_compare_and_exchange((xint *)(atomic), (oldval), (newval)))

#define x_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval)                  \
    (x_atomic_int_compare_and_exchange_full((xint *)(atomic), (oldval), (newval), (xint *)(preval)))

#define x_atomic_int_exchange(atomic, newval)                                                   \
    (x_atomic_int_exchange((xint *)(atomic), (newval)))

#define x_atomic_int_add(atomic, val)                                                           \
    (x_atomic_int_add ((xint *)(atomic), (val)))

#define x_atomic_int_and(atomic, val)                                                           \
    (x_atomic_int_and((xuint *) (atomic), (val)))

#define x_atomic_int_or(atomic, val)                                                            \
    (x_atomic_int_or((xuint *)(atomic), (val)))

#define x_atomic_int_xor(atomic, val) \
    (x_atomic_int_xor((xuint *)(atomic), (val)))

#define x_atomic_int_inc(atomic)                                                                \
    (x_atomic_int_inc((xint *)(atomic)))

#define x_atomic_int_dec_and_test(atomic)                                                       \
    (x_atomic_int_dec_and_test((xint *)(atomic)))

#if defined(xlib_typeof)
#define x_atomic_pointer_get(atomic)                                                            \
    (xlib_typeof (*(atomic)))(void *)((x_atomic_pointer_get)((void *)atomic))
#else
#define x_atomic_pointer_get(atomic)                                                            \
    (x_atomic_pointer_get(atomic))
#endif

#define x_atomic_pointer_set(atomic, newval)                                                    \
  (x_atomic_pointer_set((atomic), (xpointer)(newval)))

#define x_atomic_pointer_compare_and_exchange(atomic, oldval, newval)                           \
    (x_atomic_pointer_compare_and_exchange((atomic), (xpointer)(oldval), (xpointer)(newval)))

#define x_atomic_pointer_compare_and_exchange_full(atomic, oldval, newval, prevval)             \
    (x_atomic_pointer_compare_and_exchange_full((atomic), (xpointer)(oldval), (xpointer)(newval), (prevval)))

#define x_atomic_pointer_exchange(atomic, newval)                                               \
    (x_atomic_pointer_exchange((atomic), (xpointer)(newval)))

#define x_atomic_pointer_add(atomic, val)                                                       \
    (x_atomic_pointer_add((atomic), (xssize)(val)))

#define x_atomic_pointer_and(atomic, val)                                                       \
    (x_atomic_pointer_and((atomic), (xsize)(val)))

#define x_atomic_pointer_or(atomic, val)                                                        \
  (x_atomic_pointer_or((atomic), (xsize)(val)))

#define x_atomic_pointer_xor(atomic, val)                                                       \
  (x_atomic_pointer_xor((atomic), (xsize)(val)))

#endif

#endif
