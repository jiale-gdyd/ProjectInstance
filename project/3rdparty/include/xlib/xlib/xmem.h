#ifndef __X_MEM_H__
#define __X_MEM_H__

#include "xutils.h"
#include "xlib-typeof.h"

X_BEGIN_DECLS

typedef struct _XMemVTable XMemVTable;

#if XLIB_SIZEOF_VOID_P > XLIB_SIZEOF_LONG
#define X_MEM_ALIGN                     XLIB_SIZEOF_VOID_P
#else
#define X_MEM_ALIGN                     XLIB_SIZEOF_LONG
#endif

XLIB_AVAILABLE_IN_ALL
void x_free(xpointer mem);

XLIB_AVAILABLE_IN_2_76
void x_free_sized(xpointer mem, size_t size);

XLIB_AVAILABLE_IN_2_34
void x_clear_pointer(xpointer *pp, XDestroyNotify destroy);

XLIB_AVAILABLE_IN_ALL
xpointer x_malloc(xsize n_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
xpointer x_malloc0(xsize n_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
xpointer x_realloc(xpointer mem, xsize n_bytes) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
xpointer x_try_malloc(xsize n_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
xpointer x_try_malloc0(xsize n_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
xpointer x_try_realloc(xpointer mem, xsize n_bytes) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
xpointer x_malloc_n(xsize n_blocks, xsize n_block_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE2(1, 2);

XLIB_AVAILABLE_IN_ALL
xpointer x_malloc0_n(xsize n_blocks, xsize n_block_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE2(1, 2);

XLIB_AVAILABLE_IN_ALL
xpointer x_realloc_n(xpointer mem, xsize n_blocks, xsize n_block_bytes) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
xpointer x_try_malloc_n(xsize n_blocks, xsize n_block_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE2(1, 2);

XLIB_AVAILABLE_IN_ALL
xpointer x_try_malloc0_n(xsize n_blocks, xsize n_block_bytes) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE2(1, 2);

XLIB_AVAILABLE_IN_ALL
xpointer x_try_realloc_n(xpointer mem, xsize n_blocks, xsize n_block_bytes) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_72
xpointer x_aligned_alloc(xsize n_blocks, xsize n_block_bytes, xsize alignment) X_GNUC_WARN_UNUSED_RESULT X_GNUC_ALLOC_SIZE2(1, 2);

XLIB_AVAILABLE_IN_2_72
xpointer x_aligned_alloc0 (xsize n_blocks, xsize n_block_bytes, xsize alignment) X_GNUC_WARN_UNUSED_RESULT X_GNUC_ALLOC_SIZE2(1, 2);

XLIB_AVAILABLE_IN_2_72
void x_aligned_free(xpointer mem);

XLIB_AVAILABLE_IN_2_76
void x_aligned_free_sized(xpointer mem, size_t alignment, size_t size);

#if defined(xlib_typeof) && XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_58
#define x_clear_pointer(pp, destroy)                                                        \
    X_STMT_START {                                                                          \
        X_STATIC_ASSERT(sizeof *(pp) == sizeof(xpointer));                                  \
        xlib_typeof((pp)) _pp = (pp);                                                       \
        xlib_typeof(*(pp)) _ptr = *_pp;                                                     \
        *_pp = NULL;                                                                        \
        if (_ptr) {                                                                         \
            (destroy)(_ptr);                                                                \
        }                                                                                   \
    } X_STMT_END                                                                            \
    XLIB_AVAILABLE_MACRO_IN_2_34

#else

#define x_clear_pointer(pp, destroy)                                                        \
    X_STMT_START {                                                                          \
        X_STATIC_ASSERT(sizeof *(pp) == sizeof(xpointer));                                  \
        union { char *in; xpointer *out; } _pp;                                             \
        xpointer _p;                                                                        \
        XDestroyNotify _destroy = (XDestroyNotify)(destroy);                                \
                                                                                            \
        _pp.in = (char *)(pp);                                                              \
        _p = *_pp.out;                                                                      \
        if (_p) {                                                                           \
            *_pp.out = NULL;                                                                \
            _destroy (_p);                                                                  \
        }                                                                                   \
    } X_STMT_END                                                                            \
    XLIB_AVAILABLE_MACRO_IN_2_34
#endif

XLIB_AVAILABLE_STATIC_INLINE_IN_2_44
static inline xpointer x_steal_pointer(xpointer pp)
{
    xpointer ref;
    xpointer *ptr = (xpointer *)pp;

    ref = *ptr;
    *ptr = NULL;

    return ref;
}

#if defined(xlib_typeof) && XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_58
#define x_steal_pointer(pp)             ((xlib_typeof(*pp))(x_steal_pointer)(pp))
#else
#define x_steal_pointer(pp)             (0 ? (*(pp)) : (x_steal_pointer)(pp))
#endif

#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#define _X_NEW(struct_type, n_structs, func)                                                \
    (struct_type *)(X_GNUC_EXTENSION ({                                                     \
        xsize __n = (xsize)(n_structs);                                                     \
        xsize __s = sizeof(struct_type);                                                    \
        xpointer __p;                                                                       \
        if (__s == 1) {                                                                     \
            __p = x_##func (__n);                                                           \
        } else if (__builtin_constant_p(__n) && (__s == 0 || __n <= X_MAXSIZE / __s)) {     \
            __p = x_##func(__n * __s);                                                      \
        } else {                                                                            \
            __p = x_##func##_n(__n, __s);                                                   \
        }                                                                                   \
        __p;                                                                                \
    }))

#define _X_RENEW(struct_type, mem, n_structs, func)                                         \
    (struct_type *)(X_GNUC_EXTENSION ({                                                     \
        xsize __n = (xsize)(n_structs);                                                     \
        xsize __s = sizeof(struct_type);                                                    \
        xpointer __p = (xpointer)(mem);                                                     \
        if (__s == 1) {                                                                     \
            __p = x_##func(__p, __n);                                                       \
        } else if (__builtin_constant_p(__n) && (__s == 0 || __n <= X_MAXSIZE / __s)) {     \
            __p = x_##func(__p, __n * __s);                                                 \
        } else {                                                                            \
            __p = x_##func##_n(__p, __n, __s);                                              \
        }                                                                                   \
        __p;                                                                                \
    }))

#else

#define _X_NEW(struct_type, n_structs, func)                    ((struct_type *)x_##func##_n ((n_structs), sizeof(struct_type)))
#define _X_RENEW(struct_type, mem, n_structs, func)             ((struct_type *)x_##func##_n (mem, (n_structs), sizeof(struct_type)))
#endif

#define x_new(struct_type, n_structs)                           _X_NEW(struct_type, n_structs, malloc)
#define x_new0(struct_type, n_structs)                          _X_NEW(struct_type, n_structs, malloc0)

#define x_renew(struct_type, mem, n_structs)                    _X_RENEW(struct_type, mem, n_structs, realloc)
#define x_try_new(struct_type, n_structs)                       _X_NEW(struct_type, n_structs, try_malloc)

#define x_try_new0(struct_type, n_structs)                      _X_NEW(struct_type, n_structs, try_malloc0)
#define x_try_renew(struct_type, mem, n_structs)                _X_RENEW(struct_type, mem, n_structs, try_realloc)

struct _XMemVTable {
    xpointer (*malloc)(xsize n_bytes);
    xpointer (*realloc)(xpointer mem, xsize n_bytes);
    void (*free)(xpointer mem);
    xpointer (*calloc)(xsize n_blocks, xsize n_block_bytes);
    xpointer (*try_malloc)(xsize n_bytes);
    xpointer (*try_realloc)(xpointer mem, xsize n_bytes);
};

XLIB_DEPRECATED_IN_2_46
void x_mem_set_vtable(XMemVTable *vtable);

XLIB_DEPRECATED_IN_2_46
xboolean x_mem_is_system_malloc(void);

XLIB_VAR xboolean x_mem_gc_friendly;
XLIB_VAR XMemVTable *xlib_mem_profiler_table;

XLIB_DEPRECATED_IN_2_46
void x_mem_profile(void);

X_END_DECLS

#endif
