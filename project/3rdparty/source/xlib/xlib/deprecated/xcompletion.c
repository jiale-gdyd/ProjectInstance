#include <string.h>
#include <xlib/xlib/config.h>

#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/deprecated/xcompletion.h>

static void completion_check_cache(XCompletion *cmp, xchar **new_prefix);

XCompletion *x_completion_new(XCompletionFunc func)
{
    XCompletion *gcomp;

    gcomp = x_new(XCompletion, 1);
    gcomp->items = NULL;
    gcomp->cache = NULL;
    gcomp->prefix = NULL;
    gcomp->func = func;
    gcomp->strncmp_func = strncmp;

    return gcomp;
}

void x_completion_add_items(XCompletion *cmp, XList *items)
{
    XList *it;

    x_return_if_fail(cmp != NULL);

    if (cmp->cache) {
        x_list_free(cmp->cache);
        cmp->cache = NULL;
    }

    if (cmp->prefix) {
        x_free(cmp->prefix);
        cmp->prefix = NULL;
    }

    it = items;
    while (it) {
        cmp->items = x_list_prepend(cmp->items, it->data);
        it = it->next;
    }
}

void x_completion_remove_items(XCompletion *cmp, XList *items)
{
    XList *it;

    x_return_if_fail(cmp != NULL);

    it = items;
    while (cmp->items && it) {
        cmp->items = x_list_remove(cmp->items, it->data);
        it = it->next;
    }

    it = items;
    while (cmp->cache && it) {
        cmp->cache = x_list_remove(cmp->cache, it->data);
        it = it->next;
    }
}

void x_completion_clear_items(XCompletion *cmp)
{
    x_return_if_fail(cmp != NULL);

    x_list_free(cmp->items);
    cmp->items = NULL;

    x_list_free(cmp->cache);
    cmp->cache = NULL;

    x_free(cmp->prefix);
    cmp->prefix = NULL;
}

static void completion_check_cache(XCompletion *cmp, xchar **new_prefix)
{
    xsize i;
    xchar *s;
    xsize len;
    xsize plen;
    XList *list;
    xchar *postfix;

    if (!new_prefix) {
        return;
    }

    if (!cmp->cache) {
        *new_prefix = NULL;
        return;
    }

    len = strlen(cmp->prefix);
    list = cmp->cache;
    s = cmp->func ? cmp->func(list->data) : (xchar *)list->data;
    postfix = s + len;
    plen = strlen(postfix);
    list = list->next;

    while (list && plen) {
        s = cmp->func ? cmp->func(list->data) : (xchar *)list->data;
        s += len;

        for (i = 0; i < plen; ++i) {
            if (postfix[i] != s[i]) {
                break;
            }
        }

        plen = i;
        list = list->next;
    }

    *new_prefix = x_new0(xchar, len + plen + 1);
    strncpy(*new_prefix, cmp->prefix, len);
    strncpy(*new_prefix + len, postfix, plen);
}

XList *x_completion_complete_utf8(XCompletion *cmp, const xchar *prefix, xchar **new_prefix)
{
    XList *list;
    xchar *p, *q;

    list = x_completion_complete(cmp, prefix, new_prefix);

    if (new_prefix && *new_prefix) {
        p = *new_prefix + strlen(*new_prefix);
        q = x_utf8_find_prev_char(*new_prefix, p);

        switch (x_utf8_get_char_validated(q, p - q)) {
            case (xunichar)-2:
            case (xunichar)-1:
                *q = 0;
                break;

            default:;
        }
    }

    return list;
}

XList *x_completion_complete(XCompletion *cmp, const xchar *prefix, xchar **new_prefix)
{
    XList *list;
    xsize plen, len;
    xboolean done = FALSE;

    x_return_val_if_fail(cmp != NULL, NULL);
    x_return_val_if_fail(prefix != NULL, NULL);

    len = strlen(prefix);
    if (cmp->prefix && cmp->cache) {
        plen = strlen(cmp->prefix);
        if (plen <= len && ! cmp->strncmp_func(prefix, cmp->prefix, plen)) { 
            list = cmp->cache;
            while (list) {
                XList *next = list->next;

                if (cmp->strncmp_func(prefix, cmp->func ? cmp->func(list->data) : (xchar *)list->data, len)) {
                    cmp->cache = x_list_delete_link(cmp->cache, list);
                }

                list = next;
            }

            done = TRUE;
        }
    }

    if (!done) {
        x_list_free(cmp->cache);
        cmp->cache = NULL;
        list = cmp->items;

        while (*prefix && list) {
            if (!cmp->strncmp_func(prefix, cmp->func ? cmp->func(list->data) : (xchar *)list->data, len)) {
                cmp->cache = x_list_prepend(cmp->cache, list->data);
            }

            list = list->next;
        }
    }

    if (cmp->prefix) {
        x_free(cmp->prefix);
        cmp->prefix = NULL;
    }

    if (cmp->cache) {
        cmp->prefix = x_strdup(prefix);
    }

    completion_check_cache(cmp, new_prefix);
    return *prefix ? cmp->cache : cmp->items;
}

void x_completion_free(XCompletion *cmp)
{
    x_return_if_fail(cmp != NULL);

    x_completion_clear_items(cmp);
    x_free(cmp);
}

void x_completion_set_compare(XCompletion *cmp, XCompletionStrncmpFunc strncmp_func)
{
    cmp->strncmp_func = strncmp_func;
}
