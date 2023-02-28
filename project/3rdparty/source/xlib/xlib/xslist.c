#include <xlib/xlib/config.h>
#include <xlib/xlib/xslist.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtestutils.h>

#define _x_slist_alloc0()       x_slice_new0(XSList)
#define _x_slist_alloc()        x_slice_new(XSList)
#define _x_slist_free1(slist)   x_slice_free(XSList, slist)

XSList *x_slist_alloc(void)
{
    return _x_slist_alloc0();
}

void x_slist_free(XSList *list)
{
    x_slice_free_chain(XSList, list, next);
}

void x_slist_free_1(XSList *list)
{
    _x_slist_free1(list);
}

void x_slist_free_full(XSList *list, XDestroyNotify free_func)
{
    x_slist_foreach(list, (XFunc)free_func, NULL);
    x_slist_free(list);
}

XSList *x_slist_append(XSList *list, xpointer data)
{
    XSList *last;
    XSList *new_list;

    new_list = _x_slist_alloc();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        last = x_slist_last(list);
        last->next = new_list;

        return list;
    } else {
        return new_list;
    }
}

XSList *x_slist_prepend(XSList *list, xpointer data)
{
    XSList *new_list;

    new_list = _x_slist_alloc();
    new_list->data = data;
    new_list->next = list;

    return new_list;
}

XSList *x_slist_insert(XSList *list, xpointer data, xint position)
{
    XSList *tmp_list;
    XSList *new_list;
    XSList *prev_list;

    if (position < 0) {
        return x_slist_append(list, data);
    } else if (position == 0) {
        return x_slist_prepend(list, data);
    }

    new_list = _x_slist_alloc();
    new_list->data = data;

    if (!list) {
        new_list->next = NULL;
        return new_list;
    }

    prev_list = NULL;
    tmp_list = list;

    while ((position-- > 0) && tmp_list) {
        prev_list = tmp_list;
        tmp_list = tmp_list->next;
    }

    new_list->next = prev_list->next;
    prev_list->next = new_list;

    return list;
}

XSList *x_slist_insert_before(XSList *slist, XSList *sibling, xpointer data)
{
    if (!slist) {
        slist = _x_slist_alloc();
        slist->data = data;
        slist->next = NULL;

        x_return_val_if_fail(sibling == NULL, slist);
        return slist;
    } else {
        XSList *node, *last = NULL;

        for (node = slist; node; last = node, node = last->next) {
            if (node == sibling) {
                break;
            }
        }

        if (!last) {
            node = _x_slist_alloc();
            node->data = data;
            node->next = slist;

            return node;
        } else {
            node = _x_slist_alloc();
            node->data = data;
            node->next = last->next;
            last->next = node;

            return slist;
        }
    }
}

XSList *x_slist_concat(XSList *list1, XSList *list2)
{
    if (list2) {
        if (list1) {
            x_slist_last(list1)->next = list2;
        } else {
            list1 = list2;
        }
    }

    return list1;
}

static XSList *_x_slist_remove_data(XSList *list, xconstpointer data, xboolean all)
{
    XSList *tmp = NULL;
    XSList **previous_ptr = &list;

    while (*previous_ptr) {
        tmp = *previous_ptr;
        if (tmp->data == data) {
            *previous_ptr = tmp->next;
            x_slist_free_1(tmp);
            if (!all) {
                break;
            }
        } else {
            previous_ptr = &tmp->next;
        }
    }

    return list;
}

XSList *x_slist_remove(XSList *list, xconstpointer data)
{
    return _x_slist_remove_data(list, data, FALSE);
}

XSList *x_slist_remove_all(XSList *list, xconstpointer data)
{
    return _x_slist_remove_data(list, data, TRUE);
}

static inline XSList *_x_slist_remove_link(XSList *list, XSList *link)
{
    XSList *tmp = NULL;
    XSList **previous_ptr = &list;

    while (*previous_ptr) {
        tmp = *previous_ptr;
        if (tmp == link) {
            *previous_ptr = tmp->next;
            tmp->next = NULL;
            break;
        }

        previous_ptr = &tmp->next;
    }

    return list;
}

XSList *x_slist_remove_link(XSList *list, XSList *link_)
{
    return _x_slist_remove_link(list, link_);
}

XSList *x_slist_delete_link(XSList *list, XSList *link_)
{
    list = _x_slist_remove_link(list, link_);
    _x_slist_free1(link_);

    return list;
}

XSList *x_slist_copy(XSList *list)
{
    return x_slist_copy_deep(list, NULL, NULL);
}

