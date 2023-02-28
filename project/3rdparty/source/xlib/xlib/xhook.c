#include <xlib/xlib/config.h>
#include <xlib/xlib/xhook.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xtestutils.h>

static void default_finalize_hook(XHookList *hook_list, XHook *hook)
{
    XDestroyNotify destroy = hook->destroy;

    if (destroy) {
        hook->destroy = NULL;
        destroy(hook->data);
    }
}

void x_hook_list_init(XHookList *hook_list, xuint hook_size)
{
    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_size >= sizeof(XHook));

    hook_list->seq_id = 1;
    hook_list->hook_size = hook_size;
    hook_list->is_setup = TRUE;
    hook_list->hooks = NULL;
    hook_list->dummy3 = NULL;
    hook_list->finalize_hook = default_finalize_hook;
    hook_list->dummy[0] = NULL;
    hook_list->dummy[1] = NULL;
}

void x_hook_list_clear(XHookList *hook_list)
{
    x_return_if_fail(hook_list != NULL);

    if (hook_list->is_setup) {
        XHook *hook;

        hook_list->is_setup = FALSE;
        hook = hook_list->hooks;
        if (!hook) {

        } else {
            do {
                XHook *tmp;

                x_hook_ref(hook_list, hook);
                x_hook_destroy_link(hook_list, hook);
                tmp = hook->next;
                x_hook_unref(hook_list, hook);
                hook = tmp;
            } while (hook);
        }
    }
}

XHook *x_hook_alloc(XHookList *hook_list)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, NULL);
    x_return_val_if_fail(hook_list->is_setup, NULL);

    hook = (XHook *)x_slice_alloc0(hook_list->hook_size);
    hook->data = NULL;
    hook->next = NULL;
    hook->prev = NULL;
    hook->flags = X_HOOK_FLAG_ACTIVE;
    hook->ref_count = 0;
    hook->hook_id = 0;
    hook->func = NULL;
    hook->destroy = NULL;

    return hook;
}

void x_hook_free(XHookList *hook_list, XHook *hook)
{
    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);
    x_return_if_fail(hook != NULL);
    x_return_if_fail(X_HOOK_IS_UNLINKED(hook));
    x_return_if_fail(!X_HOOK_IN_CALL(hook));

    if(hook_list->finalize_hook != NULL) {
        hook_list->finalize_hook(hook_list, hook);
    }

    x_slice_free1(hook_list->hook_size, hook);
}

void x_hook_destroy_link(XHookList *hook_list, XHook *hook)
{
    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook != NULL);

    hook->flags &= ~X_HOOK_FLAG_ACTIVE;
    if (hook->hook_id) {
        hook->hook_id = 0;
        x_hook_unref(hook_list, hook);
    }
}

xboolean x_hook_destroy(XHookList *hook_list, xulong hook_id)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, FALSE);
    x_return_val_if_fail(hook_id > 0, FALSE);

    hook = x_hook_get(hook_list, hook_id);
    if (hook) {
        x_hook_destroy_link(hook_list, hook);
        return TRUE;
    }

    return FALSE;
}

void x_hook_unref(XHookList *hook_list, XHook *hook)
{
    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook != NULL);
    x_return_if_fail(hook->ref_count > 0);

    hook->ref_count--;
    if (!hook->ref_count) {
        x_return_if_fail(hook->hook_id == 0);
        x_return_if_fail(!X_HOOK_IN_CALL(hook));

        if (hook->prev) {
            hook->prev->next = hook->next;
        } else {
            hook_list->hooks = hook->next;
        }

        if (hook->next) {
            hook->next->prev = hook->prev;
            hook->next = NULL;
        }
        hook->prev = NULL;

        if (!hook_list->is_setup) {
            hook_list->is_setup = TRUE;
            x_hook_free(hook_list, hook);
            hook_list->is_setup = FALSE;

            if (!hook_list->hooks) {

            }
        } else {
            x_hook_free(hook_list, hook);
        }
    }
}

XHook *x_hook_ref(XHookList *hook_list, XHook *hook)
{
    x_return_val_if_fail(hook_list != NULL, NULL);
    x_return_val_if_fail(hook != NULL, NULL);
    x_return_val_if_fail(hook->ref_count > 0, NULL);

    hook->ref_count++;
    return hook;
}

void x_hook_prepend(XHookList *hook_list, XHook *hook)
{
    x_return_if_fail(hook_list != NULL);
    x_hook_insert_before(hook_list, hook_list->hooks, hook);
}

