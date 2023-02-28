#include <xlib/xlib/config.h>
#include <xlib/xlib/xqueue.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtestutils.h>

XQueue *x_queue_new(void)
{
    return x_slice_new0(XQueue);
}

void x_queue_free(XQueue *queue)
{
    x_return_if_fail(queue != NULL);

    x_list_free(queue->head);
    x_slice_free(XQueue, queue);
}

void x_queue_free_full(XQueue *queue, XDestroyNotify free_func)
{
    x_queue_foreach(queue, (XFunc)free_func, NULL);
    x_queue_free(queue);
}

void x_queue_init(XQueue *queue)
{
    x_return_if_fail(queue != NULL);

    queue->head = queue->tail = NULL;
    queue->length = 0;
}

void x_queue_clear(XQueue *queue)
{
    x_return_if_fail(queue != NULL);

    x_list_free(queue->head);
    x_queue_init(queue);
}

void x_queue_clear_full(XQueue *queue, XDestroyNotify free_func)
{
    x_return_if_fail(queue != NULL);

    if (free_func != NULL) {
        x_queue_foreach(queue, (XFunc)free_func, NULL);
    }
    x_queue_clear(queue);
}

xboolean x_queue_is_empty(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, TRUE);
    return queue->head == NULL;
}

xuint x_queue_get_length(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, 0);
    return queue->length;
}

void x_queue_reverse(XQueue *queue)
{
    x_return_if_fail(queue != NULL);

    queue->tail = queue->head;
    queue->head = x_list_reverse(queue->head);
}

XQueue *x_queue_copy(XQueue *queue)
{
    XList *list;
    XQueue *result;

    x_return_val_if_fail(queue != NULL, NULL);

    result = x_queue_new();

    for (list = queue->head; list != NULL; list = list->next) {
        x_queue_push_tail(result, list->data);
    }

    return result;
}

void x_queue_foreach (XQueue *queue, XFunc func, xpointer user_data)
{
    XList *list;

    x_return_if_fail (queue != NULL);
    x_return_if_fail (func != NULL);

    list = queue->head;
    while (list) {
        XList *next = list->next;
        func(list->data, user_data);
        list = next;
    }
}

XList *x_queue_find(XQueue *queue, xconstpointer data)
{
    x_return_val_if_fail(queue != NULL, NULL);
    return x_list_find(queue->head, data);
}

XList *x_queue_find_custom(XQueue *queue, xconstpointer data, XCompareFunc func)
{
    x_return_val_if_fail(queue != NULL, NULL);
    x_return_val_if_fail(func != NULL, NULL);

    return x_list_find_custom(queue->head, data, func);
}

void x_queue_sort(XQueue *queue, XCompareDataFunc compare_func, xpointer user_data)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(compare_func != NULL);

    queue->head = x_list_sort_with_data(queue->head, compare_func, user_data);
    queue->tail = x_list_last(queue->head);
}

void x_queue_push_head(XQueue *queue, xpointer data)
{
    x_return_if_fail(queue != NULL);

    queue->head = x_list_prepend(queue->head, data);
    if (!queue->tail) {
        queue->tail = queue->head;
    }
    queue->length++;
}

void x_queue_push_nth(XQueue *queue, xpointer data, xint n)
{
    x_return_if_fail(queue != NULL);

    if (n < 0 || (xuint)n >= queue->length) {
        x_queue_push_tail(queue, data);
        return;
    }

    x_queue_insert_before(queue, x_queue_peek_nth_link(queue, n), data);
}

void x_queue_push_head_link(XQueue *queue, XList *link)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(link != NULL);
    x_return_if_fail(link->prev == NULL);
    x_return_if_fail(link->next == NULL);

    link->next = queue->head;
    if (queue->head) {
        queue->head->prev = link;
    } else {
        queue->tail = link;
    }
    queue->head = link;
    queue->length++;
}

void x_queue_push_tail(XQueue *queue, xpointer data)
{
    x_return_if_fail(queue != NULL);

    queue->tail = x_list_append(queue->tail, data);
    if (queue->tail->next) {
        queue->tail = queue->tail->next;
    } else {
        queue->head = queue->tail;
    }
    queue->length++;
}

