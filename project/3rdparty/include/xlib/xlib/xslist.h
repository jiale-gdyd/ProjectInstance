#ifndef __X_SLIST_H__
#define __X_SLIST_H__

#include "xmem.h"
#include "xnode.h"

X_BEGIN_DECLS

typedef struct _XSList XSList;

struct _XSList {
    xpointer data;
    XSList *next;
};

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_alloc(void) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
void x_slist_free(XSList *list);

XLIB_AVAILABLE_IN_ALL
void x_slist_free_1(XSList *list);

#define x_slist_free1               x_slist_free_1

XLIB_AVAILABLE_IN_ALL
void x_slist_free_full(XSList *list, XDestroyNotify free_func);

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_append(XSList *list, xpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_prepend(XSList *list, xpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_insert(XSList *list, xpointer data, xint position) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_insert_sorted(XSList *list, xpointer data, XCompareFunc func) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_insert_sorted_with_data(XSList *list, xpointer data, XCompareDataFunc func, xpointer user_data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_insert_before(XSList *slist, XSList *sibling, xpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_concat(XSList *list1, XSList *list2) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_remove(XSList *list, xconstpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_remove_all(XSList *list, xconstpointer data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_remove_link(XSList *list, XSList *link_) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_delete_link(XSList *list, XSList *link_) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_reverse(XSList *list) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_copy(XSList *list) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_34
XSList *x_slist_copy_deep(XSList *list, XCopyFunc func, xpointer user_data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_nth(XSList *list, xuint n);

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_find(XSList *list, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_find_custom(XSList *list, xconstpointer data, XCompareFunc func);

XLIB_AVAILABLE_IN_ALL
xint x_slist_position(XSList *list, XSList *llink);

XLIB_AVAILABLE_IN_ALL
xint x_slist_index(XSList *list, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_last(XSList *list);

XLIB_AVAILABLE_IN_ALL
xuint x_slist_length(XSList *list);

XLIB_AVAILABLE_IN_ALL
void x_slist_foreach(XSList *list, XFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_sort(XSList *list, XCompareFunc compare_func) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
XSList *x_slist_sort_with_data(XSList *list, XCompareDataFunc compare_func, xpointer user_data) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_ALL
xpointer x_slist_nth_data(XSList *list, xuint n);

XLIB_AVAILABLE_IN_2_64
void x_clear_slist(XSList **slist_ptr, XDestroyNotify destroy);

#define x_clear_slist(slist_ptr, destroy)               \
    X_STMT_START {                                      \
        XSList *_slist;                                 \
                                                        \
        _slist = *(slist_ptr);                          \
        if (_slist) {                                   \
            *slist_ptr = NULL;                          \
                                                        \
            if ((destroy) != NULL) {                    \
                x_slist_free_full(_slist, (destroy));   \
            } else {                                    \
                x_slist_free(_slist);                   \
            }                                           \
        }                                               \
    } X_STMT_END                                        \
    XLIB_AVAILABLE_MACRO_IN_2_64

#define x_slist_next(slist)                             ((slist) ? (((XSList *)(slist))->next) : NULL)

X_END_DECLS

#endif
