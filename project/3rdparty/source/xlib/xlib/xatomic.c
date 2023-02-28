#include <xlib/xlib/config.h>
#include <xlib/xlib/xatomic.h>

#ifdef X_ATOMIC_LOCK_FREE

#if defined (__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

xint (x_atomic_int_get)(const volatile xint *atomic)
{
    return x_atomic_int_get(atomic);
}

void (x_atomic_int_set)(volatile xint *atomic, xint newval)
{
    x_atomic_int_set(atomic, newval);
}

void (x_atomic_int_inc)(volatile xint *atomic)
{
    x_atomic_int_inc(atomic);
}

xboolean (x_atomic_int_dec_and_test)(volatile xint *atomic)
{
    return x_atomic_int_dec_and_test(atomic);
}

xboolean (x_atomic_int_compare_and_exchange)(volatile xint *atomic, xint oldval, xint newval)
{
    return x_atomic_int_compare_and_exchange(atomic, oldval, newval);
}

xboolean (x_atomic_int_compare_and_exchange_full)(xint *atomic, xint oldval, xint newval, xint *preval)
{
    return x_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval);
}

xint (x_atomic_int_exchange)(xint *atomic, xint newval)
{
    return x_atomic_int_exchange(atomic, newval);
}

xint (x_atomic_int_add)(volatile xint *atomic, xint val)
{
    return x_atomic_int_add(atomic, val);
}

xuint (x_atomic_int_and)(volatile xuint *atomic, xuint val)
{
    return x_atomic_int_and(atomic, val);
}

xuint (x_atomic_int_or)(volatile xuint *atomic, xuint val)
{
    return x_atomic_int_or(atomic, val);
}

xuint (x_atomic_int_xor)(volatile xuint *atomic, xuint val)
{
    return x_atomic_int_xor(atomic, val);
}

xpointer (x_atomic_pointer_get)(const volatile void *atomic)
{
    return x_atomic_pointer_get((xpointer *)atomic);
}

void (x_atomic_pointer_set)(volatile void *atomic, xpointer newval)
{
    x_atomic_pointer_set((xpointer *)atomic, newval);
}

xboolean (x_atomic_pointer_compare_and_exchange)(volatile void *atomic, xpointer oldval, xpointer newval)
{
    return x_atomic_pointer_compare_and_exchange((xpointer *)atomic, oldval, newval);
}

xboolean (x_atomic_pointer_compare_and_exchange_full)(void *atomic, xpointer oldval, xpointer newval, void *preval)
{
    return x_atomic_pointer_compare_and_exchange_full((xpointer *)atomic, oldval, newval, (xpointer *)preval);
}

xpointer (x_atomic_pointer_exchange)(void *atomic, xpointer newval)
{
    return x_atomic_pointer_exchange((xpointer *)atomic, newval);
}

xssize (x_atomic_pointer_add)(volatile void *atomic, xssize val)
{
    return x_atomic_pointer_add((xpointer *)atomic, val);
}

xsize (x_atomic_pointer_and)(volatile void *atomic, xsize val)
{
    return x_atomic_pointer_and((xpointer *)atomic, val);
}

xsize (x_atomic_pointer_or)(volatile void *atomic, xsize val)
{
    return x_atomic_pointer_or((xpointer *)atomic, val);
}

xsize (x_atomic_pointer_xor)(volatile void *atomic, xsize val)
{
    return x_atomic_pointer_xor((xpointer *)atomic, val);
}

#else

#error X_ATOMIC_LOCK_FREE defined, but incapable of lock-free atomics.
#endif

#else

#include <pthread.h>

static pthread_mutex_t x_atomic_lock = PTHREAD_MUTEX_INITIALIZER;

xint (x_atomic_int_get)(const volatile xint *atomic)
{
    xint value;

    pthread_mutex_lock(&x_atomic_lock);
    value = *atomic;
    pthread_mutex_unlock(&x_atomic_lock);

    return value;
}

void (x_atomic_int_set)(volatile xint *atomic, xint value)
{
    pthread_mutex_lock(&x_atomic_lock);
    *atomic = value;
    pthread_mutex_unlock(&x_atomic_lock);
}

void (x_atomic_int_inc)(volatile xint *atomic)
{
    pthread_mutex_lock(&x_atomic_lock);
    (*atomic)++;
    pthread_mutex_unlock(&x_atomic_lock);
}