void x_queue_push_tail_link(XQueue *queue, XList *link)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(link != NULL);
    x_return_if_fail(link->prev == NULL);
    x_return_if_fail(link->next == NULL);

    link->prev = queue->tail;
    if (queue->tail) {
        queue->tail->next = link;
    } else {
        queue->head = link;
    }
    queue->tail = link;
    queue->length++;
}

void x_queue_push_nth_link(XQueue *queue, xint n, XList *link_)
{
    XList *next;
    XList *prev;

    x_return_if_fail(queue != NULL);
    x_return_if_fail(link_ != NULL);

    if (n < 0 || (xuint)n >= queue->length) {
        x_queue_push_tail_link(queue, link_);
        return;
    }

    x_assert(queue->head);
    x_assert(queue->tail);

    next = x_queue_peek_nth_link(queue, n);
    prev = next->prev;

    if (prev) {
        prev->next = link_;
    }
    next->prev = link_;

    link_->next = next;
    link_->prev = prev;

    if (queue->head->prev) {
        queue->head = queue->head->prev;
    }

    x_assert(queue->tail->next == NULL);
    queue->length++;
}

xpointer x_queue_pop_head(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);

    if (queue->head) {
        XList *node = queue->head;
        xpointer data = node->data;

        queue->head = node->next;
        if (queue->head) {
            queue->head->prev = NULL;
        } else {
            queue->tail = NULL;
        }

        x_list_free_1 (node);
        queue->length--;

        return data;
    }

    return NULL;
}

XList *x_queue_pop_head_link(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);

    if (queue->head) {
        XList *node = queue->head;

        queue->head = node->next;
        if (queue->head) {
            queue->head->prev = NULL;
            node->next = NULL;
        } else {
            queue->tail = NULL;
        }

        queue->length--;
        return node;
    }

    return NULL;
}

XList *x_queue_peek_head_link(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);
    return queue->head;
}

XList *x_queue_peek_tail_link(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);
    return queue->tail;
}

xpointer x_queue_pop_tail(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);

    if (queue->tail) {
        XList *node = queue->tail;
        xpointer data = node->data;

        queue->tail = node->prev;
        if (queue->tail) {
            queue->tail->next = NULL;
        } else {
            queue->head = NULL;
        }

        queue->length--;
        x_list_free_1(node);

        return data;
    }

    return NULL;
}

xpointer x_queue_pop_nth(XQueue *queue, xuint n)
{
    XList *nth_link;
    xpointer result;

    x_return_val_if_fail(queue != NULL, NULL);
    if (n >= queue->length) {
        return NULL;
    }

    nth_link = x_queue_peek_nth_link(queue, n);
    result = nth_link->data;
    x_queue_delete_link(queue, nth_link);

    return result;
}

XList *x_queue_pop_tail_link(XQueue *queue)
{
    x_return_val_if_fail (queue != NULL, NULL);

    if (queue->tail) {
        XList *node = queue->tail;

        queue->tail = node->prev;
        if (queue->tail) {
            queue->tail->next = NULL;
            node->prev = NULL;
        } else {
            queue->head = NULL;
        }

        queue->length--;
        return node;
    }

    return NULL;
}

XList *x_queue_pop_nth_link(XQueue *queue, xuint n)
{
    XList *link;

    x_return_val_if_fail(queue != NULL, NULL);

    if (n >= queue->length) {
        return NULL;
    }

    link = x_queue_peek_nth_link(queue, n);
    x_queue_unlink(queue, link);

    return link;
}

XList *x_queue_peek_nth_link(XQueue *queue, xuint n)
{
    xuint i;
    XList *link;

    x_return_val_if_fail(queue != NULL, NULL);

    if (n >= queue->length) {
        return NULL;
    }

    if (n > queue->length / 2) {
        n = queue->length - n - 1;

        link = queue->tail;
        for (i = 0; i < n; ++i) {
            link = link->prev;
        }
    } else {
        link = queue->head;
        for (i = 0; i < n; ++i) {
            link = link->next;
        }
    }

    return link;
}

