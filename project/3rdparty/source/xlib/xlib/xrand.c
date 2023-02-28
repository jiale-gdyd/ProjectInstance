#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xrand.h>
#include <xlib/xlib/xtimer.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xtestutils.h>

X_LOCK_DEFINE_STATIC(global_random);

#define N                               624
#define M                               397
#define MATRIX_A                        0x9908b0df
#define UPPER_MASK                      0x80000000
#define LOWER_MASK                      0x7fffffff

#define TEMPERING_MASK_B                0x9d2c5680
#define TEMPERING_MASK_C                0xefc60000
#define TEMPERING_SHIFT_U(y)            (y >> 11)
#define TEMPERING_SHIFT_S(y)            (y << 7)
#define TEMPERING_SHIFT_T(y)            (y << 15)
#define TEMPERING_SHIFT_L(y)            (y >> 18)

#define X_RAND_DOUBLE_TRANSFORM         2.3283064365386962890625e-10

static xuint get_random_version(void)
{
    static xsize initialized = FALSE;
    static xuint random_version;

    if (x_once_init_enter(&initialized)) {
        const xchar *version_string = x_getenv("X_RANDOM_VERSION");
        if (!version_string || version_string[0] == '\000' ||  strcmp(version_string, "2.2") == 0)
            random_version = 22;
        else if (strcmp(version_string, "2.0") == 0)
            random_version = 20;
        else {
            x_warning("Unknown X_RANDOM_VERSION \"%s\". Using version 2.2.", version_string);
            random_version = 22;
        }
        x_once_init_leave(&initialized, TRUE);
    }

    return random_version;
}

struct _XRand {
    xuint32 mt[N];
    xuint   mti;
};

XRand *x_rand_new_with_seed (xuint32 seed)
{
    XRand *rand = x_new0(XRand, 1);
    x_rand_set_seed(rand, seed);

    return rand;
}

XRand *x_rand_new_with_seed_array(const xuint32 *seed, xuint seed_length)
{
    XRand *rand = x_new0(XRand, 1);
    x_rand_set_seed_array(rand, seed, seed_length);
    return rand;
}

XRand *x_rand_new(void)
{
    xuint32 seed[4];
    static xboolean dev_urandom_exists = TRUE;

    if (dev_urandom_exists) {
        FILE *dev_urandom;

        do {
            dev_urandom = fopen("/dev/urandom", "rbe");
        } while X_UNLIKELY(dev_urandom == NULL && errno == EINTR);

        if (dev_urandom) {
            int r;

            setvbuf(dev_urandom, NULL, _IONBF, 0);
            do {
                errno = 0;
                r = fread(seed, sizeof(seed), 1, dev_urandom);
            } while X_UNLIKELY(errno == EINTR);

            if (r != 1) {
                dev_urandom_exists = FALSE;
            }

            fclose(dev_urandom);
        } else {
            dev_urandom_exists = FALSE;
        }
    }

    if (!dev_urandom_exists) {
        xint64 now_us = x_get_real_time();
        seed[0] = now_us / X_USEC_PER_SEC;
        seed[1] = now_us % X_USEC_PER_SEC;
        seed[2] = getpid();
        seed[3] = getppid();
    }

    return x_rand_new_with_seed_array(seed, 4);
}

void x_rand_free(XRand *rand)
{
    x_return_if_fail(rand != NULL);
    x_free(rand);
}

XRand *x_rand_copy(XRand *rand)
{
    XRand *new_rand;

    x_return_val_if_fail(rand != NULL, NULL);

    new_rand = x_new0(XRand, 1);
    memcpy(new_rand, rand, sizeof (XRand));

    return new_rand;
}

void x_rand_set_seed(XRand *rand, xuint32 seed)
{
    x_return_if_fail(rand != NULL);

    switch (get_random_version()) {
        case 20:
            if (seed == 0) {
                seed = 0x6b842128;
            }

            rand->mt[0]= seed;
            for (rand->mti = 1; rand->mti < N; rand->mti++) {
                rand->mt[rand->mti] = (69069 * rand->mt[rand->mti - 1]);
            }
            break;

        case 22:
            rand->mt[0] = seed;
            for (rand->mti = 1; rand->mti < N; rand->mti++) {
                rand->mt[rand->mti] = 1812433253UL * (rand->mt[rand->mti - 1] ^ (rand->mt[rand->mti - 1] >> 30)) + rand->mti;
            }
            break;

        default:
            x_assert_not_reached();
    }
}

