#ifndef __X_COMPLETION_H__
#define __X_COMPLETION_H__

#include "../xlist.h"

X_BEGIN_DECLS

typedef struct _XCompletion XCompletion;

typedef xchar *(*XCompletionFunc)(xpointer);
typedef xint (*XCompletionStrncmpFunc)(const xchar *s1, const xchar *s2, xsize n);

struct _XCompletion {
    XList                  *items;
    XCompletionFunc        func;
    xchar                  *prefix;
    XList                  *cache;
    XCompletionStrncmpFunc strncmp_func;
};

XLIB_DEPRECATED_IN_2_26
XCompletion *x_completion_new(XCompletionFunc func);

XLIB_DEPRECATED_IN_2_26
void x_completion_add_items(XCompletion *cmp, XList *items);

XLIB_DEPRECATED_IN_2_26
void x_completion_remove_items(XCompletion *cmp, XList *items);

XLIB_DEPRECATED_IN_2_26
void x_completion_clear_items(XCompletion *cmp);

XLIB_DEPRECATED_IN_2_26
XList *x_completion_complete(XCompletion *cmp, const xchar *prefix, xchar **new_prefix);

XLIB_DEPRECATED_IN_2_26
XList *x_completion_complete_utf8(XCompletion *cmp, const xchar *prefix, xchar **new_prefix);

XLIB_DEPRECATED_IN_2_26
void x_completion_set_compare(XCompletion *cmp, XCompletionStrncmpFunc strncmp_func);

XLIB_DEPRECATED_IN_2_26
void x_completion_free(XCompletion *cmp);

X_END_DECLS

#endif
