#include <xlib/xlib/config.h>
#include <xlib/xlib/xlist.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xtestutils.h>

#define _x_list_alloc()         x_slice_new(XList)
#define _x_list_alloc0()        x_slice_new0(XList)
#define _x_list_free1(list)     x_slice_free(XList, list)

XList *x_list_alloc(void)
{
    return _x_list_alloc0();
}

void x_list_free(XList *list)
{
    x_slice_free_chain(XList, list, next);
}

void x_list_free_1(XList *list)
{
    _x_list_free1(list);
}

void x_list_free_full(XList *list, XDestroyNotify free_func)
{
    x_list_foreach(list, (XFunc)free_func, NULL);
    x_list_free(list);
}

XList *x_list_append(XList *list, xpointer data)
{
    XList *last;
    XList *new_list;

    new_list = _x_list_alloc();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        last = x_list_last(list);
        last->next = new_list;
        new_list->prev = last;

        return list;
    } else {
        new_list->prev = NULL;
        return new_list;
    }
}

XList *x_list_prepend(XList *list, xpointer data)
{
    XList *new_list;

    new_list = _x_list_alloc();
    new_list->data = data;
    new_list->next = list;
    
    if (list) {
        new_list->prev = list->prev;
        if (list->prev) {
            list->prev->next = new_list;
        }
        list->prev = new_list;
    } else {
        new_list->prev = NULL;
    }

    return new_list;
}

XList *x_list_insert(XList *list, xpointer data, xint position)
{
    XList *new_list;
    XList *tmp_list;

    if (position < 0) {
        return x_list_append(list, data);
    } else if (position == 0) {
        return x_list_prepend(list, data);
    }

    tmp_list = x_list_nth(list, position);
    if (!tmp_list) {
        return x_list_append(list, data);
    }

    new_list = _x_list_alloc();
    new_list->data = data;
    new_list->prev = tmp_list->prev;
    tmp_list->prev->next = new_list;
    new_list->next = tmp_list;
    tmp_list->prev = new_list;

    return list;
}

XList *x_list_insert_before_link(XList *list, XList *sibling, XList *link_)
{
    x_return_val_if_fail(link_ != NULL, list);
    x_return_val_if_fail(link_->prev == NULL, list);
    x_return_val_if_fail(link_->next == NULL, list);

    if (list == NULL) {
        x_return_val_if_fail(sibling == NULL, list);
        return link_;
    } else if (sibling != NULL) {
        link_->prev = sibling->prev;
        link_->next = sibling;
        sibling->prev = link_;

        if (link_->prev != NULL) {
            link_->prev->next = link_;
            return list;
        } else {
            x_return_val_if_fail(sibling == list, link_);
            return link_;
        }
    } else {
        XList *last;

        for (last = list; last->next != NULL; last = last->next);

        last->next = link_;
        last->next->prev = last;
        last->next->next = NULL;

        return list;
    }
}

XList *x_list_insert_before(XList *list, XList *sibling, xpointer data)
{
    if (list == NULL) {
        list = x_list_alloc();
        list->data = data;

        x_return_val_if_fail(sibling == NULL, list);
        return list;
    } else if (sibling != NULL) {
        XList *node;

        node = _x_list_alloc();
        node->data = data;
        node->prev = sibling->prev;
        node->next = sibling;
        sibling->prev = node;

        if (node->prev != NULL) {
            node->prev->next = node;
            return list;
        } else {
            x_return_val_if_fail(sibling == list, node);
            return node;
        }
    } else {
        XList *last;

        for (last = list; last->next != NULL; last = last->next);

        last->next = _x_list_alloc();
        last->next->data = data;
        last->next->prev = last;
        last->next->next = NULL;

        return list;
    }
}

XList *x_list_concat(XList *list1, XList *list2)
{
    XList *tmp_list;

    if (list2) {
        tmp_list = x_list_last(list1);
        if (tmp_list) {
            tmp_list->next = list2;
        } else {
            list1 = list2;
        }
        list2->prev = tmp_list;
    }

    return list1;
}

