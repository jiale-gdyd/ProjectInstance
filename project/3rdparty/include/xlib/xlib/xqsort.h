#ifndef __X_QSORT_H__
#define __X_QSORT_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_DEPRECATED_IN_2_82_FOR(x_sort_array)
void x_qsort_with_data(xconstpointer pbase, xint total_elems, xsize size, XCompareDataFunc compare_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_82
void x_sort_array(const void *array, size_t n_elements, size_t element_size, XCompareDataFunc compare_func, void *user_data);

X_END_DECLS

#endif
