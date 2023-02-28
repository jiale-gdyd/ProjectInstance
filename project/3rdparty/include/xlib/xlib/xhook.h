#ifndef __X_HOOK_H__
#define __X_HOOK_H__

#include "xmem.h"

X_BEGIN_DECLS

typedef struct _XHook XHook;
typedef struct _XHookList XHookList;

typedef void (*XHookFunc)(xpointer data);
typedef xboolean (*XHookCheckFunc)(xpointer data);
typedef void (*XHookMarshaller)(XHook *hook, xpointer marshal_data);
typedef xint (*XHookCompareFunc)(XHook *new_hook, XHook *sibling);
typedef xboolean (*XHookFindFunc)(XHook *hook, xpointer data);
typedef void (*XHookFinalizeFunc)(XHookList *hook_list, XHook *hook);
typedef xboolean (*XHookCheckMarshaller)(XHook *hook, xpointer marshal_data);

typedef enum {
    X_HOOK_FLAG_ACTIVE  = 1 << 0,
    X_HOOK_FLAG_IN_CALL = 1 << 1,
    X_HOOK_FLAG_MASK    = 0x0f
} XHookFlagMask;

#define X_HOOK_FLAG_USER_SHIFT      (4)

struct _XHookList {
    xulong            seq_id;
    xuint             hook_size : 16;
    xuint             is_setup : 1;
    XHook             *hooks;
    xpointer          dummy3;
    XHookFinalizeFunc finalize_hook;
    xpointer          dummy[2];
};

struct _XHook {
    xpointer       data;
    XHook          *next;
    XHook          *prev;
    xuint          ref_count;
    xulong         hook_id;
    xuint          flags;
    xpointer       func;
    XDestroyNotify destroy;
};

#define X_HOOK(hook)                ((XHook *)(hook))
#define X_HOOK_FLAGS(hook)          (X_HOOK(hook)->flags)
#define X_HOOK_ACTIVE(hook)         ((X_HOOK_FLAGS(hook) & X_HOOK_FLAG_ACTIVE) != 0)
#define X_HOOK_IN_CALL(hook)        ((X_HOOK_FLAGS(hook) & X_HOOK_FLAG_IN_CALL) != 0)
#define X_HOOK_IS_VALID(hook)       (X_HOOK(hook)->hook_id != 0 && (X_HOOK_FLAGS(hook) & X_HOOK_FLAG_ACTIVE))
#define X_HOOK_IS_UNLINKED(hook)    (X_HOOK(hook)->next == NULL && X_HOOK(hook)->prev == NULL && X_HOOK(hook)->hook_id == 0 && X_HOOK(hook)->ref_count == 0)

XLIB_AVAILABLE_IN_ALL
void x_hook_list_init(XHookList *hook_list, xuint hook_size);

XLIB_AVAILABLE_IN_ALL
void x_hook_list_clear(XHookList *hook_list);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_alloc(XHookList *hook_list);

XLIB_AVAILABLE_IN_ALL
void x_hook_free(XHookList *hook_list, XHook *hook);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_ref(XHookList *hook_list, XHook *hook);

XLIB_AVAILABLE_IN_ALL
void x_hook_unref(XHookList *hook_list, XHook *hook);

XLIB_AVAILABLE_IN_ALL
xboolean x_hook_destroy(XHookList *hook_list, xulong hook_id);

XLIB_AVAILABLE_IN_ALL
void x_hook_destroy_link(XHookList *hook_list, XHook *hook);

XLIB_AVAILABLE_IN_ALL
void x_hook_prepend(XHookList *hook_list, XHook *hook);

XLIB_AVAILABLE_IN_ALL
void x_hook_insert_before(XHookList *hook_list, XHook *sibling, XHook *hook);

XLIB_AVAILABLE_IN_ALL
void x_hook_insert_sorted(XHookList *hook_list, XHook *hook, XHookCompareFunc func);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_get(XHookList *hook_list, xulong hook_id);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_find(XHookList *hook_list, xboolean need_valids, XHookFindFunc func, xpointer data);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_find_data(XHookList *hook_list, xboolean need_valids, xpointer data);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_find_func(XHookList *hook_list, xboolean need_valids, xpointer func);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_find_func_data(XHookList *hook_list, xboolean need_valids, xpointer func, xpointer data);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_first_valid(XHookList *hook_list, xboolean may_be_in_call);

XLIB_AVAILABLE_IN_ALL
XHook *x_hook_next_valid(XHookList *hook_list, XHook *hook, xboolean may_be_in_call);

XLIB_AVAILABLE_IN_ALL
xint x_hook_compare_ids(XHook *new_hook, XHook *sibling);

#define x_hook_append(hook_list, hook)          x_hook_insert_before((hook_list), NULL, (hook))

XLIB_AVAILABLE_IN_ALL
void x_hook_list_invoke(XHookList *hook_list, xboolean may_recurse);

XLIB_AVAILABLE_IN_ALL
void x_hook_list_invoke_check(XHookList *hook_list, xboolean may_recurse);

XLIB_AVAILABLE_IN_ALL
void x_hook_list_marshal(XHookList *hook_list, xboolean may_recurse, XHookMarshaller marshaller, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_hook_list_marshal_check(XHookList *hook_list, xboolean may_recurse, XHookCheckMarshaller marshaller, xpointer marshal_data);

X_END_DECLS

#endif
