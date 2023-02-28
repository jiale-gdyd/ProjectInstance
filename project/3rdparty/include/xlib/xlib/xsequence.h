#ifndef __X_SEQUENCE_H__
#define __X_SEQUENCE_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XSequence XSequence;
typedef struct _XSequenceNode XSequenceIter;

typedef xint (*XSequenceIterCompareFunc)(XSequenceIter *a, XSequenceIter *b, xpointer data);

XLIB_AVAILABLE_IN_ALL
XSequence *x_sequence_new(XDestroyNotify data_destroy);

XLIB_AVAILABLE_IN_ALL
void x_sequence_free(XSequence *seq);

XLIB_AVAILABLE_IN_ALL
xint x_sequence_get_length(XSequence *seq);

XLIB_AVAILABLE_IN_ALL
void x_sequence_foreach(XSequence *seq, XFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_foreach_range(XSequenceIter *begin, XSequenceIter *end, XFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_sort(XSequence *seq, XCompareDataFunc cmp_func, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_sort_iter(XSequence *seq, XSequenceIterCompareFunc cmp_func, xpointer cmp_data);

XLIB_AVAILABLE_IN_2_48
xboolean x_sequence_is_empty(XSequence *seq);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_get_begin_iter(XSequence *seq);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_get_end_iter(XSequence *seq);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_get_iter_at_pos(XSequence *seq, xint pos);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_append(XSequence *seq, xpointer data);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_prepend(XSequence *seq, xpointer data);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_insert_before(XSequenceIter *iter, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_move(XSequenceIter *src, XSequenceIter *dest);

XLIB_AVAILABLE_IN_ALL
void x_sequence_swap(XSequenceIter *a, XSequenceIter *b);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_insert_sorted(XSequence *seq, xpointer data, XCompareDataFunc cmp_func, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_insert_sorted_iter(XSequence *seq, xpointer data, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_sort_changed(XSequenceIter *iter, XCompareDataFunc cmp_func, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_sort_changed_iter(XSequenceIter *iter, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
void x_sequence_remove(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
void x_sequence_remove_range(XSequenceIter *begin, XSequenceIter *end);

XLIB_AVAILABLE_IN_ALL
void x_sequence_move_range(XSequenceIter *dest, XSequenceIter *begin, XSequenceIter *end);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_search(XSequence *seq, xpointer data, XCompareDataFunc cmp_func, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_search_iter(XSequence *seq, xpointer data, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_lookup(XSequence *seq, xpointer data, XCompareDataFunc cmp_func, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_lookup_iter(XSequence *seq, xpointer data, XSequenceIterCompareFunc iter_cmp, xpointer cmp_data);

XLIB_AVAILABLE_IN_ALL
xpointer x_sequence_get(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
void x_sequence_set(XSequenceIter *iter, xpointer data);

XLIB_AVAILABLE_IN_ALL
xboolean x_sequence_iter_is_begin(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
xboolean x_sequence_iter_is_end(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_iter_next(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_iter_prev(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
xint x_sequence_iter_get_position(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_iter_move(XSequenceIter *iter, xint delta);

XLIB_AVAILABLE_IN_ALL
XSequence *x_sequence_iter_get_sequence(XSequenceIter *iter);

XLIB_AVAILABLE_IN_ALL
xint x_sequence_iter_compare(XSequenceIter *a, XSequenceIter *b);

XLIB_AVAILABLE_IN_ALL
XSequenceIter *x_sequence_range_get_midpoint(XSequenceIter *begin, XSequenceIter *end);

X_END_DECLS

#endif