static inline XList *_x_list_remove_link(XList *list, XList *link)
{
    if (link == NULL) {
        return list;
    }

    if (link->prev) {
        if (link->prev->next == link) {
            link->prev->next = link->next;
        } else {
            x_warning("corrupted double-linked list detected");
        }
    }

    if (link->next) {
        if (link->next->prev == link) {
            link->next->prev = link->prev;
        } else {
            x_warning("corrupted double-linked list detected");
        }
    }

    if (link == list) {
        list = list->next;
    }

    link->next = NULL;
    link->prev = NULL;

    return list;
}

XList *x_list_remove(XList *list, xconstpointer data)
{
    XList *tmp;

    tmp = list;
    while (tmp) {
        if (tmp->data != data) {
            tmp = tmp->next;
        } else {
            list = _x_list_remove_link(list, tmp);
            _x_list_free1(tmp);
            break;
        }
    }

    return list;
}

XList *x_list_remove_all(XList *list, xconstpointer data)
{
    XList *tmp = list;

    while (tmp) {
        if (tmp->data != data) {
            tmp = tmp->next;
        } else {
            XList *next = tmp->next;

            if (tmp->prev) {
                tmp->prev->next = next;
            } else {
                list = next;
            }

            if (next) {
                next->prev = tmp->prev;
            }

            _x_list_free1(tmp);
            tmp = next;
        }
    }

    return list;
}

XList *x_list_remove_link(XList *list, XList *llink)
{
    return _x_list_remove_link(list, llink);
}

XList *x_list_delete_link(XList *list, XList *link_)
{
    list = _x_list_remove_link(list, link_);
    _x_list_free1(link_);

    return list;
}

XList *x_list_copy(XList *list)
{
    return x_list_copy_deep(list, NULL, NULL);
}

XList *x_list_copy_deep(XList *list, XCopyFunc func, xpointer user_data)
{
    XList *new_list = NULL;

    if (list) {
        XList *last;

        new_list = _x_list_alloc();
        if (func) {
            new_list->data = func(list->data, user_data);
        } else {
            new_list->data = list->data;
        }

        new_list->prev = NULL;
        last = new_list;
        list = list->next;

        while (list) {
            last->next = _x_list_alloc();
            last->next->prev = last;
            last = last->next;

            if (func) {
                last->data = func(list->data, user_data);
            } else {
                last->data = list->data;
            }
            list = list->next;
        }

        last->next = NULL;
    }

    return new_list;
}

XList *x_list_reverse(XList *list)
{
    XList *last;

    last = NULL;
    while (list) {
        last = list;
        list = last->next;
        last->next = last->prev;
        last->prev = list;
    }

    return last;
}

XList *x_list_nth(XList *list, xuint n)
{
    while ((n-- > 0) && list) {
        list = list->next;
    }

    return list;
}

XList *x_list_nth_prev(XList *list, xuint n)
{
    while ((n-- > 0) && list) {
        list = list->prev;
    }

    return list;
}

xpointer x_list_nth_data(XList *list, xuint n)
{
    while ((n-- > 0) && list) {
        list = list->next;
    }

    return list ? list->data : NULL;
}

XList *x_list_find(XList *list, xconstpointer data)
{
    while (list) {
        if (list->data == data) {
            break;
        }

        list = list->next;
    }

    return list;
}

XList *x_list_find_custom(XList *list, xconstpointer data, XCompareFunc func)
{
    x_return_val_if_fail(func != NULL, list);

    while (list) {
        if (!func(list->data, data)) {
            return list;
        }

        list = list->next;
    }

    return NULL;
}

xint x_list_position(XList *list, XList *llink)
{
    xint i;

    i = 0;
    while (list) {
        if (list == llink) {
            return i;
        }

        i++;
        list = list->next;
    }

    return -1;
}

xint x_list_index(XList *list, xconstpointer data)
{
    xint i;

    i = 0;
    while (list) {
        if (list->data == data) {
            return i;
        }

        i++;
        list = list->next;
    }

    return -1;
}