void x_hook_insert_before(XHookList *hook_list, XHook *sibling, XHook *hook)
{
    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);
    x_return_if_fail(hook != NULL);
    x_return_if_fail(X_HOOK_IS_UNLINKED(hook));
    x_return_if_fail(hook->ref_count == 0);

    hook->hook_id = hook_list->seq_id++;
    hook->ref_count = 1;

    if (sibling) {
        if (sibling->prev) {
            hook->prev = sibling->prev;
            hook->prev->next = hook;
            hook->next = sibling;
            sibling->prev = hook;
        } else {
            hook_list->hooks = hook;
            hook->next = sibling;
            sibling->prev = hook;
        }
    } else {
        if (hook_list->hooks) {
            sibling = hook_list->hooks;
            while (sibling->next) {
                sibling = sibling->next;
            }
            hook->prev = sibling;
            sibling->next = hook;
        } else {
            hook_list->hooks = hook;
        }
    }
}

void x_hook_list_invoke(XHookList *hook_list, xboolean may_recurse)
{
    XHook *hook;

    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);

    hook = x_hook_first_valid(hook_list, may_recurse);
    while (hook) {
        XHookFunc func;
        xboolean was_in_call;
        
        func = (XHookFunc)hook->func;
        
        was_in_call = X_HOOK_IN_CALL(hook);
        hook->flags |= X_HOOK_FLAG_IN_CALL;
        func (hook->data);
        if (!was_in_call) {
            hook->flags &= ~X_HOOK_FLAG_IN_CALL;
        }

        hook = x_hook_next_valid(hook_list, hook, may_recurse);
    }
}

void x_hook_list_invoke_check(XHookList *hook_list, xboolean may_recurse)
{
    XHook *hook;

    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);

    hook = x_hook_first_valid(hook_list, may_recurse);
    while (hook) {
        XHookCheckFunc func;
        xboolean was_in_call;
        xboolean need_destroy;
        
        func = (XHookCheckFunc)hook->func;
        
        was_in_call = X_HOOK_IN_CALL(hook);
        hook->flags |= X_HOOK_FLAG_IN_CALL;
        need_destroy = !func (hook->data);
        if (!was_in_call) {
            hook->flags &= ~X_HOOK_FLAG_IN_CALL;
        }

        if (need_destroy) {
            x_hook_destroy_link(hook_list, hook);
        }

        hook = x_hook_next_valid(hook_list, hook, may_recurse);
    }
}

void x_hook_list_marshal_check(XHookList *hook_list, xboolean may_recurse, XHookCheckMarshaller marshaller, xpointer data)
{
    XHook *hook;

    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);
    x_return_if_fail(marshaller != NULL);

    hook = x_hook_first_valid(hook_list, may_recurse);
    while (hook) {
        xboolean was_in_call;
        xboolean need_destroy;

        was_in_call = X_HOOK_IN_CALL(hook);
        hook->flags |= X_HOOK_FLAG_IN_CALL;
        need_destroy = !marshaller(hook, data);
        if (!was_in_call) {
            hook->flags &= ~X_HOOK_FLAG_IN_CALL;
        }

        if (need_destroy) {
            x_hook_destroy_link(hook_list, hook);
        }

        hook = x_hook_next_valid(hook_list, hook, may_recurse);
    }
}

void x_hook_list_marshal(XHookList *hook_list, xboolean may_recurse, XHookMarshaller marshaller, xpointer data)
{
    XHook *hook;

    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);
    x_return_if_fail(marshaller != NULL);

    hook = x_hook_first_valid(hook_list, may_recurse);
    while (hook) {
        xboolean was_in_call;

        was_in_call = X_HOOK_IN_CALL(hook);
        hook->flags |= X_HOOK_FLAG_IN_CALL;
        marshaller (hook, data);
        if (!was_in_call) {
            hook->flags &= ~X_HOOK_FLAG_IN_CALL;
        }

        hook = x_hook_next_valid(hook_list, hook, may_recurse);
    }
}

XHook *x_hook_first_valid(XHookList *hook_list, xboolean may_be_in_call)
{
    x_return_val_if_fail(hook_list != NULL, NULL);

    if (hook_list->is_setup) {
        XHook *hook;

        hook = hook_list->hooks;
        if (hook) {
            x_hook_ref(hook_list, hook);
            if (X_HOOK_IS_VALID(hook) && (may_be_in_call || !X_HOOK_IN_CALL(hook))) {
                return hook;
            } else {
                return x_hook_next_valid(hook_list, hook, may_be_in_call);
            }
        }
    }

    return NULL;
}

