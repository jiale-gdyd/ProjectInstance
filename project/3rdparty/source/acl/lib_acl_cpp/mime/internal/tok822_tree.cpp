#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include "acl/lib_acl/stdlib/acl_mymalloc.h"

/* Global library. */

#include "tok822.hpp"

/* tok822_append - insert token list, return end of inserted list */

TOK822 *tok822_append(TOK822 *t1, TOK822 *t2)
{
    TOK822 *next = t1->next;

    t1->next = t2;
    t2->prev = t1;

    t2->owner = t1->owner;
    while (t2->next)
    (t2 = t2->next)->owner = t1->owner;

    t2->next = next;
    if (next)
    next->prev = t2;
    return (t2);
}

/* tok822_prepend - insert token list, return end of inserted list */

TOK822 *tok822_prepend(TOK822 *t1, TOK822 *t2)
{
    TOK822 *prev = t1->prev;

    if (prev)
    prev->next = t2;
    t2->prev = prev;

    t2->owner = t1->owner;
    while (t2->next)
    (t2 = t2->next)->owner = t1->owner;

    t2->next = t1;
    t1->prev = t2;
    return (t2);
}

/* tok822_cut_before - split list before token, return predecessor token */

TOK822 *tok822_cut_before(TOK822 *tp)
{
    TOK822 *prev = tp->prev;

    if (prev) {
    prev->next = 0;
    tp->prev = 0;
    }
    return (prev);
}

/* tok822_cut_after - split list after token, return successor token */

TOK822 *tok822_cut_after(TOK822 *tp)
{
    TOK822 *next = tp->next;

    if (next) {
    next->prev = 0;
    tp->next = 0;
    }
    return (next);
}

/* tok822_unlink - take token away from list, return predecessor token */

TOK822 *tok822_unlink(TOK822 *tp)
{
    TOK822 *prev = tp->prev;
    TOK822 *next = tp->next;

    if (prev)
    prev->next = next;
    if (next)
    next->prev = prev;
    tp->prev = tp->next = 0;
    return (prev);
}

/* tok822_sub_append - append sublist, return end of appended list */

TOK822 *tok822_sub_append(TOK822 *t1, TOK822 *t2)
{
    if (t1->head) {
    return (t1->tail = tok822_append(t1->tail, t2));
    } else {
    t1->head = t2;
    while (t2->next)
        (t2 = t2->next)->owner = t1;
    return (t1->tail = t2);
    }
}

/* tok822_sub_prepend - prepend sublist, return end of prepended list */

TOK822 *tok822_sub_prepend(TOK822 *t1, TOK822 *t2)
{
    TOK822 *tp;

    if (t1->head) {
    tp = tok822_prepend(t1->head, t2);
    t1->head = t2;
    return (tp);
    } else {
    t1->head = t2;
    while (t2->next)
        (t2 = t2->next)->owner = t1;
    return (t1->tail = t2);
    }
}

/* tok822_sub_keep_before - cut sublist, return tail of disconnected list */

TOK822 *tok822_sub_keep_before(TOK822 *t1, TOK822 *t2)
{
    TOK822 *tail = t1->tail;

    if ((t1->tail = tok822_cut_before(t2)) == 0)
    t1->head = 0;
    return (tail);
}

/* tok822_sub_keep_after - cut sublist, return head of disconnected list */

TOK822 *tok822_sub_keep_after(TOK822 *t1, TOK822 *t2)
{
    TOK822 *head = t1->head;

    if ((t1->head = tok822_cut_after(t2)) == 0)
        t1->tail = 0;
    return (head);
}

/* tok822_free_tree - destroy token tree */

TOK822 *tok822_free_tree(TOK822 *tp)
{
    if (tp) {
        if (tp->next)
            tok822_free_tree(tp->next);
        if (tp->head)
            tok822_free_tree(tp->head);
        tok822_free(tp);
    }
    return (0);
}

/* tok822_apply - apply action to specified tokens */

int     tok822_apply(TOK822 *tree, int type, TOK822_ACTION action)
{
    TOK822 *tp;
    int     result = 0;

    for (tp = tree; tp; tp = tp->next) {
        if (type == 0 || tp->type == type)
            if ((result = action(tp)) != 0)
                break;
    }
    return (result);
}

/* tok822_grep - list matching tokens */

TOK822 **tok822_grep(TOK822 *tree, int type)
{
    TOK822 **list;
    TOK822 *tp;
    int     count;

    for (count = 0, tp = tree; tp; tp = tp->next)
        if (type == 0 || tp->type == type)
            count++;

    list = (TOK822 **) acl_mymalloc(sizeof(*list) * (count + 1));

    for (count = 0, tp = tree; tp; tp = tp->next)
        if (type == 0 || tp->type == type)
            list[count++] = tp;

    list[count] = 0;
    return (list);
}

#endif // !defined(ACL_MIME_DISABLE)