void x_rand_set_seed_array(XRand *rand, const xuint32 *seed, xuint seed_length)
{
    xuint i, j, k;

    x_return_if_fail(rand != NULL);
    x_return_if_fail(seed_length >= 1);

    x_rand_set_seed(rand, 19650218UL);

    i = 1;
    j = 0;
    k = (N > seed_length ? N : seed_length);

    for (; k; k--) {
        rand->mt[i] = (rand->mt[i] ^ ((rand->mt[i - 1] ^ (rand->mt[i - 1] >> 30)) * 1664525UL)) + seed[j] + j;
        rand->mt[i] &= 0xffffffffUL;

        i++;
        j++;

        if (i >= N) {
            rand->mt[0] = rand->mt[N - 1];
            i = 1;
        }

        if (j >= seed_length) {
            j = 0;
        }
    }

    for (k = N - 1; k; k--) {
        rand->mt[i] = (rand->mt[i] ^ ((rand->mt[i - 1] ^ (rand->mt[i - 1] >> 30)) * 1566083941UL)) - i;
        rand->mt[i] &= 0xffffffffUL;

        i++;
        if (i >= N) {
            rand->mt[0] = rand->mt[N - 1];
            i = 1;
        }
    }

    rand->mt[0] = 0x80000000UL;
}

xuint32 x_rand_int(XRand *rand)
{
    xuint32 y;
    static const xuint32 mag01[2] = {0x0, MATRIX_A};

    x_return_val_if_fail(rand != NULL, 0);

    if (rand->mti >= N) {
        int kk;

        for (kk = 0; kk < N - M; kk++) {
            y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk + 1] & LOWER_MASK);
            rand->mt[kk] = rand->mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
        }

        for (; kk < N - 1; kk++) {
            y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk + 1] & LOWER_MASK);
            rand->mt[kk] = rand->mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }

        y = (rand->mt[N - 1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
        rand->mt[N - 1] = rand->mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];

        rand->mti = 0;
    }

    y = rand->mt[rand->mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}

xint32 x_rand_int_range(XRand *rand, xint32 begin, xint32 end)
{
    xuint32 random = 0;
    xuint32 dist = end - begin;

    x_return_val_if_fail(rand != NULL, begin);
    x_return_val_if_fail(end > begin, begin);

    switch (get_random_version()) {
        case 20:
            if (dist <= 0x10000L) {
                xdouble double_rand = x_rand_int(rand) * (X_RAND_DOUBLE_TRANSFORM + X_RAND_DOUBLE_TRANSFORM * X_RAND_DOUBLE_TRANSFORM);
                random = (xint32)(double_rand * dist);
            } else {
                random = (xint32)x_rand_double_range(rand, 0, dist);
            }
            break;

        case 22:
            if (dist == 0) {
                random = 0;
            } else {
                xuint32 maxvalue;
                if (dist <= 0x80000000u) {
                    xuint32 leftover = (0x80000000u % dist) * 2;
                    if (leftover >= dist) {
                        leftover -= dist;
                    }
    
                    maxvalue = 0xffffffffu - leftover;
                } else {
                    maxvalue = dist - 1;
                }
            
                do {
                    random = x_rand_int(rand);
                } while (random > maxvalue);

                random %= dist;
            }
            break;

        default:
            x_assert_not_reached();
    }

    return begin + random;
}

xdouble  x_rand_double(XRand *rand)
{
    xdouble retval = x_rand_int(rand) * X_RAND_DOUBLE_TRANSFORM;
    retval = (retval + x_rand_int(rand)) * X_RAND_DOUBLE_TRANSFORM;

    if (retval >= 1.0)  {
        return x_rand_double(rand);
    }

    return retval;
}

xdouble x_rand_double_range(XRand *rand, xdouble begin, xdouble end)
{
    xdouble r;

    r = x_rand_double(rand);
    return r * end - (r - 1) * begin;
}

static XRand *get_global_random(void)
{
    static XRand *global_random;

    if (!global_random) {
        global_random = x_rand_new();
    }

    return global_random;
}

xuint32 x_random_int(void)
{
    xuint32 result;

    X_LOCK(global_random);
    result = x_rand_int(get_global_random());
    X_UNLOCK(global_random);

    return result;
}

xint32 x_random_int_range(xint32 begin, xint32 end)
{
    xint32 result;

    X_LOCK(global_random);
    result = x_rand_int_range(get_global_random(), begin, end);
    X_UNLOCK(global_random);

    return result;
}

xdouble x_random_double(void)
{
    double result;

    X_LOCK(global_random);
    result = x_rand_double(get_global_random ());
    X_UNLOCK(global_random);

    return result;
}

xdouble x_random_double_range(xdouble begin, xdouble end)
{
    double result;

    X_LOCK(global_random);
    result = x_rand_double_range(get_global_random(), begin, end);
    X_UNLOCK(global_random);

    return result;
}

void x_random_set_seed(xuint32 seed)
{
  X_LOCK(global_random);
  x_rand_set_seed(get_global_random(), seed);
  X_UNLOCK(global_random);
}
