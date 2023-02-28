#ifndef __X_SLICE_H__
#define __X_SLICE_H__

#include <string.h>
#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
xpointer x_slice_alloc(xsize block_size) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
xpointer x_slice_alloc0(xsize block_size) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
xpointer x_slice_copy(xsize block_size, xconstpointer mem_block) X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_ALL
void x_slice_free1(xsize block_size, xpointer mem_block);

XLIB_AVAILABLE_IN_ALL
void x_slice_free_chain_with_offset(xsize block_size, xpointer mem_chain, xsize next_offset);

#define x_slice_new(type)                       ((type *)x_slice_alloc(sizeof(type)))

#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#define x_slice_new0(type)                                              \
    (type *)(X_GNUC_EXTENSION ({                                        \
        xsize __s = sizeof(type);                                       \
        xpointer __p;                                                   \
        __p = x_slice_alloc(__s);                                       \
        memset(__p, 0, __s);                                            \
        __p;                                                            \
    }))
#else
#define x_slice_new0(type)                      ((type *)x_slice_alloc0(sizeof(type)))
#endif

#define x_slice_dup(type, mem)                  (1 ? (type *)x_slice_copy(sizeof (type), (mem)) : ((void) ((type *)0 == (mem)), (type *)0))

#define x_slice_free(type, mem)                                         \
    X_STMT_START {                                                      \
        if (1) {                                                        \
            x_slice_free1(sizeof(type), (mem));                         \
        } else {                                                        \
            (void)((type *)0 == (mem));                                 \
        }                                                               \
    } X_STMT_END

#define x_slice_free_chain(type, mem_chain, next)                       \
    X_STMT_START {                                                      \
        if (1) {                                                        \
            x_slice_free_chain_with_offset(sizeof(type), (mem_chain), X_STRUCT_OFFSET(type, next)); \
        } else {                                                        \
            (void)((type *)0 == (mem_chain));                           \
        }                                                               \
    } X_STMT_END

typedef enum {
    X_SLICE_CONFIG_ALWAYS_MALLOC = 1,
    X_SLICE_CONFIG_BYPASS_MAGAZINES,
    X_SLICE_CONFIG_WORKING_SET_MSECS,
    X_SLICE_CONFIG_COLOR_INCREMENT,
    X_SLICE_CONFIG_CHUNK_SIZES,
    X_SLICE_CONFIG_CONTENTION_COUNTER
} XSliceConfig;

XLIB_DEPRECATED_IN_2_34
void x_slice_set_config(XSliceConfig ckey, xint64 value);

XLIB_DEPRECATED_IN_2_34
xint64 x_slice_get_config(XSliceConfig ckey);

XLIB_DEPRECATED_IN_2_34
xint64 *x_slice_get_config_state(XSliceConfig ckey, xint64 address, xuint *n_values);

X_END_DECLS

#endif