XSList *x_slist_copy_deep(XSList *list, XCopyFunc func, xpointer user_data)
{
    XSList *new_list = NULL;

    if (list) {
        XSList *last;

        new_list = _x_slist_alloc();
        if (func) {
            new_list->data = func (list->data, user_data);
        } else {
            new_list->data = list->data;
        }

        last = new_list;
        list = list->next;

        while (list) {
            last->next = _x_slist_alloc();
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

XSList *x_slist_reverse(XSList *list)
{
    XSList *prev = NULL;

    while (list) {
        XSList *next = list->next;

        list->next = prev;
        prev = list;
        list = next;
    }

    return prev;
}

XSList *x_slist_nth(XSList *list, xuint n)
{
    while ((n-- > 0) && list) {
        list = list->next;
    }

    return list;
}

xpointer x_slist_nth_data(XSList *list, xuint n)
{
    while ((n-- > 0) && list) {
        list = list->next;
    }

    return list ? list->data : NULL;
}

XSList *x_slist_find(XSList *list, xconstpointer data)
{
    while (list) {
        if (list->data == data) {
            break;
        }

        list = list->next;
    }

    return list;
}

XSList *x_slist_find_custom(XSList *list, xconstpointer data, XCompareFunc func)
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

xint x_slist_position(XSList *list, XSList *llink)
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

xint x_slist_index(XSList *list, xconstpointer data)
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

XSList *x_slist_last(XSList *list)
{
    if (list) {
        while (list->next) {
            list = list->next;
        }
    }

    return list;
}

xuint x_slist_length(XSList *list)
{
    xuint length;

    length = 0;
    while (list) {
        length++;
        list = list->next;
    }

    return length;
}

void x_slist_foreach(XSList *list, XFunc func, xpointer user_data)
{
    while (list) {
        XSList *next = list->next;
        (*func)(list->data, user_data);
        list = next;
    }
}

static XSList *x_slist_insert_sorted_real(XSList *list, xpointer data, XFunc func, xpointer user_data)
{
    xint cmp;
    XSList *new_list;
    XSList *tmp_list = list;
    XSList *prev_list = NULL;

    x_return_val_if_fail(func != NULL, list);

    if (!list) {
        new_list = _x_slist_alloc();
        new_list->data = data;
        new_list->next = NULL;
        return new_list;
    }

    cmp = ((XCompareDataFunc)func)(data, tmp_list->data, user_data);

    while ((tmp_list->next) && (cmp > 0)) {
        prev_list = tmp_list;
        tmp_list = tmp_list->next;

        cmp = ((XCompareDataFunc)func)(data, tmp_list->data, user_data);
    }

    new_list = _x_slist_alloc();
    new_list->data = data;

    if ((!tmp_list->next) && (cmp > 0)) {
        tmp_list->next = new_list;
        new_list->next = NULL;
        return list;
    }

    if (prev_list) {
        prev_list->next = new_list;
        new_list->next = tmp_list;
        return list;
    } else {
        new_list->next = list;
        return new_list;
    }
}

XSList *x_slist_insert_sorted(XSList *list, xpointer data, XCompareFunc func)
{
    return x_slist_insert_sorted_real(list, data, (XFunc)func, NULL);
}

XSList *x_slist_insert_sorted_with_data(XSList *list, xpointer data, XCompareDataFunc func, xpointer user_data)
{
    return x_slist_insert_sorted_real(list, data, (XFunc)func, user_data);
}

static XSList *x_slist_sort_merge(XSList *l1, XSList *l2, XFunc compare_func, xpointer user_data)
{
    xint cmp;
    XSList list, *l;

    l = &list;

    while (l1 && l2) {
        cmp = ((XCompareDataFunc)compare_func)(l1->data, l2->data, user_data);
        if (cmp <= 0) {
            l = l->next = l1;
            l1 = l1->next;
        } else {
            l = l->next = l2;
            l2 = l2->next;
        }
    }

    l->next = l1 ? l1 : l2;
    return list.next;
}

static XSList *x_slist_sort_real(XSList *list, XFunc compare_func, xpointer user_data)
{
    XSList *l1, *l2;

    if (!list) {
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

    return x_slist_sort_merge(x_slist_sort_real(list, compare_func, user_data), x_slist_sort_real(l2, compare_func, user_data), compare_func, user_data);
}

XSList *x_slist_sort(XSList *list, XCompareFunc compare_func)
{
    return x_slist_sort_real(list, (XFunc)compare_func, NULL);
}

XSList *x_slist_sort_with_data(XSList *list, XCompareDataFunc compare_func, xpointer user_data)
{
    return x_slist_sort_real(list, (XFunc)compare_func, user_data);
}

void (x_clear_slist)(XSList **slist_ptr, XDestroyNotify destroy)
{
    XSList *slist;

    slist = *slist_ptr;
    if (slist) {
        *slist_ptr = NULL;

        if (destroy) {
            x_slist_free_full(slist, destroy);
        } else {
            x_slist_free(slist);
        }
    }
}
