#include <xlib/xlib/config.h>
#include <xlib/xlib/xslist.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xmacros.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xbitlock.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xthreadprivate.h>

#ifdef X_BIT_LOCK_FORCE_FUTEX_EMULATION
#undef HAVE_FUTEX
#undef HAVE_FUTEX_TIME64
#endif

#define CONTENTION_CLASSES  11

#if (defined (i386) || defined(__amd64__))
#if X_GNUC_CHECK_VERSION(4, 5)
#define USE_ASM_GOTO        1
#endif
#endif

static xint x_bit_lock_contended[CONTENTION_CLASSES];

X_ALWAYS_INLINE static inline xuint bit_lock_contended_class(xpointer address)
{
    return ((xsize)address) % X_N_ELEMENTS(x_bit_lock_contended);
}

static void x_futex_wait(const xint *address, xint value)
{
    x_futex_simple(address, (xsize)FUTEX_WAIT_PRIVATE, (xsize)value, NULL);
}

static void x_futex_wake(const xint *address)
{
    x_futex_simple(address, (xsize)FUTEX_WAKE_PRIVATE, (xsize)1, NULL);
}

void x_bit_lock(volatile xint *address, xint lock_bit)
{
    xint *address_nonvolatile = (xint *)address;

#ifdef USE_ASM_GOTO
retry:
    __asm__ volatile goto ("lock bts %1, (%0)\n"
                            "jc %l[contended]"
                            :
                            : "r" (address), "r" (lock_bit)
                            : "cc", "memory"
                            : contended);
    return;

contended:
    {
        xuint v;
        xuint mask = 1u << lock_bit;

        v = (xuint)x_atomic_int_get(address_nonvolatile);
        if (v & mask) {
            xuint classt = bit_lock_contended_class(address_nonvolatile);

            x_atomic_int_add(&x_bit_lock_contended[classt], +1);
            x_futex_wait(address_nonvolatile, v);
            x_atomic_int_add(&x_bit_lock_contended[classt], -1);
        }
    }

    goto retry;
#else
    xuint v;
    xuint mask = 1u << lock_bit;

retry:
    v = x_atomic_int_or(address_nonvolatile, mask);
    if (v & mask) {
        xuint classt = bit_lock_contended_class(address_nonvolatile);

        x_atomic_int_add(&x_bit_lock_contended[classt], +1);
        x_futex_wait(address_nonvolatile, v);
        x_atomic_int_add(&x_bit_lock_contended[classt], -1);

        goto retry;
    }
#endif
}

xboolean x_bit_trylock(volatile xint *address, xint lock_bit)
{
#ifdef USE_ASM_GOTO
    xboolean result;

    __asm__ volatile ("lock bts %2, (%1)\n"
                        "setnc %%al\n"
                        "movzx %%al, %0"
                        : "=r" (result)
                        : "r" (address), "r" (lock_bit)
                        : "cc", "memory");

    return result;
#else
    xint *address_nonvolatile = (xint *)address;
    xuint mask = 1u << lock_bit;
    xuint v;

    v = x_atomic_int_or(address_nonvolatile, mask);

    return ~v & mask;
#endif
}

void x_bit_unlock(volatile xint *address, xint lock_bit)
{
    xint *address_nonvolatile = (xint *)address;

#ifdef USE_ASM_GOTO
    __asm__ volatile ("lock btr %1, (%0)"
                        :
                        : "r" (address), "r" (lock_bit)
                        : "cc", "memory");
#else
    xuint mask = 1u << lock_bit;
    x_atomic_int_and(address_nonvolatile, ~mask);
#endif

    {
        xuint classt = bit_lock_contended_class(address_nonvolatile);

        if (x_atomic_int_get(&x_bit_lock_contended[classt])) {
            x_futex_wake(address_nonvolatile);
        }
    }
}

static const xint *x_futex_int_address(const void *address)
{
    const xint *int_address = (const xint *)address;

    X_STATIC_ASSERT(X_BYTE_ORDER == X_LITTLE_ENDIAN || (X_BYTE_ORDER == X_BIG_ENDIAN && sizeof (int) == 4 && (sizeof(xpointer) == 4 || sizeof(xpointer) == 8)));

#if X_BYTE_ORDER == X_BIG_ENDIAN && XLIB_SIZEOF_VOID_P == 8
    int_address++;
#endif

    return int_address;
}

X_ALWAYS_INLINE static inline xpointer pointer_bit_lock_mask_ptr(xpointer ptr, xuint lock_bit, xboolean set, xuintptr preserve_mask, xpointer preserve_ptr)
{
    xuintptr x_ptr;
    xuintptr lock_mask;
    xuintptr x_preserve_ptr;

    x_ptr = (xuintptr)ptr;

    if (preserve_mask != 0)  {
        x_preserve_ptr = (xuintptr)preserve_ptr;
        x_ptr = (x_preserve_ptr & preserve_mask) | (x_ptr & ~preserve_mask);
    }

    if (lock_bit == X_MAXUINT) {
        return (xpointer)x_ptr;
    }

    lock_mask = (xuintptr)(1u << lock_bit);
    if (set) {
        return (xpointer)(x_ptr | lock_mask);
    } else {
        return (xpointer)(x_ptr & ~lock_mask);
    }
}

