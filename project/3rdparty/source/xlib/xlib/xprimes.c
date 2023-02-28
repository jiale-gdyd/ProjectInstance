#include <xlib/xlib/config.h>
#include <xlib/xlib/xprimes.h>

static const xuint x_primes[] = {
    11,
    19,
    37,
    73,
    109,
    163,
    251,
    367,
    557,
    823,
    1237,
    1861,
    2777,
    4177,
    6247,
    9371,
    14057,
    21089,
    31627,
    47431,
    71143,
    106721,
    160073,
    240101,
    360163,
    540217,
    810343,
    1215497,
    1823231,
    2734867,
    4102283,
    6153409,
    9230113,
    13845163,
};

xuint x_spaced_primes_closest(xuint num)
{
    xsize i;

    for (i = 0; i < X_N_ELEMENTS(x_primes); i++) {
        if (x_primes[i] > num) {
            return x_primes[i];
        }
    }

    return x_primes[X_N_ELEMENTS(x_primes) - 1];
}