xboolean (x_atomic_int_dec_and_test)(volatile xint *atomic)
{
    xboolean is_zero;

    pthread_mutex_lock(&x_atomic_lock);
    is_zero = --(*atomic) == 0;
    pthread_mutex_unlock(&x_atomic_lock);

    return is_zero;
}

xboolean (x_atomic_int_compare_and_exchange)(volatile xint *atomic, xint oldval, xint newval)
{
    xboolean success;

    pthread_mutex_lock(&x_atomic_lock);
    if ((success = (*atomic == oldval))) {
        *atomic = newval;
    }
    pthread_mutex_unlock(&x_atomic_lock);

    return success;
}

xboolean (x_atomic_int_compare_and_exchange_full)(xint *atomic, xint oldval, xint newval, xint *preval)
{
    xboolean success;

    pthread_mutex_lock(&x_atomic_lock);
    *preval = *atomic;
    if ((success = (*atomic == oldval))) {
        *atomic = newval;
    }
    pthread_mutex_unlock(&x_atomic_lock);

    return success;
}

xint (x_atomic_int_exchange)(xint *atomic, xint newval)
{
    xint oldval;
    xint *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *ptr;
    *ptr = newval;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xint (x_atomic_int_add)(volatile xint *atomic, xint val)
{
    xint oldval;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *atomic;
    *atomic = oldval + val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xuint (x_atomic_int_and)(volatile xuint *atomic, xuint val)
{
    xuint oldval;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *atomic;
    *atomic = oldval & val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xuint (x_atomic_int_or)(volatile xuint *atomic, xuint val)
{
    xuint oldval;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *atomic;
    *atomic = oldval | val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xuint (x_atomic_int_xor)(volatile xuint *atomic, xuint val)
{
    xuint oldval;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *atomic;
    *atomic = oldval ^ val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}


xpointer (x_atomic_pointer_get)(const volatile void *atomic)
{
    xpointer value;
    const xpointer *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    value = *ptr;
    pthread_mutex_unlock(&x_atomic_lock);

    return value;
}

void (x_atomic_pointer_set)(volatile void *atomic, xpointer newval)
{
    xpointer *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    *ptr = newval;
    pthread_mutex_unlock(&x_atomic_lock);
}

xboolean (x_atomic_pointer_compare_and_exchange)(volatile void *atomic, xpointer oldval, xpointer newval)
{
    xboolean success;
    xpointer *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    if ((success = (*ptr == oldval))) {
        *ptr = newval;
    }
    pthread_mutex_unlock(&x_atomic_lock);

    return success;
}

xboolean (x_atomic_pointer_compare_and_exchange_full)(void *atomic, xpointer oldval, xpointer newval, void *preval)
{
    xboolean success;
    xpointer *ptr = atomic;
    xpointer *pre = preval;

    pthread_mutex_lock(&x_atomic_lock);
    *pre = *ptr;
    if ((success = (*ptr == oldval))) {
        *ptr = newval;
    }
    pthread_mutex_unlock(&x_atomic_lock);

    return success;
}

xpointer (x_atomic_pointer_exchange)(void *atomic, xpointer newval)
{
    xpointer oldval;
    xpointer *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *ptr;
    *ptr = newval;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xssize (x_atomic_pointer_add)(volatile void *atomic, xssize val)
{
    xssize oldval;
    xssize *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *ptr;
    *ptr = oldval + val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xsize (x_atomic_pointer_and)(volatile void *atomic, xsize val)
{
    xsize oldval;
    xsize *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *ptr;
    *ptr = oldval & val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xsize (x_atomic_pointer_or)(volatile void *atomic, xsize val)
{
    xsize oldval;
    xsize *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *ptr;
    *ptr = oldval | val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}

xsize (x_atomic_pointer_xor)(volatile void *atomic, xsize val)
{
    xsize oldval;
    xsize *ptr = atomic;

    pthread_mutex_lock(&x_atomic_lock);
    oldval = *ptr;
    *ptr = oldval ^ val;
    pthread_mutex_unlock(&x_atomic_lock);

    return oldval;
}
#endif

xint x_atomic_int_exchange_and_add(volatile xint *atomic, xint val)
{
    return (x_atomic_int_add)((xint *)atomic, val);
}
