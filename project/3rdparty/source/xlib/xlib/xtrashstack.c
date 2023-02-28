#include <xlib/xlib/config.h>
#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif
#include <xlib/xlib/xtrashstack.h>

void x_trash_stack_push(XTrashStack **stack_p, xpointer data_p)
{
    XTrashStack *data = (XTrashStack *)data_p;

    data->next = *stack_p;
    *stack_p = data;
}

xpointer x_trash_stack_pop(XTrashStack **stack_p)
{
    XTrashStack *data;

    data = *stack_p;
    if (data) {
        *stack_p = data->next;
        data->next = NULL;
    }

    return data;
}

xpointer x_trash_stack_peek(XTrashStack **stack_p)
{
    XTrashStack *data;
    data = *stack_p;

    return data;
}

xuint x_trash_stack_height(XTrashStack **stack_p)
{
    xuint i = 0;
    XTrashStack *data;

    for (data = *stack_p; data; data = data->next) {
        i++;
    }

    return i;
}
