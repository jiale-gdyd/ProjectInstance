#ifndef __X_QSORT_H__
#define __X_QSORT_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
void x_qsort_with_data(xconstpointer pbase, xint total_elems, xsize size, XCompareDataFunc compare_func, xpointer user_data);

X_END_DECLS

#endif
