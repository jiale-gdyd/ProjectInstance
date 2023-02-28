#ifndef __X_QUARK_H__
#define __X_QUARK_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef xuint32 XQuark;

XLIB_AVAILABLE_IN_ALL
XQuark x_quark_try_string(const xchar *string);

XLIB_AVAILABLE_IN_ALL
XQuark x_quark_from_static_string(const xchar *string);

XLIB_AVAILABLE_IN_ALL
XQuark x_quark_from_string(const xchar *string);

XLIB_AVAILABLE_IN_ALL
const xchar *x_quark_to_string(XQuark quark) X_GNUC_CONST;

#define X_DEFINE_QUARK(QN, q_n)                             \
    XQuark q_n##_quark (void)                               \
    {                                                       \
        static XQuark q;                                    \
                                                            \
        if X_UNLIKELY (q == 0) {                            \
            q = x_quark_from_static_string(#QN);            \
        }                                                   \
                                                            \
        return q;                                           \
    }

XLIB_AVAILABLE_IN_ALL
const xchar *x_intern_string(const xchar *string);

XLIB_AVAILABLE_IN_ALL
const xchar *x_intern_static_string(const xchar *string);

X_END_DECLS

#endif
