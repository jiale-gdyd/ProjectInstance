#ifndef __X_UTILS_PRIVATE_H__
#define __X_UTILS_PRIVATE_H__

#include <time.h>
#include <math.h>

#include "xlibconfig.h"
#include "xtypes.h"
#include "xtestutils.h"

X_BEGIN_DECLS

void x_set_user_dirs(const xchar *first_dir_type, ...) X_GNUC_NULL_TERMINATED;

static inline xsize x_nearest_pow(xsize num)
{
    xsize n = num - 1;

    x_assert((num > 0) && (num <= (X_MAXSIZE / 2)));

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if XLIB_SIZEOF_SIZE_T == 8
    n |= n >> 32;
#endif

    return n + 1;
}

static inline int x_isnan(double d)
{
    return isnan(d);
}

void _x_unset_cached_tmp_dir(void);

xboolean _x_localtime(time_t timet, struct tm *tm);

xboolean x_set_prgname_once(const xchar *prgname);

X_END_DECLS

#endif