XHook *x_hook_next_valid(XHookList *hook_list, XHook *hook, xboolean may_be_in_call)
{
    XHook *ohook = hook;

    x_return_val_if_fail(hook_list != NULL, NULL);

    if (!hook) {
        return NULL;
    }

    hook = hook->next;
    while (hook) {
        if (X_HOOK_IS_VALID(hook) && (may_be_in_call || !X_HOOK_IN_CALL(hook))) {
            x_hook_ref(hook_list, hook);
            x_hook_unref(hook_list, ohook);
            return hook;
        }

        hook = hook->next;
    }
    x_hook_unref(hook_list, ohook);

    return NULL;
}

XHook *x_hook_get(XHookList *hook_list, xulong hook_id)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, NULL);
    x_return_val_if_fail(hook_id > 0, NULL);

    hook = hook_list->hooks;
    while (hook) {
        if (hook->hook_id == hook_id) {
            return hook;
        }

        hook = hook->next;
    }

    return NULL;
}

XHook *x_hook_find(XHookList *hook_list, xboolean need_valids, XHookFindFunc func, xpointer data)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, NULL);
    x_return_val_if_fail(func != NULL, NULL);

    hook = hook_list->hooks;
    while (hook) {
        XHook *tmp;

        if (!hook->hook_id) {
            hook = hook->next;
            continue;
        }

        x_hook_ref(hook_list, hook);
        if (func (hook, data) && hook->hook_id && (!need_valids || X_HOOK_ACTIVE(hook))) {
            x_hook_unref(hook_list, hook);
            return hook;
        }

        tmp = hook->next;
        x_hook_unref(hook_list, hook);
        hook = tmp;
    }

    return NULL;
}

XHook *x_hook_find_data(XHookList *hook_list, xboolean need_valids, xpointer data)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, NULL);

    hook = hook_list->hooks;
    while (hook) {
        if (hook->data == data && hook->hook_id && (!need_valids || X_HOOK_ACTIVE(hook))) {
            return hook;
        }

        hook = hook->next;
    }

    return NULL;
}

XHook *x_hook_find_func(XHookList *hook_list, xboolean need_valids, xpointer func)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, NULL);
    x_return_val_if_fail(func != NULL, NULL);

    hook = hook_list->hooks;
    while (hook) {
        if (hook->func == func && hook->hook_id && (!need_valids || X_HOOK_ACTIVE(hook))) {
            return hook;
        }

        hook = hook->next;
    }

    return NULL;
}

XHook *x_hook_find_func_data(XHookList *hook_list, xboolean need_valids, xpointer func, xpointer data)
{
    XHook *hook;

    x_return_val_if_fail(hook_list != NULL, NULL);
    x_return_val_if_fail(func != NULL, NULL);

    hook = hook_list->hooks;
    while (hook) {
        if (hook->data == data && hook->func == func && hook->hook_id && (!need_valids || X_HOOK_ACTIVE(hook))) {
            return hook;
        }

        hook = hook->next;
    }

    return NULL;
}

void x_hook_insert_sorted(XHookList *hook_list, XHook *hook, XHookCompareFunc func)
{
    XHook *sibling;

    x_return_if_fail(hook_list != NULL);
    x_return_if_fail(hook_list->is_setup);
    x_return_if_fail(hook != NULL);
    x_return_if_fail(X_HOOK_IS_UNLINKED (hook));
    x_return_if_fail(hook->func != NULL);
    x_return_if_fail(func != NULL);

    sibling = hook_list->hooks;
    while (sibling && !sibling->hook_id) {
        sibling = sibling->next;
    }

    while (sibling) {
        XHook *tmp;

        x_hook_ref(hook_list, sibling);
        if (func (hook, sibling) <= 0 && sibling->hook_id) {
            x_hook_unref(hook_list, sibling);
            break;
        }

        tmp = sibling->next;
        while (tmp && !tmp->hook_id) {
            tmp = tmp->next;
        }

        x_hook_unref(hook_list, sibling);
        sibling = tmp;
    }

    x_hook_insert_before(hook_list, sibling, hook);
}

xint x_hook_compare_ids(XHook *new_hook, XHook *sibling)
{
    if (new_hook->hook_id < sibling->hook_id) {
        return -1;
    } else if (new_hook->hook_id > sibling->hook_id) {
        return 1;
    }

    return 0;
}
