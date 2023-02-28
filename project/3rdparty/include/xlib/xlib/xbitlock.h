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

XLIB_AVAILABLE_IN_ALL
xboolean x_pointer_bit_trylock(volatile void *address, xint lock_bit);

XLIB_AVAILABLE_IN_ALL
void x_pointer_bit_unlock(volatile void *address, xint lock_bit);

#define x_pointer_bit_lock(address, lock_bit)                                   \
    (X_GNUC_EXTENSION ({                                                        \
        X_STATIC_ASSERT(sizeof *(address) == sizeof(xpointer));                 \
        x_pointer_bit_lock((address), (lock_bit));                              \
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

X_END_DECLS

#endif