xint x_queue_link_index(XQueue *queue, XList *link_)
{
    x_return_val_if_fail(queue != NULL, -1);
    return x_list_position(queue->head, link_);
}

void x_queue_unlink(XQueue *queue, XList *link_)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(link_ != NULL);

    if (link_ == queue->tail) {
        queue->tail = queue->tail->prev;
    }

    queue->head = x_list_remove_link(queue->head, link_);
    queue->length--;
}

void x_queue_delete_link(XQueue *queue, XList *link_)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(link_ != NULL);

    x_queue_unlink(queue, link_);
    x_list_free(link_);
}

xpointer x_queue_peek_head(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);
    return queue->head ? queue->head->data : NULL;
}

xpointer x_queue_peek_tail(XQueue *queue)
{
    x_return_val_if_fail(queue != NULL, NULL);
    return queue->tail ? queue->tail->data : NULL;
}

xpointer x_queue_peek_nth(XQueue *queue, xuint n)
{
    XList *link;

    x_return_val_if_fail(queue != NULL, NULL);

    link = x_queue_peek_nth_link(queue, n);
    if (link) {
        return link->data;
    }

    return NULL;
}

xint x_queue_index(XQueue *queue, xconstpointer data)
{
    x_return_val_if_fail(queue != NULL, -1);
    return x_list_index(queue->head, data);
}

xboolean x_queue_remove(XQueue *queue, xconstpointer data)
{
    XList *link;

    x_return_val_if_fail(queue != NULL, FALSE);

    link = x_list_find(queue->head, data);
    if (link) {
        x_queue_delete_link(queue, link);
    }

    return (link != NULL);
}

xuint x_queue_remove_all(XQueue *queue, xconstpointer data)
{
    XList *list;
    xuint old_length;

    x_return_val_if_fail(queue != NULL, 0);

    old_length = queue->length;

    list = queue->head;
    while (list) {
        XList *next = list->next;
        if (list->data == data) {
            x_queue_delete_link(queue, list);
        }

        list = next;
    }

    return (old_length - queue->length);
}

void x_queue_insert_before(XQueue *queue, XList *sibling, xpointer data)
{
    x_return_if_fail(queue != NULL);

    if (sibling == NULL) {
        x_queue_push_tail(queue, data);
    } else {
        queue->head = x_list_insert_before(queue->head, sibling, data);
        queue->length++;
    }
}

void x_queue_insert_before_link(XQueue *queue, XList *sibling, XList *link_)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(link_ != NULL);
    x_return_if_fail(link_->prev == NULL);
    x_return_if_fail(link_->next == NULL);

    if X_UNLIKELY(sibling == NULL) {
        x_queue_push_tail_link(queue, link_);
    } else {
        queue->head = x_list_insert_before_link(queue->head, sibling, link_);
        queue->length++;
    }
}

void x_queue_insert_after(XQueue *queue, XList *sibling, xpointer data)
{
    x_return_if_fail(queue != NULL);

    if (sibling == NULL) {
        x_queue_push_head(queue, data);
    } else {
        x_queue_insert_before(queue, sibling->next, data);
    }
}

void x_queue_insert_after_link(XQueue *queue, XList *sibling, XList *link_)
{
    x_return_if_fail(queue != NULL);
    x_return_if_fail(link_ != NULL);
    x_return_if_fail(link_->prev == NULL);
    x_return_if_fail(link_->next == NULL);

    if X_UNLIKELY(sibling == NULL) {
        x_queue_push_head_link(queue, link_);
    } else {
        x_queue_insert_before_link(queue, sibling->next, link_);
    }
}

void x_queue_insert_sorted(XQueue *queue, xpointer data, XCompareDataFunc func, xpointer user_data)
{
    XList *list;

    x_return_if_fail(queue != NULL);

    list = queue->head;
    while (list && func(list->data, data, user_data) < 0) {
        list = list->next;
    }

    x_queue_insert_before(queue, list, data);
}