XList *x_list_last(XList *list)
{
    if (list) {
        while (list->next) {
            list = list->next;
        }
    }

    return list;
}

XList *x_list_first(XList *list)
{
    if (list) {
        while (list->prev) {
            list = list->prev;
        }
    }

    return list;
}

xuint x_list_length(XList *list)
{
    xuint length;

    length = 0;
    while (list) {
        length++;
        list = list->next;
    }

    return length;
}

void x_list_foreach(XList *list, XFunc func, xpointer user_data)
{
    while (list) {
        XList *next = list->next;
        (*func)(list->data, user_data);
        list = next;
    }
}

static XList *x_list_insert_sorted_real(XList *list, xpointer data, XFunc func, xpointer user_data)
{
    xint cmp;
    XList *new_list;
    XList *tmp_list = list;

    x_return_val_if_fail(func != NULL, list);

    if (!list) {
        new_list = _x_list_alloc0();
        new_list->data = data;
        return new_list;
    }

    cmp = ((XCompareDataFunc)func)(data, tmp_list->data, user_data);

    while ((tmp_list->next) && (cmp > 0)) {
        tmp_list = tmp_list->next;
        cmp = ((XCompareDataFunc)func)(data, tmp_list->data, user_data);
    }

    new_list = _x_list_alloc0();
    new_list->data = data;

    if ((!tmp_list->next) && (cmp > 0)) {
        tmp_list->next = new_list;
        new_list->prev = tmp_list;
        return list;
    }

    if (tmp_list->prev) {
        tmp_list->prev->next = new_list;
        new_list->prev = tmp_list->prev;
    }

    new_list->next = tmp_list;
    tmp_list->prev = new_list;

    if (tmp_list == list) {
        return new_list;
    } else {
        return list;
    }
}

XList *x_list_insert_sorted(XList *list, xpointer data, XCompareFunc func)
{
    return x_list_insert_sorted_real(list, data, (XFunc)func, NULL);
}

XList *x_list_insert_sorted_with_data(XList *list, xpointer data, XCompareDataFunc func, xpointer user_data)
{
    return x_list_insert_sorted_real(list, data, (XFunc)func, user_data);
}

static XList *x_list_sort_merge(XList *l1,  XList *l2, XFunc compare_func, xpointer user_data)
{
    xint cmp;
    XList list, *l, *lprev;

    l = &list; 
    lprev = NULL;

    while (l1 && l2) {
        cmp = ((XCompareDataFunc)compare_func)(l1->data, l2->data, user_data);
        if (cmp <= 0) {
            l->next = l1;
            l1 = l1->next;
        } else  {
            l->next = l2;
            l2 = l2->next;
        }

        l = l->next;
        l->prev = lprev; 
        lprev = l;
    }

    l->next = l1 ? l1 : l2;
    l->next->prev = l;

    return list.next;
}

static XList *x_list_sort_real(XList *list, XFunc compare_func, xpointer user_data)
{
    XList *l1, *l2;

    if (!list)  {
        return NULL;
    }

    if (!list->next) {
        return list;
    }

    l1 = list; 
    l2 = list->next;

    while ((l2 = l2->next) != NULL) {
        if ((l2 = l2->next) == NULL) {
            break;
        }

        l1 = l1->next;
    }

    l2 = l1->next; 
    l1->next = NULL; 

    return x_list_sort_merge(x_list_sort_real(list, compare_func, user_data), x_list_sort_real(l2, compare_func, user_data), compare_func, user_data);
}

XList *x_list_sort(XList *list, XCompareFunc compare_func)
{
    return x_list_sort_real(list, (XFunc)compare_func, NULL);
}

XList *x_list_sort_with_data(XList *list, XCompareDataFunc compare_func, xpointer user_data)
{
    return x_list_sort_real(list, (XFunc)compare_func, user_data);
}

void (x_clear_list)(XList **list_ptr, XDestroyNotify destroy)
{
    XList *list;

    list = *list_ptr;
    if (list) {
        *list_ptr = NULL;

        if (destroy) {
            x_list_free_full(list, destroy);
        } else {
            x_list_free(list);
        }
    }
}
