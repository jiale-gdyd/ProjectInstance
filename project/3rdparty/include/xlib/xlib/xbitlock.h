#ifndef __X_BITLOCK_H__
#define __X_BITLOCK_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
void x_bit_lock(volatile xint *address, xint lock_bit);

XLIB_AVAILABLE_IN_ALL
xboolean x_bit_trylock(volatile xint *address, xint lock_bit);

XLIB_AVAILABLE_IN_ALL
void x_bit_unlock(volatile xint *address, xint lock_bit);

XLIB_AVAILABLE_IN_ALL
void x_pointer_bit_lock(volatile void *address, xint lock_bit);

XLIB_AVAILABLE_IN_2_80
void x_pointer_bit_lock_and_get(xpointer address, xuint lock_bit, xuintptr *out_ptr);

XLIB_AVAILABLE_IN_ALL
xboolean x_pointer_bit_trylock(volatile void *address, xint lock_bit);

XLIB_AVAILABLE_IN_ALL
void x_pointer_bit_unlock(volatile void *address, xint lock_bit);

XLIB_AVAILABLE_IN_2_80
xpointer x_pointer_bit_lock_mask_ptr(xpointer ptr, xuint lock_bit, xboolean set, xuintptr preserve_mask, xpointer preserve_ptr);

XLIB_AVAILABLE_IN_2_80
void x_pointer_bit_unlock_and_set(void *address, xuint lock_bit, xpointer ptr, xuintptr preserve_mask);

#define x_pointer_bit_lock(address, lock_bit)                                   \
    (X_GNUC_EXTENSION ({                                                        \
        X_STATIC_ASSERT(sizeof *(address) == sizeof(xpointer));                 \
        x_pointer_bit_lock((address), (lock_bit));                              \
    }))

#define x_pointer_bit_lock_and_get(address, lock_bit, out_ptr)                  \
    (X_GNUC_EXTENSION ({                                                        \
        X_STATIC_ASSERT(sizeof *(address) == sizeof(xpointer));                 \
        x_pointer_bit_lock_and_get((address), (lock_bit), (out_ptr));           \
    }))

#define x_pointer_bit_trylock(address, lock_bit)                                \
    (X_GNUC_EXTENSION ({                                                        \
        X_STATIC_ASSERT(sizeof *(address) == sizeof(xpointer));                 \
        x_pointer_bit_trylock((address), (lock_bit));                           \
    }))

#define x_pointer_bit_unlock(address, lock_bit)                                 \
    (X_GNUC_EXTENSION ({                                                        \
        X_STATIC_ASSERT(sizeof *(address) == sizeof(xpointer));                 \
        x_pointer_bit_unlock((address), (lock_bit));                            \
    }))

#define x_pointer_bit_unlock_and_set(address, lock_bit, ptr, preserve_mask)          \
    (X_GNUC_EXTENSION ({                                                             \
        X_STATIC_ASSERT(sizeof *(address) == sizeof(xpointer));                      \
        x_pointer_bit_unlock_and_set((address), (lock_bit), (ptr), (preserve_mask)); \
    }))

X_END_DECLS

#endif
