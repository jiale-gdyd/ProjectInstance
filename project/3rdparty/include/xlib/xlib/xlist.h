#ifndef __X_LIST_H__
#define __X_LIST_H__

#include "xmem.h"
#include "xnode.h"

X_BEGIN_DECLS

typedef struct _XList {
    xpointer      data;
    struct _XList *next;
    struct _XList *prev;
} XList;

XLIB_AVAILABLE_IN_ALL
XList *x_list_alloc(void) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
void x_list_free(XList *list);

XLIB_AVAILABLE_IN_ALL
void x_list_free_1(XList *list);

#define x_list_free1                   x_list_free_1

XLIB_AVAILABLE_IN_ALL
void x_list_free_full(XList *list, XDestroyNotify free_func);

XLIB_AVAILABLE_IN_ALL
XList *x_list_append(XList *list, xpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_prepend(XList *list, xpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_insert(XList *list, xpointer data, xint position) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_insert_sorted(XList *list, xpointer data, XCompareFunc func) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_insert_sorted_with_data(XList*list, xpointer data, XCompareDataFunc func, xpointer user_data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_insert_before(XList *list, XList *sibling, xpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_62
XList *x_list_insert_before_link(XList *list, XList *sibling, XList *link_) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_concat(XList *list1, XList *list2) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_remove(XList *list, xconstpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_remove_all(XList *list, xconstpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_remove_link(XList *list, XList *llink) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_delete_link(XList *list, XList *link_) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_reverse(XList *list) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_copy(XList *list) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_34
XList *x_list_copy_deep(XList *list, XCopyFunc func, xpointer user_data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_nth(XList *list, xuint n);

XLIB_AVAILABLE_IN_ALL
XList *x_list_nth_prev(XList *list, xuint n);

XLIB_AVAILABLE_IN_ALL
XList *x_list_find(XList *list, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
XList *x_list_find_custom(XList *list, xconstpointer data, XCompareFunc func);

XLIB_AVAILABLE_IN_ALL
xint x_list_position(XList *list, XList *llink);

XLIB_AVAILABLE_IN_ALL
xint x_list_index(XList *list, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
XList *x_list_last(XList *list);

XLIB_AVAILABLE_IN_ALL
XList *x_list_first(XList *list);

XLIB_AVAILABLE_IN_ALL
xuint x_list_length(XList *list);

XLIB_AVAILABLE_IN_ALL
void x_list_foreach(XList *list, XFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
XList *x_list_sort(XList *list, XCompareFunc compare_func) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XList *x_list_sort_with_data(XList *list, XCompareDataFunc compare_func, xpointer user_data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
xpointer x_list_nth_data(XList *list, xuint n);

XLIB_AVAILABLE_IN_2_64
void x_clear_list (XList **list_ptr, XDestroyNotify destroy);

#define x_clear_list(list_ptr, destroy)                 \
    X_STMT_START {                                      \
        XList *_list;                                   \
                                                        \
        _list = *(list_ptr);                            \
        if (_list) {                                    \
            *list_ptr = NULL;                           \
                                                        \
            if ((destroy) != NULL) {                    \
                x_list_free_full(_list, (destroy));     \
            } else {                                    \
                x_list_free(_list);                     \
            }                                           \
        }                                               \
    } X_STMT_END                                        \
    XLIB_AVAILABLE_MACRO_IN_2_64

#define x_list_previous(list)               ((list) ? (((XList *)(list))->prev) : NULL)
#define x_list_next(list)                   ((list) ? (((XList *)(list))->next) : NULL)

X_END_DECLS

#endif
