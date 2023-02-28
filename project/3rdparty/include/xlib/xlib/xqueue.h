#ifndef __X_QUEUE_H__
#define __X_QUEUE_H__

#include "xlist.h"

X_BEGIN_DECLS

typedef struct _XQueue XQueue;

struct _XQueue {
    XList *head;
    XList *tail;
    xuint length;
};

#define X_QUEUE_INIT        { NULL, NULL, 0 }

XLIB_AVAILABLE_IN_ALL
XQueue *x_queue_new(void);

XLIB_AVAILABLE_IN_ALL
void x_queue_free(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_queue_free_full(XQueue *queue, XDestroyNotify free_func);

XLIB_AVAILABLE_IN_ALL
void x_queue_init(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_queue_clear(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
xboolean x_queue_is_empty(XQueue *queue);

XLIB_AVAILABLE_IN_2_60
void x_queue_clear_full(XQueue *queue, XDestroyNotify free_func);

XLIB_AVAILABLE_IN_ALL
xuint x_queue_get_length(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_queue_reverse(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
XQueue *x_queue_copy(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
void x_queue_foreach(XQueue *queue, XFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_find(XQueue *queue, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_find_custom(XQueue *queue, xconstpointer data, XCompareFunc func);

XLIB_AVAILABLE_IN_ALL
void x_queue_sort (XQueue *queue, XCompareDataFunc compare_func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_queue_push_head(XQueue *queue, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_queue_push_tail(XQueue *queue, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_queue_push_nth(XQueue *queue, xpointer data, xint n);

XLIB_AVAILABLE_IN_ALL
xpointer x_queue_pop_head(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_queue_pop_tail(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_queue_pop_nth(XQueue *queue, xuint n);

XLIB_AVAILABLE_IN_ALL
xpointer x_queue_peek_head(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_queue_peek_tail(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
xpointer x_queue_peek_nth(XQueue *queue, xuint n);

XLIB_AVAILABLE_IN_ALL
xint x_queue_index(XQueue *queue, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
xboolean x_queue_remove(XQueue *queue, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
xuint x_queue_remove_all(XQueue *queue, xconstpointer data);

XLIB_AVAILABLE_IN_ALL
void x_queue_insert_before(XQueue *queue, XList *sibling, xpointer data);

XLIB_AVAILABLE_IN_2_62
void x_queue_insert_before_link(XQueue *queue, XList *sibling, XList *link_);

XLIB_AVAILABLE_IN_ALL
void x_queue_insert_after(XQueue *queue, XList *sibling, xpointer data);

XLIB_AVAILABLE_IN_2_62
void x_queue_insert_after_link(XQueue *queue, XList *sibling, XList *link_);

XLIB_AVAILABLE_IN_ALL
void x_queue_insert_sorted(XQueue *queue, xpointer data, XCompareDataFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_queue_push_head_link(XQueue *queue, XList *link_);

XLIB_AVAILABLE_IN_ALL
void x_queue_push_tail_link(XQueue *queue, XList *link_);

XLIB_AVAILABLE_IN_ALL
void x_queue_push_nth_link(XQueue *queue, xint n, XList *link_);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_pop_head_link(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_pop_tail_link(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_pop_nth_link(XQueue *queue, xuint n);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_peek_head_link(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_peek_tail_link(XQueue *queue);

XLIB_AVAILABLE_IN_ALL
XList *x_queue_peek_nth_link(XQueue *queue, xuint n);

XLIB_AVAILABLE_IN_ALL
xint x_queue_link_index(XQueue *queue, XList *link_);

XLIB_AVAILABLE_IN_ALL
void x_queue_unlink(XQueue *queue, XList *link_);

XLIB_AVAILABLE_IN_ALL
void x_queue_delete_link(XQueue *queue, XList *link_);

X_END_DECLS

#endif
