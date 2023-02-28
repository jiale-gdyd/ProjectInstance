#ifndef __X_TRASH_STACK_H__
#define __X_TRASH_STACK_H__

#include "xutils.h"

X_BEGIN_DECLS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _XTrashStack XTrashStack XLIB_DEPRECATED_TYPE_IN_2_48;

struct _XTrashStack {
    XTrashStack *next;
} XLIB_DEPRECATED_TYPE_IN_2_48;

XLIB_DEPRECATED_IN_2_48
void x_trash_stack_push(XTrashStack **stack_p, xpointer data_p);

XLIB_DEPRECATED_IN_2_48
xpointer x_trash_stack_pop(XTrashStack **stack_p);

XLIB_DEPRECATED_IN_2_48
xpointer x_trash_stack_peek(XTrashStack **stack_p);

XLIB_DEPRECATED_IN_2_48
xuint x_trash_stack_height(XTrashStack **stack_p);

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
