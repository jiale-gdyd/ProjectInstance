#ifndef __X_RAND_H__
#define __X_RAND_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XRand XRand;

XLIB_AVAILABLE_IN_ALL
XRand *x_rand_new_with_seed(xuint32  seed);

XLIB_AVAILABLE_IN_ALL
XRand *x_rand_new_with_seed_array(const xuint32 *seed, xuint seed_length);

XLIB_AVAILABLE_IN_ALL
XRand *x_rand_new(void);

XLIB_AVAILABLE_IN_ALL
void x_rand_free(XRand *rand_);

XLIB_AVAILABLE_IN_ALL
XRand *x_rand_copy(XRand *rand_);

XLIB_AVAILABLE_IN_ALL
void x_rand_set_seed(XRand *rand_, xuint32 seed);

XLIB_AVAILABLE_IN_ALL
void x_rand_set_seed_array(XRand *rand_, const xuint32 *seed, xuint seed_length);

#define x_rand_boolean(rand_)           ((x_rand_int(rand_) & (1 << 15)) != 0)

XLIB_AVAILABLE_IN_ALL
xuint32 x_rand_int(XRand *rand_);

XLIB_AVAILABLE_IN_ALL
xint32 x_rand_int_range(XRand *rand_, xint32 begin, xint32 end);

XLIB_AVAILABLE_IN_ALL
xdouble x_rand_double(XRand *rand_);

XLIB_AVAILABLE_IN_ALL
xdouble x_rand_double_range(XRand *rand_, xdouble begin, xdouble end);

XLIB_AVAILABLE_IN_ALL
void x_random_set_seed(xuint32 seed);

#define x_random_boolean()              ((x_random_int() & (1 << 15)) != 0)

XLIB_AVAILABLE_IN_ALL
xuint32 x_random_int(void);

XLIB_AVAILABLE_IN_ALL
xint32 x_random_int_range(xint32 begin, xint32 end);

XLIB_AVAILABLE_IN_ALL
xdouble x_random_double (void);

XLIB_AVAILABLE_IN_ALL
xdouble x_random_double_range(xdouble begin, xdouble end);

X_END_DECLS

#endif