void (x_pointer_bit_lock_and_get)(xpointer address, xuint lock_bit, xuintptr *out_ptr)
{
    xuintptr v;
    xuintptr mask;
    xuint classt = bit_lock_contended_class(address);

    x_return_if_fail(lock_bit < 32);

    mask = 1u << lock_bit;

#ifdef USE_ASM_GOTO
    if (X_LIKELY(!out_ptr)) {
        while (TRUE) {
            __asm__ volatile goto ("lock bts %1, (%0)\n"
                                 "jc %l[contended]"
                                 : /* no output */
                                 : "r"(address), "r"((xsize)lock_bit)
                                 : "cc", "memory"
                                 : contended);
            return;

contended:
            v = (xuintptr)x_atomic_pointer_get((xpointer *)address);
            if (v & mask) {
                x_atomic_int_add(&x_bit_lock_contended[classt], +1);
                x_futex_wait(x_futex_int_address(address), v);
                x_atomic_int_add(&x_bit_lock_contended[classt], -1);
            }
        }
    }
#endif

retry:
    v = x_atomic_pointer_or((xpointer *)address, mask);
    if (v & mask) {
        x_atomic_int_add(&x_bit_lock_contended[classt], +1);
        x_futex_wait(x_futex_int_address(address), (xuint)v);
        x_atomic_int_add(&x_bit_lock_contended[classt], -1);

        goto retry;
    }

    if (out_ptr) {
        *out_ptr = (v | mask);
    }
}

void (x_pointer_bit_lock)(volatile void *address, xint lock_bit)
{
    x_pointer_bit_lock_and_get((xpointer *)address, (xuint)lock_bit, NULL);
}

xboolean (x_pointer_bit_trylock)(volatile void *address, xint lock_bit)
{
    x_return_val_if_fail(lock_bit < 32, FALSE);

    {
#ifdef USE_ASM_GOTO
        xboolean result;

        __asm__ volatile ("lock bts %2, (%1)\n"
                        "setnc %%al\n"
                        "movzx %%al, %0"
                        : "=r" (result)
                        : "r" (address), "r" ((xsize) lock_bit)
                        : "cc", "memory");

        return result;
#else
        xuintptr v;
        xsize mask = 1u << lock_bit;
        void *address_nonvolatile = (void *)address;
        xpointer *pointer_address = address_nonvolatile;

        x_return_val_if_fail(lock_bit < 32, FALSE);
        v = x_atomic_pointer_or(pointer_address, mask);

        return (~(xsize) v & mask) != 0;
#endif
    }
}

void (x_pointer_bit_unlock)(volatile void *address, xint lock_bit)
{
    void *address_nonvolatile = (void *)address;

    x_return_if_fail(lock_bit < 32);

    {
#ifdef USE_ASM_GOTO
        __asm__ volatile ("lock btr %1, (%0)"
                        :
                        : "r" (address), "r" ((xsize) lock_bit)
                        : "cc", "memory");
#else
        xpointer *pointer_address = address_nonvolatile;
        xsize mask = 1u << lock_bit;

        x_atomic_pointer_and(pointer_address, ~mask);
#endif
        {
            xuint classt = bit_lock_contended_class (address_nonvolatile);
            if (x_atomic_int_get(&x_bit_lock_contended[classt])) {
                x_futex_wake(x_futex_int_address (address_nonvolatile));
            }
        }
    }
}

xpointer x_pointer_bit_lock_mask_ptr(xpointer ptr, xuint lock_bit, xboolean set, xuintptr preserve_mask, xpointer preserve_ptr)
{
    x_return_val_if_fail(lock_bit < 32u || lock_bit == X_MAXUINT, ptr);
    return pointer_bit_lock_mask_ptr(ptr, lock_bit, set, preserve_mask, preserve_ptr);
}

void (x_pointer_bit_unlock_and_set)(void *address, xuint lock_bit, xpointer ptr, xuintptr preserve_mask)
{
    xpointer ptr2;
    xpointer *pointer_address = address;
    xuint classt = bit_lock_contended_class(address);

    x_return_if_fail(lock_bit < 32u);

    if (preserve_mask != 0) {
        xpointer old_ptr = x_atomic_pointer_get((xpointer *)address);

again:
        ptr2 = pointer_bit_lock_mask_ptr(ptr, lock_bit, FALSE, preserve_mask, old_ptr);
        if (!x_atomic_pointer_compare_and_exchange_full(pointer_address, old_ptr, ptr2, &old_ptr)) {
            goto again;
        }
    } else {
        ptr2 = pointer_bit_lock_mask_ptr(ptr, lock_bit, FALSE, 0, NULL);
        x_atomic_pointer_set(pointer_address, ptr2);
    }

    if (x_atomic_int_get(&x_bit_lock_contended[classt]) > 0) {
        x_futex_wake(x_futex_int_address(address));
    }

    x_return_if_fail(ptr == pointer_bit_lock_mask_ptr(ptr, lock_bit, FALSE, 0, NULL));
}
