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
            xuint classt = ((xsize)address_nonvolatile) % X_N_ELEMENTS (x_bit_lock_contended);

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
        xuint classt = ((xsize)address_nonvolatile) % X_N_ELEMENTS (x_bit_lock_contended);

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
        xuint classt = ((xsize)address_nonvolatile) % X_N_ELEMENTS(x_bit_lock_contended);

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

void (x_pointer_bit_lock)(volatile void *address, xint lock_bit)
{
    void *address_nonvolatile = (void *)address;
    x_return_if_fail(lock_bit < 32);

    {
#ifdef USE_ASM_GOTO
retry:
        __asm__ volatile goto ("lock bts %1, (%0)\n"
                            "jc %l[contended]"
                            :
                            : "r" (address), "r" ((xsize) lock_bit)
                            : "cc", "memory"
                            : contended);
        return;

contended:
        {
            xsize v;
            xsize mask = 1u << lock_bit;
            xsize *pointer_address = (xsize *)address_nonvolatile;

            v = (xsize)x_atomic_pointer_get(pointer_address);
            if (v & mask) {
                xuint classt = ((xsize)address_nonvolatile) % X_N_ELEMENTS(x_bit_lock_contended);

                x_atomic_int_add(&x_bit_lock_contended[classt], +1);
                x_futex_wait(x_futex_int_address(address_nonvolatile), v);
                x_atomic_int_add(&x_bit_lock_contended[classt], -1);
            }
        }

        goto retry;
#else
        xsize v;
        xsize mask = 1u << lock_bit;
        xsize *pointer_address = address_nonvolatile;

retry:
        v = x_atomic_pointer_or(pointer_address, mask);
        if (v & mask) {
            xuint classt = ((xsize)address_nonvolatile) % X_N_ELEMENTS(x_bit_lock_contended);

            x_atomic_int_add(&x_bit_lock_contended[classt], +1);
            x_futex_wait(x_futex_int_address(address_nonvolatile), (xuint)v);
            x_atomic_int_add(&x_bit_lock_contended[classt], -1);

            goto retry;
        }
#endif
    }
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
        xsize v;
        xsize mask = 1u << lock_bit;
        void *address_nonvolatile = (void *)address;
        xsize *pointer_address = address_nonvolatile;

        x_return_val_if_fail(lock_bit < 32, FALSE);
        v = x_atomic_pointer_or(pointer_address, mask);

        return ~v & mask;
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
        xsize *pointer_address = address_nonvolatile;
        xsize mask = 1u << lock_bit;

        x_atomic_pointer_and(pointer_address, ~mask);
#endif
        {
            xuint classt = ((xsize) address_nonvolatile) % X_N_ELEMENTS(x_bit_lock_contended);
            if (x_atomic_int_get(&x_bit_lock_contended[classt])) {
                x_futex_wake(x_futex_int_address (address_nonvolatile));
            }
        }
    }
}
