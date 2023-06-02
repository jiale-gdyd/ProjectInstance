#include <string.h>
#include <signal.h>

#include <xlib/xlib/config.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xsignal.h>
#include <xlib/xobj/xobject.h>
#include <xlib/xobj/xvaluetypes.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xlib/xbsearcharray.h>
#include <xlib/xobj/xobject_trace.h>
#include <xlib/xobj/xvaluecollector.h>

#define REPORT_BUG                          "please report occurrence circumstances to https://gitlab.gnome.org/GNOME/glib/issues/new"

typedef struct _Handler Handler;
typedef struct _Emission Emission;
typedef struct _SignalKey SignalKey;
typedef struct _SignalNode SignalNode;
typedef struct _HandlerList HandlerList;
typedef struct _HandlerMatch HandlerMatch;

typedef enum {
    EMISSION_STOP,
    EMISSION_RUN,
    EMISSION_HOOK,
    EMISSION_RESTART
} EmissionState;

static inline xuint signal_id_lookup(const xchar *name, XType itype);
static void signal_destroy_R(SignalNode *signal_node);
static inline HandlerList *handler_list_ensure(xuint signal_id, xpointer instance);
static inline HandlerList *handler_list_lookup(xuint signal_id, xpointer instance);
static inline Handler *handler_new(xuint signal_id, xpointer instance, xboolean after);
static void handler_insert(xuint signal_id, xpointer instance, Handler *handler);
static Handler *handler_lookup(xpointer instance, xulong handler_id, XClosure *closure, xuint *signal_id_p);
static inline HandlerMatch *handler_match_prepend(HandlerMatch *list, Handler *handler, xuint signal_id);
static inline HandlerMatch *handler_match_free1_R(HandlerMatch *node, xpointer instance);
static HandlerMatch *handlers_find(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data, xboolean one_and_only);
static inline void handler_ref(Handler *handler);
static inline void handler_unref_R(xuint signal_id, xpointer instance, Handler *handler);
static xint handler_lists_cmp(xconstpointer node1, xconstpointer node2);
static inline void emission_push(Emission *emission);
static inline void emission_pop(Emission *emission);
static inline Emission *emission_find(xuint signal_id, XQuark detail, xpointer instance);
static xint class_closures_cmp(xconstpointer node1, xconstpointer node2);
static xint signal_key_cmp(xconstpointer node1, xconstpointer node2);
static xboolean signal_emit_unlocked_R(SignalNode *node, XQuark detail, xpointer instance, XValue *return_value, const XValue *instance_and_params);
static void add_invalid_closure_notify(Handler *handler, xpointer instance);
static void remove_invalid_closure_notify(Handler *handler, xpointer instance);
static void invalid_closure_notify(xpointer data, XClosure *closure);
static const xchar *type_debug_name(XType type);
static void node_check_deprecated(const SignalNode *node);
static void node_update_single_va_closure(SignalNode *node);

typedef struct {
    XSignalAccumulator func;
    xpointer           data;
} SignalAccumulator;

typedef struct {
    XHook  hook;
    XQuark detail;
} SignalHook;

#define SIGNAL_HOOK(hook)                   ((SignalHook *)(hook))

struct _SignalNode {
    xuint                signal_id;
    XType                itype;
    const xchar          *name;
    xuint                destroyed : 1;
    xuint                flags : 9;
    xuint                n_params : 8;
    xuint                single_va_closure_is_valid : 1;
    xuint                single_va_closure_is_after : 1;
    XType                *param_types;
    XType                return_type;
    XBSearchArray        *class_closure_bsa;
    SignalAccumulator    *accumulator;
    XSignalCMarshaller   c_marshaller;
    XSignalCVaMarshaller va_marshaller;
    XHookList            *emission_hooks;
    XClosure             *single_va_closure;
};

#define SINGLE_VA_CLOSURE_EMPTY_MAGIC       XINT_TO_POINTER(1)

struct _SignalKey {
    XType  itype;
    XQuark quark;
    xuint  signal_id;
};

struct _Emission {
    Emission              *next;
    xpointer              instance;
    XSignalInvocationHint ihint;
    EmissionState         state;
    XType                 chain_type;
};

struct _HandlerList {
    xuint   signal_id;
    Handler *handlers;
    Handler *tail_before;
    Handler *tail_after;
};

struct _Handler {
    xulong   sequential_number;
    Handler  *next;
    Handler  *prev;
    XQuark   detail;
    xuint    signal_id;
    xuint    ref_count;
    xuint    block_count : 16;
#define HANDLER_MAX_BLOCK_COUNT     (1 << 16)
    xuint    after : 1;
    xuint    has_invalid_closure_notify : 1;
    XClosure *closure;
    xpointer instance;
};

struct _HandlerMatch {
    Handler      *handler;
    HandlerMatch *next;
    xuint        signal_id;
};

typedef struct {
    XType     instance_type;
    XClosure  *closure;
} ClassClosure;

static XBSearchArray *x_signal_key_bsa = NULL;

static const XBSearchConfig x_signal_key_bconfig = {
    sizeof(SignalKey),
    signal_key_cmp,
    X_BSEARCH_ARRAY_ALIGN_POWER2,
};

static XBSearchConfig x_signal_hlbsa_bconfig = {
    sizeof(HandlerList),
    handler_lists_cmp,
    0,
};

static XBSearchConfig x_class_closure_bconfig = {
    sizeof(ClassClosure),
    class_closures_cmp,
    0,
};

static Emission *x_emissions = NULL;
static XHashTable *x_handlers = NULL;
static xulong x_handler_sequential_number = 1;
static XHashTable *x_handler_list_bsa_ht = NULL;

X_LOCK_DEFINE_STATIC(x_signal_mutex);
#define SIGNAL_LOCK()                       X_LOCK(x_signal_mutex)
#define SIGNAL_UNLOCK()                     X_UNLOCK(x_signal_mutex)

static xuint x_n_signal_nodes = 0;
static SignalNode **x_signal_nodes = NULL;

static inline SignalNode *LOOKUP_SIGNAL_NODE(xuint signal_id)
{
    if (signal_id < x_n_signal_nodes) {
        return x_signal_nodes[signal_id];
    } else {
        return NULL;
    }
}

static void canonicalize_key(xchar *key)
{
    xchar *p;

    for (p = key; *p != 0; p++) {
        xchar c = *p;
        if (c == '_') {
            *p = '-';
        }
    }
}

static xboolean is_canonical(const xchar *key)
{
    return (strchr(key, '_') == NULL);
}

xboolean x_signal_is_valid_name(const xchar *name)
{
    if (x_str_equal(name, "-gtk-private-changed")) {
        return TRUE;
    }

    return x_param_spec_is_valid_name(name);
}

static inline xuint signal_id_lookup(const xchar *name, XType itype)
{
    XQuark quark;
    SignalKey key;
    xuint n_ifaces;
    XType *ifaces, type = itype;

    quark = x_quark_try_string(name);
    key.quark = quark;

    do {
        SignalKey *signal_key;

        key.itype = type;
        signal_key = (SignalKey *)x_bsearch_array_lookup(x_signal_key_bsa, &x_signal_key_bconfig, &key);

        if (signal_key) {
            return signal_key->signal_id;
        }

        type = x_type_parent(type);
    } while (type);

    ifaces = x_type_interfaces(itype, &n_ifaces);
    while (n_ifaces--) {
        SignalKey *signal_key;

        key.itype = ifaces[n_ifaces];
        signal_key = (SignalKey *)x_bsearch_array_lookup(x_signal_key_bsa, &x_signal_key_bconfig, &key);
        if (signal_key) {
            x_free(ifaces);
            return signal_key->signal_id;
        }
    }
    x_free(ifaces);

    if (!is_canonical(name)) {
        xuint signal_id;
        xchar *name_copy = x_strdup(name);
        canonicalize_key(name_copy);

        signal_id = signal_id_lookup(name_copy, itype);
        x_free(name_copy);

        return signal_id;
    }

    return 0;
}

static xint class_closures_cmp(xconstpointer node1, xconstpointer node2)
{
    const ClassClosure *c1 = (const ClassClosure *)node1, *c2 = (const ClassClosure *)node2;
    return X_BSEARCH_ARRAY_CMP(c1->instance_type, c2->instance_type);
}

static xint handler_lists_cmp(xconstpointer node1, xconstpointer node2)
{
    const HandlerList *hlist1 = (const HandlerList *)node1, *hlist2 = (const HandlerList *)node2;
    return X_BSEARCH_ARRAY_CMP(hlist1->signal_id, hlist2->signal_id);
}

static inline HandlerList *handler_list_ensure(xuint signal_id, xpointer instance)
{
    HandlerList key;
    XBSearchArray *hlbsa = (XBSearchArray *)x_hash_table_lookup(x_handler_list_bsa_ht, instance);

    key.signal_id = signal_id;
    key.handlers = NULL;
    key.tail_before = NULL;
    key.tail_after = NULL;
    if (!hlbsa) {
        hlbsa = x_bsearch_array_create(&x_signal_hlbsa_bconfig);
        hlbsa = x_bsearch_array_insert(hlbsa, &x_signal_hlbsa_bconfig, &key);
        x_hash_table_insert(x_handler_list_bsa_ht, instance, hlbsa);
    } else {
        XBSearchArray *o = hlbsa;

        hlbsa = x_bsearch_array_insert(o, &x_signal_hlbsa_bconfig, &key);
        if (hlbsa != o) {
            x_hash_table_insert(x_handler_list_bsa_ht, instance, hlbsa);
        }
    }

    return (HandlerList *)x_bsearch_array_lookup(hlbsa, &x_signal_hlbsa_bconfig, &key);
}

static inline HandlerList *handler_list_lookup(xuint signal_id, xpointer instance)
{
    HandlerList key;
    XBSearchArray *hlbsa = (XBSearchArray *)x_hash_table_lookup(x_handler_list_bsa_ht, instance);

    key.signal_id = signal_id;
    return hlbsa ? (HandlerList *)x_bsearch_array_lookup(hlbsa, &x_signal_hlbsa_bconfig, &key) : NULL;
}

static xuint handler_hash(xconstpointer key)
{
    return (xuint)((Handler *)key)->sequential_number;
}

static xboolean handler_equal(xconstpointer a, xconstpointer b)
{
    Handler *ha = (Handler *)a;
    Handler *hb = (Handler *)b;
    return (ha->sequential_number == hb->sequential_number) && (ha->instance  == hb->instance);
}

static Handler *handler_lookup(xpointer instance, xulong handler_id, XClosure *closure, xuint *signal_id_p)
{
    XBSearchArray *hlbsa;

    if (handler_id) {
        Handler key;
        key.sequential_number = handler_id;
        key.instance = instance;
        return (Handler *)x_hash_table_lookup(x_handlers, &key);
    }

    hlbsa = (XBSearchArray *)x_hash_table_lookup(x_handler_list_bsa_ht, instance);
    if (hlbsa) {
        xuint i;

        for (i = 0; i < hlbsa->n_nodes; i++) {
            Handler *handler;
            HandlerList *hlist = (HandlerList *)x_bsearch_array_get_nth(hlbsa, &x_signal_hlbsa_bconfig, i);

            for (handler = hlist->handlers; handler; handler = handler->next) {
                if (closure ? (handler->closure == closure) : (handler->sequential_number == handler_id)) {
                    if (signal_id_p) {
                        *signal_id_p = hlist->signal_id;
                    }

                    return handler;
                }
            }
        }
    }

    return NULL;
}

static inline HandlerMatch *handler_match_prepend(HandlerMatch *list, Handler *handler, xuint signal_id)
{
    HandlerMatch *node;

    node = x_slice_new(HandlerMatch);
    node->handler = handler;
    node->next = list;
    node->signal_id = signal_id;
    handler_ref(handler);

    return node;
}

static inline HandlerMatch *handler_match_free1_R(HandlerMatch *node, xpointer instance)
{
    HandlerMatch *next = node->next;

    handler_unref_R(node->signal_id, instance, node->handler);
    x_slice_free(HandlerMatch, node);

    return next;
}

static HandlerMatch *handlers_find(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data, xboolean one_and_only)
{
    HandlerMatch *mlist = NULL;

    if (mask & X_SIGNAL_MATCH_ID) {
        Handler *handler;
        SignalNode *node = NULL;
        HandlerList *hlist = handler_list_lookup(signal_id, instance);

        if (mask & X_SIGNAL_MATCH_FUNC) {
            node = LOOKUP_SIGNAL_NODE(signal_id);
            if (!node || !node->c_marshaller) {
                return NULL;
            }
        }

        mask = (XSignalMatchType)~mask;
        for (handler = hlist ? hlist->handlers : NULL; handler; handler = handler->next) {
            if (handler->sequential_number
                && ((mask & X_SIGNAL_MATCH_DETAIL) || handler->detail == detail)
                && ((mask & X_SIGNAL_MATCH_CLOSURE) || handler->closure == closure)
                && ((mask & X_SIGNAL_MATCH_DATA) || handler->closure->data == data)
                && ((mask & X_SIGNAL_MATCH_UNBLOCKED) || handler->block_count == 0)
                && ((mask & X_SIGNAL_MATCH_FUNC) || (handler->closure->marshal == node->c_marshaller
                && X_REAL_CLOSURE(handler->closure)->meta_marshal == NULL
                && ((XCClosure *)handler->closure)->callback == func)))
            {
                mlist = handler_match_prepend(mlist, handler, signal_id);
                if (one_and_only) {
                    return mlist;
                }
            }
        }
    } else {
        XBSearchArray *hlbsa = (XBSearchArray *)x_hash_table_lookup(x_handler_list_bsa_ht, instance);

        mask = (XSignalMatchType)~mask;
        if (hlbsa) {
            xuint i;

            for (i = 0; i < hlbsa->n_nodes; i++) {
                Handler *handler;
                SignalNode *node = NULL;
                HandlerList *hlist = (HandlerList *)x_bsearch_array_get_nth(hlbsa, &x_signal_hlbsa_bconfig, i);

                if (!(mask & X_SIGNAL_MATCH_FUNC)) {
                    node = LOOKUP_SIGNAL_NODE(hlist->signal_id);
                    if (!node->c_marshaller) {
                        continue;
                    }
                }

                for (handler = hlist->handlers; handler; handler = handler->next) {
                    if (handler->sequential_number
                        && ((mask & X_SIGNAL_MATCH_DETAIL) || handler->detail == detail)
                        && ((mask & X_SIGNAL_MATCH_CLOSURE) || handler->closure == closure)
                        && ((mask & X_SIGNAL_MATCH_DATA) || handler->closure->data == data)
                        && ((mask & X_SIGNAL_MATCH_UNBLOCKED) || handler->block_count == 0)
                        && ((mask & X_SIGNAL_MATCH_FUNC) || (handler->closure->marshal == node->c_marshaller
                        && X_REAL_CLOSURE(handler->closure)->meta_marshal == NULL
                        && ((XCClosure *)handler->closure)->callback == func)))
                    {
                        mlist = handler_match_prepend(mlist, handler, hlist->signal_id);
                        if (one_and_only) {
                            return mlist;
                        }
                    }
                }
            }
        }
    }

    return mlist;
}

static inline Handler *handler_new(xuint signal_id, xpointer instance, xboolean after)
{
    Handler *handler = x_slice_new(Handler);

    if (x_handler_sequential_number < 1) {
        x_error(X_STRLOC ": handler id overflow, %s", REPORT_BUG);
    }

    handler->sequential_number = x_handler_sequential_number++;
    handler->prev = NULL;
    handler->next = NULL;
    handler->detail = 0;
    handler->signal_id = signal_id;
    handler->instance = instance;
    handler->ref_count = 1;
    handler->block_count = 0;
    handler->after = after != FALSE;
    handler->closure = NULL;
    handler->has_invalid_closure_notify = 0;

    x_hash_table_add(x_handlers, handler);
    return handler;
}

static inline void handler_ref(Handler *handler)
{
    x_return_if_fail(handler->ref_count > 0);
    handler->ref_count++;
}

static inline void handler_unref_R(xuint signal_id, xpointer instance, Handler *handler)
{
    x_return_if_fail(handler->ref_count > 0);

    handler->ref_count--;

    if (X_UNLIKELY(handler->ref_count == 0)) {
        HandlerList *hlist = NULL;

        if (handler->next) {
            handler->next->prev = handler->prev;
        }

        if (handler->prev) {
            handler->prev->next = handler->next;
        } else {
            hlist = handler_list_lookup(signal_id, instance);
            x_assert(hlist != NULL);
            hlist->handlers = handler->next;
        }

        if (instance) {
            if (!handler->after && (!handler->next || handler->next->after)) {
                if (!hlist) {
                    hlist = handler_list_lookup(signal_id, instance);
                }

                if (hlist) {
                    x_assert(hlist->tail_before == handler);
                    hlist->tail_before = handler->prev;
                }
            }

            if (!handler->next) {
                if (!hlist) {
                    hlist = handler_list_lookup(signal_id, instance);
                }

                if (hlist) {
                    x_assert(hlist->tail_after == handler);
                    hlist->tail_after = handler->prev;
                }
            }
        }

        SIGNAL_UNLOCK();
        x_closure_unref(handler->closure);
        SIGNAL_LOCK();
        x_slice_free(Handler, handler);
    }
}

static void handler_insert(xuint signal_id, xpointer instance, Handler *handler)
{
    HandlerList *hlist;

    x_assert(handler->prev == NULL && handler->next == NULL);

    hlist = handler_list_ensure(signal_id, instance);
    if (!hlist->handlers) {
        hlist->handlers = handler;
        if (!handler->after) {
            hlist->tail_before = handler;
        }
    } else if (handler->after) {
        handler->prev = hlist->tail_after;
        hlist->tail_after->next = handler;
    } else {
        if (hlist->tail_before) {
            handler->next = hlist->tail_before->next;
            if (handler->next) {
                handler->next->prev = handler;
            }

            handler->prev = hlist->tail_before;
            hlist->tail_before->next = handler;
        } else {
            handler->next = hlist->handlers;
            if (handler->next) {
                handler->next->prev = handler;
            }

            hlist->handlers = handler;
        }

        hlist->tail_before = handler;
    }

    if (!handler->next) {
        hlist->tail_after = handler;
    }
}

static void node_update_single_va_closure(SignalNode *node)
{
    XClosure *closure = NULL;
    xboolean is_after = FALSE;

    if (X_TYPE_IS_OBJECT(node->itype) && (node->flags & (X_SIGNAL_MUST_COLLECT)) == 0 && (node->emission_hooks == NULL || node->emission_hooks->hooks == NULL)) {
        ClassClosure *cc;
        XSignalFlags run_type;
        XBSearchArray *bsa = (XBSearchArray *)node->class_closure_bsa;

        if (bsa == NULL || bsa->n_nodes == 0) {
            closure = (XClosure *)SINGLE_VA_CLOSURE_EMPTY_MAGIC;
        } else if (bsa->n_nodes == 1) {
            cc = (ClassClosure *)x_bsearch_array_get_nth(bsa, &x_class_closure_bconfig, 0);
            if (cc->instance_type == 0) {
                run_type = (XSignalFlags)(node->flags & (X_SIGNAL_RUN_FIRST | X_SIGNAL_RUN_LAST | X_SIGNAL_RUN_CLEANUP));
                if (run_type == X_SIGNAL_RUN_FIRST || run_type == X_SIGNAL_RUN_LAST) {
                    closure = cc->closure;
                    is_after = (run_type == X_SIGNAL_RUN_LAST);
                }
            }
        }
    }

    node->single_va_closure_is_valid = TRUE;
    node->single_va_closure = closure;
    node->single_va_closure_is_after = is_after;
}

static inline void emission_push(Emission *emission)
{
    emission->next = x_emissions;
    x_emissions = emission;
}

static inline void emission_pop(Emission  *emission)
{
    Emission *node, *last = NULL;

    for (node = x_emissions; node; last = node, node = last->next) {
        if (node == emission) {
            if (last) {
                last->next = node->next;
            } else {
                x_emissions = node->next;
            }

            return;
        }
    }

    x_assert_not_reached();
}

static inline Emission *emission_find(xuint signal_id, XQuark detail, xpointer instance)
{
    Emission *emission;

    for (emission = x_emissions; emission; emission = emission->next) {
        if (emission->instance == instance && emission->ihint.signal_id == signal_id && emission->ihint.detail == detail) {
            return emission;
        }
    }

    return NULL;
}

static inline Emission *emission_find_innermost(xpointer instance)
{
    Emission *emission;

    for (emission = x_emissions; emission; emission = emission->next) {
        if (emission->instance == instance) {
            return emission;
        }
    }

    return NULL;
}

static xint signal_key_cmp(xconstpointer node1, xconstpointer node2)
{
    const SignalKey *key1 = (const SignalKey *)node1, *key2 = (const SignalKey *)node2;

    if (key1->itype == key2->itype) {
        return X_BSEARCH_ARRAY_CMP(key1->quark, key2->quark);
    } else {
        return X_BSEARCH_ARRAY_CMP(key1->itype, key2->itype);
    }
}

void _x_signal_init (void)
{
    SIGNAL_LOCK();
    if (!x_n_signal_nodes) {
        x_handler_list_bsa_ht = x_hash_table_new(x_direct_hash, NULL);
        x_signal_key_bsa = x_bsearch_array_create(&x_signal_key_bconfig);

        x_n_signal_nodes = 1;
        x_signal_nodes = x_renew(SignalNode *, x_signal_nodes, x_n_signal_nodes);
        x_signal_nodes[0] = NULL;
        x_handlers = x_hash_table_new(handler_hash, handler_equal);
    }
    SIGNAL_UNLOCK();
}

void _x_signals_destroy(XType itype)
{
    xuint i;

    SIGNAL_LOCK();
    for (i = 1; i < x_n_signal_nodes; i++) {
        SignalNode *node = x_signal_nodes[i];

        if (node->itype == itype) {
            if (node->destroyed) {
                x_critical(X_STRLOC ": signal \"%s\" of type '%s' already destroyed", node->name, type_debug_name(node->itype));
            } else {
                signal_destroy_R(node);
            }
        }
    }
    SIGNAL_UNLOCK();
}

void x_signal_stop_emission(xpointer instance, xuint signal_id, XQuark detail)
{
    SignalNode *node;

    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(signal_id > 0);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (node && detail && !(node->flags & X_SIGNAL_DETAILED)) {
        x_critical("%s: signal id '%u' does not support detail (%u)", X_STRLOC, signal_id, detail);
        SIGNAL_UNLOCK();
        return;
    }

    if (node && x_type_is_a(X_TYPE_FROM_INSTANCE(instance), node->itype)) {
        Emission *emission = emission_find(signal_id, detail, instance);

        if (emission) {
            if (emission->state == EMISSION_HOOK) {
                x_critical(X_STRLOC ": emission of signal \"%s\" for instance '%p' cannot be stopped from emission hook", node->name, instance);
            } else if (emission->state == EMISSION_RUN) {
                emission->state = EMISSION_STOP;
            }
        } else {
            x_critical(X_STRLOC ": no emission of signal \"%s\" to stop for instance '%p'", node->name, instance);
        }
    } else {
        x_critical("%s: signal id '%u' is invalid for instance '%p'", X_STRLOC, signal_id, instance);
    }
    SIGNAL_UNLOCK();
}

static void signal_finalize_hook(XHookList *hook_list, XHook *hook)
{
    XDestroyNotify destroy = hook->destroy;

    if (destroy) {
        hook->destroy = NULL;
        SIGNAL_UNLOCK();
        destroy (hook->data);
        SIGNAL_LOCK();
    }
}

xulong x_signal_add_emission_hook(xuint signal_id, XQuark detail, XSignalEmissionHook hook_func, xpointer hook_data, XDestroyNotify data_destroy)
{
    XHook *hook;
    SignalNode *node;
    SignalHook *signal_hook;
    static xulong seq_hook_id = 1;

    x_return_val_if_fail(signal_id > 0, 0);
    x_return_val_if_fail(hook_func != NULL, 0);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (!node || node->destroyed) {
        x_critical("%s: invalid signal id '%u'", X_STRLOC, signal_id);
        SIGNAL_UNLOCK();
        return 0;
    }

    if (node->flags & X_SIGNAL_NO_HOOKS)  {
        x_critical("%s: signal id '%u' does not support emission hooks (X_SIGNAL_NO_HOOKS flag set)", X_STRLOC, signal_id);
        SIGNAL_UNLOCK();
        return 0;
    }

    if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
        x_critical("%s: signal id '%u' does not support detail (%u)", X_STRLOC, signal_id, detail);
        SIGNAL_UNLOCK();
        return 0;
    }

    node->single_va_closure_is_valid = FALSE;
    if (!node->emission_hooks) {
        node->emission_hooks = x_new(XHookList, 1);
        x_hook_list_init(node->emission_hooks, sizeof(SignalHook));
        node->emission_hooks->finalize_hook = signal_finalize_hook;
    }

    node_check_deprecated (node);

    hook = x_hook_alloc(node->emission_hooks);
    hook->data = hook_data;
    hook->func = (xpointer) hook_func;
    hook->destroy = data_destroy;
    signal_hook = SIGNAL_HOOK(hook);
    signal_hook->detail = detail;
    node->emission_hooks->seq_id = seq_hook_id;
    x_hook_append(node->emission_hooks, hook);
    seq_hook_id = node->emission_hooks->seq_id;

    SIGNAL_UNLOCK();

    return hook->hook_id;
}

void x_signal_remove_emission_hook(xuint signal_id, xulong hook_id)
{
    SignalNode *node;

    x_return_if_fail(signal_id > 0);
    x_return_if_fail(hook_id > 0);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (!node || node->destroyed) {
        x_critical("%s: invalid signal id '%u'", X_STRLOC, signal_id);
        goto out;
    } else if (!node->emission_hooks || !x_hook_destroy(node->emission_hooks, hook_id)) {
        x_critical("%s: signal \"%s\" had no hook (%lu) to remove", X_STRLOC, node->name, hook_id);
    }
    node->single_va_closure_is_valid = FALSE;

out:
    SIGNAL_UNLOCK();
}

static inline xuint signal_parse_name(const xchar *name, XType itype, XQuark *detail_p, xboolean force_quark)
{
    xuint signal_id;
    const xchar *colon = strchr(name, ':');

    if (!colon) {
        signal_id = signal_id_lookup(name, itype);
        if (signal_id && detail_p) {
            *detail_p = 0;
        }
    } else if (colon[1] == ':') {
        xchar buffer[32];
        xuint l = colon - name;

        if (colon[2] == '\0') {
            return 0;
        }

        if (l < 32) {
            memcpy (buffer, name, l);
            buffer[l] = 0;
            signal_id = signal_id_lookup(buffer, itype);
        } else{
            xchar *signal = x_new(xchar, l + 1);

            memcpy(signal, name, l);
            signal[l] = 0;
            signal_id = signal_id_lookup(signal, itype);
            x_free(signal);
        }

        if (signal_id && detail_p) {
            *detail_p = (force_quark ? x_quark_from_string : x_quark_try_string)(colon + 2);
        }
    } else {
        signal_id = 0;
    }

    return signal_id;
}

xboolean x_signal_parse_name(const xchar *detailed_signal, XType itype, xuint *signal_id_p, XQuark *detail_p, xboolean force_detail_quark)
{
    xuint signal_id;
    SignalNode *node;
    XQuark detail = 0;

    x_return_val_if_fail(detailed_signal != NULL, FALSE);
    x_return_val_if_fail(X_TYPE_IS_INSTANTIATABLE(itype) || X_TYPE_IS_INTERFACE(itype), FALSE);

    SIGNAL_LOCK();
    signal_id = signal_parse_name(detailed_signal, itype, &detail, force_detail_quark);

    node = signal_id ? LOOKUP_SIGNAL_NODE(signal_id) : NULL;

    if (!node || node->destroyed || (detail && !(node->flags & X_SIGNAL_DETAILED))) {
        SIGNAL_UNLOCK();
        return FALSE;
    }

    SIGNAL_UNLOCK();

    if (signal_id_p) {
        *signal_id_p = signal_id;
    }

    if (detail_p) {
        *detail_p = detail;
    }

    return TRUE;
}

void x_signal_stop_emission_by_name(xpointer instance, const xchar *detailed_signal)
{
    XType itype;
    xuint signal_id;
    XQuark detail = 0;

    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(detailed_signal != NULL);

    SIGNAL_LOCK();
    itype = X_TYPE_FROM_INSTANCE(instance);
    signal_id = signal_parse_name(detailed_signal, itype, &detail, TRUE);

    if (signal_id) {
        SignalNode *node = LOOKUP_SIGNAL_NODE(signal_id);

        if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
            x_critical("%s: signal '%s' does not support details", X_STRLOC, detailed_signal);
        } else if (!x_type_is_a(itype, node->itype)) {
            x_critical("%s: signal '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
        } else {
            Emission *emission = emission_find(signal_id, detail, instance);
            if (emission) {
                if (emission->state == EMISSION_HOOK) {
                    x_critical(X_STRLOC ": emission of signal \"%s\" for instance '%p' cannot be stopped from emission hook", node->name, instance);
                } else if (emission->state == EMISSION_RUN) {
                    emission->state = EMISSION_STOP;
                }
            } else {
                x_critical(X_STRLOC ": no emission of signal \"%s\" to stop for instance '%p'", node->name, instance);
            }
        }
    } else {
        x_critical("%s: signal '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
    }
    SIGNAL_UNLOCK();
}

xuint x_signal_lookup(const xchar *name, XType itype)
{
    xuint signal_id;

    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(X_TYPE_IS_INSTANTIATABLE(itype) || X_TYPE_IS_INTERFACE(itype), 0);

    SIGNAL_LOCK();
    signal_id = signal_id_lookup(name, itype);
    SIGNAL_UNLOCK();

    if (!signal_id) {
        if (!x_type_name(itype)) {
            x_critical(X_STRLOC ": unable to look up signal \"%s\" for invalid type id '%" X_XSIZE_FORMAT "'", name, itype);
        } else if (!x_signal_is_valid_name(name)) {
            x_critical(X_STRLOC ": unable to look up invalid signal name \"%s\" on type '%s'", name, x_type_name(itype));
        }
    }

    return signal_id;
}

xuint *x_signal_list_ids(XType itype, xuint *n_ids)
{
    xuint i;
    xuint n_nodes;
    XArray *result;
    SignalKey *keys;

    x_return_val_if_fail(X_TYPE_IS_INSTANTIATABLE(itype) || X_TYPE_IS_INTERFACE(itype), NULL);
    x_return_val_if_fail(n_ids != NULL, NULL);

    SIGNAL_LOCK();
    keys = (SignalKey *)x_bsearch_array_get_nth(x_signal_key_bsa, &x_signal_key_bconfig, 0);
    n_nodes = (xuint)x_bsearch_array_get_n_nodes(x_signal_key_bsa);
    result = (XArray *)x_array_new(FALSE, FALSE, sizeof(xuint));

    for (i = 0; i < n_nodes; i++) {
        if (keys[i].itype == itype) {
            x_array_append_val(result, keys[i].signal_id);
        }
    }

    *n_ids = result->len;

    SIGNAL_UNLOCK();
    if (!n_nodes) {
        if (!x_type_name(itype)) {
            x_critical(X_STRLOC ": unable to list signals for invalid type id '%" X_XSIZE_FORMAT "'", itype);
        } else if (!X_TYPE_IS_INSTANTIATABLE(itype) && !X_TYPE_IS_INTERFACE(itype)) {
            x_critical(X_STRLOC ": unable to list signals of non instantiatable type '%s'", x_type_name(itype));
        } else if (!x_type_class_peek(itype) && !X_TYPE_IS_INTERFACE(itype)) {
            x_critical(X_STRLOC ": unable to list signals of unloaded type '%s'", x_type_name(itype));
        }
    }

    return (xuint *)x_array_free(result, FALSE);
}

const xchar *x_signal_name(xuint signal_id)
{
    SignalNode *node;
    const xchar *name;

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    name = node ? node->name : NULL;
    SIGNAL_UNLOCK();

    return (char *)name;
}

void x_signal_query(xuint signal_id, XSignalQuery *query)
{
    SignalNode *node;

    x_return_if_fail(query != NULL);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (!node || node->destroyed) {
        query->signal_id = 0;
    } else {
        query->signal_id = node->signal_id;
        query->signal_name = node->name;
        query->itype = node->itype;
        query->signal_flags = (XSignalFlags)node->flags;
        query->return_type = node->return_type;
        query->n_params = node->n_params;
        query->param_types = node->param_types;
    }
    SIGNAL_UNLOCK();
}

xuint x_signal_new(const xchar *signal_name, XType itype, XSignalFlags signal_flags, xuint class_offset, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, ...)
{
    va_list args;
    xuint signal_id;

    x_return_val_if_fail(signal_name != NULL, 0);

    va_start(args, n_params);
    signal_id = x_signal_new_valist(signal_name, itype, signal_flags, class_offset ? x_signal_type_cclosure_new(itype, class_offset) : NULL, accumulator, accu_data, c_marshaller, return_type, n_params, args);
    va_end(args);

    return signal_id;
}

xuint x_signal_new_class_handler(const xchar *signal_name, XType itype, XSignalFlags signal_flags, XCallback class_handler, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, ...)
{
    va_list args;
    xuint signal_id;

    x_return_val_if_fail(signal_name != NULL, 0);

    va_start(args, n_params);
    signal_id = x_signal_new_valist(signal_name, itype, signal_flags, class_handler ? x_cclosure_new(class_handler, NULL, NULL) : NULL, accumulator, accu_data, c_marshaller, return_type, n_params, args);
    va_end(args);

    return signal_id;
}

static inline ClassClosure *signal_find_class_closure(SignalNode *node, XType itype)
{
    ClassClosure *cc;
    XBSearchArray *bsa = node->class_closure_bsa;

    if (bsa) {
        ClassClosure key;

        if (x_bsearch_array_get_n_nodes(bsa) == 1){
            cc = (ClassClosure *)x_bsearch_array_get_nth(bsa, &x_class_closure_bconfig, 0);
            if (cc && cc->instance_type == 0) {
                return cc;
            }
        }

        key.instance_type = itype;
        cc = (ClassClosure *)x_bsearch_array_lookup(bsa, &x_class_closure_bconfig, &key);
        while (!cc && key.instance_type) {
            key.instance_type = x_type_parent(key.instance_type);
            cc = (ClassClosure *)x_bsearch_array_lookup(bsa, &x_class_closure_bconfig, &key);
        }
    } else {
        cc = NULL;
    }

    return cc;
}

static inline XClosure *signal_lookup_closure(SignalNode *node, XTypeInstance *instance)
{
    ClassClosure *cc;

    cc = signal_find_class_closure (node, X_TYPE_FROM_INSTANCE(instance));
    return cc ? cc->closure : NULL;
}

static void signal_add_class_closure(SignalNode *node, XType itype, XClosure *closure)
{
    ClassClosure key;

    node->single_va_closure_is_valid = FALSE;

    if (!node->class_closure_bsa) {
        node->class_closure_bsa = x_bsearch_array_create(&x_class_closure_bconfig);
    }

    key.instance_type = itype;
    key.closure = x_closure_ref(closure);
    node->class_closure_bsa = x_bsearch_array_insert(node->class_closure_bsa, &x_class_closure_bconfig, &key);
    x_closure_sink(closure);
    if (node->c_marshaller && closure && X_CLOSURE_NEEDS_MARSHAL(closure)) {
        x_closure_set_marshal(closure, node->c_marshaller);
        if (node->va_marshaller) {
            _x_closure_set_va_marshal(closure, node->va_marshaller);
        }
    }
}

xuint x_signal_newv(const xchar *signal_name, XType itype, XSignalFlags signal_flags, XClosure *class_closure, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, XType *param_types)
{
    SignalNode *node;
    const xchar *name;
    xuint signal_id, i;
    xchar *signal_name_copy = NULL;
    XSignalCVaMarshaller va_marshaller;
    XSignalCMarshaller builtin_c_marshaller;
    XSignalCVaMarshaller builtin_va_marshaller;

    x_return_val_if_fail(signal_name != NULL, 0);
    x_return_val_if_fail(x_signal_is_valid_name(signal_name), 0);
    x_return_val_if_fail(X_TYPE_IS_INSTANTIATABLE(itype) || X_TYPE_IS_INTERFACE(itype), 0);

    if (n_params) {
        x_return_val_if_fail(param_types != NULL, 0);
    }
    x_return_val_if_fail((return_type & X_SIGNAL_TYPE_STATIC_SCOPE) == 0, 0);

    if (return_type == (X_TYPE_NONE & ~X_SIGNAL_TYPE_STATIC_SCOPE)) {
        x_return_val_if_fail(accumulator == NULL, 0);
    }

    if (!accumulator) {
        x_return_val_if_fail(accu_data == NULL, 0);
    }
    x_return_val_if_fail((signal_flags & X_SIGNAL_ACCUMULATOR_FIRST_RUN) == 0, 0);

    if (!is_canonical(signal_name)) {
        signal_name_copy = x_strdup(signal_name);
        canonicalize_key(signal_name_copy);
        name = signal_name_copy;
    } else {
        name = signal_name;
    }

    SIGNAL_LOCK();

    signal_id = signal_id_lookup(name, itype);
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (node && !node->destroyed) {
        x_critical(X_STRLOC ": signal \"%s\" already exists in the '%s' %s", name, type_debug_name (node->itype), X_TYPE_IS_INTERFACE(node->itype) ? "interface" : "class ancestry");
        x_free(signal_name_copy);
        SIGNAL_UNLOCK();
        return 0;
    }

    if (node && node->itype != itype) {
        x_critical(X_STRLOC ": signal \"%s\" for type '%s' was previously created for type '%s'", name, type_debug_name(itype), type_debug_name(node->itype));
        x_free(signal_name_copy);
        SIGNAL_UNLOCK();
        return 0;
    }

    for (i = 0; i < n_params; i++) {
        if (!X_TYPE_IS_VALUE(param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE)) {
            x_critical(X_STRLOC ": parameter %d of type '%s' for signal \"%s::%s\" is not a value type", i + 1, type_debug_name(param_types[i]), type_debug_name(itype), name);
            x_free(signal_name_copy);
            SIGNAL_UNLOCK();
            return 0;
        }
    }

    if (return_type != X_TYPE_NONE && !X_TYPE_IS_VALUE(return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE)) {
        x_critical(X_STRLOC ": return value of type '%s' for signal \"%s::%s\" is not a value type", type_debug_name(return_type), type_debug_name(itype), name);
        x_free(signal_name_copy);
        SIGNAL_UNLOCK();
        return 0;
    }

    if (!node) {
        SignalKey key;

        signal_id = x_n_signal_nodes++;
        node = x_new(SignalNode, 1);
        node->signal_id = signal_id;
        x_signal_nodes = x_renew(SignalNode*, x_signal_nodes, x_n_signal_nodes);
        x_signal_nodes[signal_id] = node;
        node->itype = itype;
        key.itype = itype;
        key.signal_id = signal_id;
        node->name = x_intern_string(name);
        key.quark = x_quark_from_string (name);
        x_signal_key_bsa = x_bsearch_array_insert(x_signal_key_bsa, &x_signal_key_bconfig, &key);

        TRACE(XOBJECT_SIGNAL_NEW(signal_id, name, itype));
    }
    node->destroyed = FALSE;

    node->single_va_closure_is_valid = FALSE;
    node->flags = signal_flags & X_SIGNAL_FLAGS_MASK;
    node->n_params = n_params;
    node->param_types = (XType *)x_memdup2(param_types, sizeof(XType) * n_params);
    node->return_type = return_type;
    node->class_closure_bsa = NULL;
    if (accumulator) {
        node->accumulator = x_new(SignalAccumulator, 1);
        node->accumulator->func = accumulator;
        node->accumulator->data = accu_data;
    } else {
        node->accumulator = NULL;
    }

    builtin_c_marshaller = NULL;
    builtin_va_marshaller = NULL;

    if (n_params == 0 && return_type == X_TYPE_NONE) {
        builtin_c_marshaller = x_cclosure_marshal_VOID__VOID;
        builtin_va_marshaller = x_cclosure_marshal_VOID__VOIDv;
    } else if (n_params == 1 && return_type == X_TYPE_NONE) {
#define ADD_CHECK(__type__) \
    else if (x_type_is_a(param_types[0] & ~X_SIGNAL_TYPE_STATIC_SCOPE, X_TYPE_ ##__type__)) {   \
        builtin_c_marshaller = x_cclosure_marshal_VOID__ ## __type__;                           \
        builtin_va_marshaller = x_cclosure_marshal_VOID__ ## __type__ ##v;                      \
    }

        if (0) {}
        ADD_CHECK(BOOLEAN)
        ADD_CHECK(CHAR)
        ADD_CHECK(UCHAR)
        ADD_CHECK(INT)
        ADD_CHECK(UINT)
        ADD_CHECK(LONG)
        ADD_CHECK(ULONG)
        ADD_CHECK(ENUM)
        ADD_CHECK(FLAGS)
        ADD_CHECK(FLOAT)
        ADD_CHECK(DOUBLE)
        ADD_CHECK(STRING)
        ADD_CHECK(PARAM)
        ADD_CHECK(BOXED)
        ADD_CHECK(POINTER)
        ADD_CHECK(OBJECT)
        ADD_CHECK(VARIANT)
    }

    if (c_marshaller == NULL) {
        if (builtin_c_marshaller) {
            c_marshaller = builtin_c_marshaller;
            va_marshaller = builtin_va_marshaller;
        } else {
            c_marshaller = x_cclosure_marshal_generic;
            va_marshaller = x_cclosure_marshal_generic_va;
        }
    } else {
        va_marshaller = NULL;
    }

    node->c_marshaller = c_marshaller;
    node->va_marshaller = va_marshaller;
    node->emission_hooks = NULL;
    if (class_closure) {
        signal_add_class_closure(node, 0, class_closure);
    }
    SIGNAL_UNLOCK();

    x_free(signal_name_copy);

    return signal_id;
}

void x_signal_set_va_marshaller(xuint signal_id, XType instance_type, XSignalCVaMarshaller va_marshaller)
{
    SignalNode *node;

    x_return_if_fail(signal_id > 0);
    x_return_if_fail(va_marshaller != NULL);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (node) {
        node->va_marshaller = va_marshaller;
        if (node->class_closure_bsa) {
            ClassClosure *cc = (ClassClosure *)x_bsearch_array_get_nth(node->class_closure_bsa, &x_class_closure_bconfig, 0);
            if (cc->closure->marshal == node->c_marshaller) {
                _x_closure_set_va_marshal(cc->closure, va_marshaller);
            }
        }

        node->single_va_closure_is_valid = FALSE;
    }

    SIGNAL_UNLOCK();
}

xuint x_signal_new_valist(const xchar *signal_name, XType itype, XSignalFlags signal_flags, XClosure *class_closure, XSignalAccumulator accumulator, xpointer accu_data, XSignalCMarshaller c_marshaller, XType return_type, xuint n_params, va_list args)
{
    xuint i;
    xuint signal_id;
    XType *param_types;
    XType *param_types_heap = NULL;
    XType param_types_stack[200 / sizeof(XType)];

    param_types = param_types_stack;
    if (n_params > 0) {
        if (X_UNLIKELY(n_params > X_N_ELEMENTS(param_types_stack))) {
            param_types_heap = x_new(XType, n_params);
            param_types = param_types_heap;
        }

        for (i = 0; i < n_params; i++) {
            param_types[i] = va_arg(args, XType);
        }
    }

    signal_id = x_signal_newv(signal_name, itype, signal_flags, class_closure, accumulator, accu_data, c_marshaller, return_type, n_params, param_types);
    x_free(param_types_heap);

    return signal_id;
}

static void signal_destroy_R(SignalNode *signal_node)
{
    SignalNode node = *signal_node;

    signal_node->destroyed = TRUE;

    signal_node->single_va_closure_is_valid = FALSE;
    signal_node->n_params = 0;
    signal_node->param_types = NULL;
    signal_node->return_type = 0;
    signal_node->class_closure_bsa = NULL;
    signal_node->accumulator = NULL;
    signal_node->c_marshaller = NULL;
    signal_node->va_marshaller = NULL;
    signal_node->emission_hooks = NULL;

    SIGNAL_UNLOCK();
    x_free(node.param_types);
    if (node.class_closure_bsa) {
        xuint i;

        for (i = 0; i < node.class_closure_bsa->n_nodes; i++) {
            ClassClosure *cc = (ClassClosure *)x_bsearch_array_get_nth(node.class_closure_bsa, &x_class_closure_bconfig, i);
            x_closure_unref(cc->closure);
        }

        x_bsearch_array_free(node.class_closure_bsa, &x_class_closure_bconfig);
    }

    x_free(node.accumulator);
    if (node.emission_hooks) {
        x_hook_list_clear(node.emission_hooks);
        x_free(node.emission_hooks);
    }
    SIGNAL_LOCK();
}

void x_signal_override_class_closure(xuint signal_id, XType instance_type, XClosure *class_closure)
{
    SignalNode *node;

    x_return_if_fail(signal_id > 0);
    x_return_if_fail(class_closure != NULL);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    node_check_deprecated(node);
    if (!x_type_is_a(instance_type, node->itype)) {
        x_critical("%s: type '%s' cannot be overridden for signal id '%u'", X_STRLOC, type_debug_name(instance_type), signal_id);
    } else {
        ClassClosure *cc = signal_find_class_closure(node, instance_type);
        
        if (cc && cc->instance_type == instance_type) {
            x_critical("%s: type '%s' is already overridden for signal id '%u'", X_STRLOC, type_debug_name(instance_type), signal_id);
        } else {
            signal_add_class_closure(node, instance_type, class_closure);
        }
    }
    SIGNAL_UNLOCK();
}

void x_signal_override_class_handler(const xchar *signal_name, XType instance_type, XCallback class_handler)
{
    xuint signal_id;

    x_return_if_fail(signal_name != NULL);
    x_return_if_fail(instance_type != X_TYPE_NONE);
    x_return_if_fail(class_handler != NULL);

    signal_id = x_signal_lookup(signal_name, instance_type);

    if (signal_id) {
        x_signal_override_class_closure (signal_id, instance_type, x_cclosure_new(class_handler, NULL, NULL));
    } else {
        x_critical("%s: signal name '%s' is invalid for type id '%" X_XSIZE_FORMAT "'", X_STRLOC, signal_name, instance_type);
    }
}

void x_signal_chain_from_overridden(const XValue *instance_and_params, XValue *return_value)
{
    xpointer instance;
    xuint n_params = 0;
    XClosure *closure = NULL;
    Emission *emission = NULL;
    XType chain_type = 0, restore_type = 0;

    x_return_if_fail(instance_and_params != NULL);
    instance = x_value_peek_pointer(instance_and_params);
    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));

    SIGNAL_LOCK();
    emission = emission_find_innermost(instance);
    if (emission) {
        SignalNode *node = LOOKUP_SIGNAL_NODE(emission->ihint.signal_id);

        x_assert(node != NULL);

        if (emission->chain_type != X_TYPE_NONE) {
            ClassClosure *cc = signal_find_class_closure(node, emission->chain_type);

            x_assert(cc != NULL);

            n_params = node->n_params;
            restore_type = cc->instance_type;
            cc = signal_find_class_closure(node, x_type_parent(cc->instance_type));
            if (cc && cc->instance_type != restore_type) {
                closure = cc->closure;
                chain_type = cc->instance_type;
            }
        } else {
            x_critical("%s: signal id '%u' cannot be chained from current emission stage for instance '%p'", X_STRLOC, node->signal_id, instance);
        }
    } else {
        x_critical("%s: no signal is currently being emitted for instance '%p'", X_STRLOC, instance);
    }

    if (closure) {
        emission->chain_type = chain_type;
        SIGNAL_UNLOCK();
        x_closure_invoke(closure, return_value, n_params + 1, instance_and_params, &emission->ihint);
        SIGNAL_LOCK();
        emission->chain_type = restore_type;
    }
    SIGNAL_UNLOCK();
}

void x_signal_chain_from_overridden_handler(xpointer instance, ...)
{
    xuint n_params = 0;
    SignalNode *node = NULL;
    XClosure *closure = NULL;
    Emission *emission = NULL;
    XType chain_type = 0, restore_type = 0;

    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));

    SIGNAL_LOCK();
    emission = emission_find_innermost(instance);
    if (emission) {
        node = LOOKUP_SIGNAL_NODE(emission->ihint.signal_id);

        x_assert(node != NULL);

        if (emission->chain_type != X_TYPE_NONE) {
            ClassClosure *cc = signal_find_class_closure (node, emission->chain_type);

            x_assert(cc != NULL);

            n_params = node->n_params;
            restore_type = cc->instance_type;
            cc = signal_find_class_closure (node, x_type_parent(cc->instance_type));
            if (cc && cc->instance_type != restore_type) {
                closure = cc->closure;
                chain_type = cc->instance_type;
            }
        } else {
            x_critical("%s: signal id '%u' cannot be chained from current emission stage for instance '%p'", X_STRLOC, node->signal_id, instance);
        }
    } else {
        x_critical("%s: no signal is currently being emitted for instance '%p'", X_STRLOC, instance);
    }

    if (closure) {
        xuint i;
        va_list var_args;
        XValue *param_values;
        XType signal_return_type;
        XValue *instance_and_params;

        va_start(var_args, instance);

        signal_return_type = node->return_type;
        instance_and_params = x_newa0(XValue, n_params + 1);
        param_values = instance_and_params + 1;

        for (i = 0; i < node->n_params; i++) {
            xchar *error;
            XType ptype = node->param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE;
            xboolean static_scope = node->param_types[i] & X_SIGNAL_TYPE_STATIC_SCOPE;

            SIGNAL_UNLOCK();
            X_VALUE_COLLECT_INIT(param_values + i, ptype, var_args, static_scope ? X_VALUE_NOCOPY_CONTENTS : 0, &error);
            if (error) {
                x_critical("%s: %s", X_STRLOC, error);
                x_free(error);

                while (i--) {
                    x_value_unset(param_values + i);
                }

                va_end(var_args);
                return;
            }
            SIGNAL_LOCK();
        }

        SIGNAL_UNLOCK();
        x_value_init_from_instance(instance_and_params, instance);
        SIGNAL_LOCK();

        emission->chain_type = chain_type;
        SIGNAL_UNLOCK();

        if (signal_return_type == X_TYPE_NONE) {
            x_closure_invoke(closure, NULL, n_params + 1, instance_and_params, &emission->ihint);
        } else {
            xchar *error = NULL;
            XValue return_value = X_VALUE_INIT;
            XType rtype = signal_return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE;
            xboolean static_scope = signal_return_type & X_SIGNAL_TYPE_STATIC_SCOPE;

            x_value_init(&return_value, rtype);

            x_closure_invoke(closure, &return_value, n_params + 1, instance_and_params, &emission->ihint);

            X_VALUE_LCOPY(&return_value, var_args, static_scope ? X_VALUE_NOCOPY_CONTENTS : 0, &error);
            if (!error) {
                x_value_unset(&return_value);
            } else {
                x_critical("%s: %s", X_STRLOC, error);
                x_free(error);
            }
        }

        for (i = 0; i < n_params; i++) {
            x_value_unset(param_values + i);
        }
        x_value_unset(instance_and_params);

        va_end(var_args);

        SIGNAL_LOCK();
        emission->chain_type = restore_type;
    }
    SIGNAL_UNLOCK();
}

XSignalInvocationHint *x_signal_get_invocation_hint(xpointer instance)
{
    Emission *emission = NULL;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), NULL);

    SIGNAL_LOCK();
    emission = emission_find_innermost(instance);
    SIGNAL_UNLOCK();

    return emission ? &emission->ihint : NULL;
}

xulong x_signal_connect_closure_by_id(xpointer instance, xuint signal_id, XQuark detail, XClosure *closure, xboolean after)
{
    SignalNode *node;
    xulong handler_seq_no = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail(signal_id > 0, 0);
    x_return_val_if_fail(closure != NULL, 0);

    SIGNAL_LOCK();
    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (node) {
        if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
            x_critical("%s: signal id '%u' does not support detail (%u)", X_STRLOC, signal_id, detail);
        } else if (!x_type_is_a(X_TYPE_FROM_INSTANCE(instance), node->itype)) {
            x_critical("%s: signal id '%u' is invalid for instance '%p'", X_STRLOC, signal_id, instance);
        } else {
            Handler *handler = handler_new(signal_id, instance, after);

            if (X_TYPE_IS_OBJECT(node->itype)) {
                _x_object_set_has_signal_handler((XObject *)instance, signal_id);
            }

            handler_seq_no = handler->sequential_number;
            handler->detail = detail;
            handler->closure = x_closure_ref(closure);
            x_closure_sink(closure);
            add_invalid_closure_notify(handler, instance);
            handler_insert(signal_id, instance, handler);

            if (node->c_marshaller && X_CLOSURE_NEEDS_MARSHAL(closure)) {
                x_closure_set_marshal(closure, node->c_marshaller);
                if (node->va_marshaller) {
                    _x_closure_set_va_marshal(closure, node->va_marshaller);
                }
            }
        }
    } else {
        x_critical("%s: signal id '%u' is invalid for instance '%p'", X_STRLOC, signal_id, instance);
    }
    SIGNAL_UNLOCK();

    return handler_seq_no;
}

xulong x_signal_connect_closure(xpointer instance, const xchar *detailed_signal, XClosure *closure, xboolean after)
{
    XType itype;
    xuint signal_id;
    XQuark detail = 0;
    xulong handler_seq_no = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail(detailed_signal != NULL, 0);
    x_return_val_if_fail(closure != NULL, 0);

    SIGNAL_LOCK();
    itype = X_TYPE_FROM_INSTANCE(instance);
    signal_id = signal_parse_name(detailed_signal, itype, &detail, TRUE);

    if (signal_id) {
        SignalNode *node = LOOKUP_SIGNAL_NODE(signal_id);

        if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
            x_critical("%s: signal '%s' does not support details", X_STRLOC, detailed_signal);
        } else if (!x_type_is_a(itype, node->itype)) {
            x_critical("%s: signal '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
        } else {
            Handler *handler = handler_new(signal_id, instance, after);

            if (X_TYPE_IS_OBJECT(node->itype)) {
                _x_object_set_has_signal_handler((XObject *)instance, signal_id);
            }

            handler_seq_no = handler->sequential_number;
            handler->detail = detail;
            handler->closure = x_closure_ref(closure);
            x_closure_sink(closure);
            add_invalid_closure_notify(handler, instance);
            handler_insert(signal_id, instance, handler);

            if (node->c_marshaller && X_CLOSURE_NEEDS_MARSHAL(handler->closure)) {
                x_closure_set_marshal(handler->closure, node->c_marshaller);
                if (node->va_marshaller) {
                    _x_closure_set_va_marshal(handler->closure, node->va_marshaller);
                }
            }
        }
     } else {
        x_critical("%s: signal '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
     }
    SIGNAL_UNLOCK();

    return handler_seq_no;
}

static void node_check_deprecated(const SignalNode *node)
{
    static const xchar *x_enable_diagnostic = NULL;

    if (X_UNLIKELY(!x_enable_diagnostic)) {
        x_enable_diagnostic = x_getenv("X_ENABLE_DIAGNOSTIC");
        if (!x_enable_diagnostic) {
            x_enable_diagnostic = "0";
        }
    }

    if (x_enable_diagnostic[0] == '1') {
        if (node->flags & X_SIGNAL_DEPRECATED) {
            x_critical("The signal %s::%s is deprecated and shouldn't be used anymore. It will be removed in a future version.", type_debug_name(node->itype), node->name);
        }
    }
}

xulong x_signal_connect_data(xpointer instance, const xchar *detailed_signal, XCallback c_handler, xpointer data, XClosureNotify destroy_data, XConnectFlags connect_flags)
{
    XType itype;
    xuint signal_id;
    XQuark detail = 0;
    xboolean swapped, after;
    xulong handler_seq_no = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail(detailed_signal != NULL, 0);
    x_return_val_if_fail(c_handler != NULL, 0);

    swapped = (connect_flags & X_CONNECT_SWAPPED) != FALSE;
    after = (connect_flags & X_CONNECT_AFTER) != FALSE;

    SIGNAL_LOCK();
    itype = X_TYPE_FROM_INSTANCE(instance);
    signal_id = signal_parse_name(detailed_signal, itype, &detail, TRUE);
    if (signal_id) {
        SignalNode *node = LOOKUP_SIGNAL_NODE(signal_id);

        node_check_deprecated (node);

        if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
            x_critical("%s: signal '%s' does not support details", X_STRLOC, detailed_signal);
        } else if (!x_type_is_a(itype, node->itype)) {
            x_critical("%s: signal '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
        } else {
            Handler *handler = handler_new(signal_id, instance, after);

            if (X_TYPE_IS_OBJECT(node->itype)) {
                _x_object_set_has_signal_handler((XObject *)instance, signal_id);
            }

            handler_seq_no = handler->sequential_number;
            handler->detail = detail;
            handler->closure = x_closure_ref((swapped ? x_cclosure_new_swap : x_cclosure_new)(c_handler, data, destroy_data));
            x_closure_sink(handler->closure);
            handler_insert(signal_id, instance, handler);
            if (node->c_marshaller && X_CLOSURE_NEEDS_MARSHAL(handler->closure)) {
                x_closure_set_marshal (handler->closure, node->c_marshaller);
                if (node->va_marshaller) {
                    _x_closure_set_va_marshal(handler->closure, node->va_marshaller);
                }
            }
        }
    } else {
        x_critical("%s: signal '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
    }
    SIGNAL_UNLOCK();

    return handler_seq_no;
}

static void signal_handler_block_unlocked (xpointer instance, xulong handler_id);

void x_signal_handler_block(xpointer instance, xulong handler_id)
{
    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(handler_id > 0);

    SIGNAL_LOCK();
    signal_handler_block_unlocked(instance, handler_id);
    SIGNAL_UNLOCK();
}

static void signal_handler_block_unlocked(xpointer instance, xulong handler_id)
{
    Handler *handler;

    handler = handler_lookup(instance, handler_id, NULL, NULL);
    if (handler) {
        if (handler->block_count >= HANDLER_MAX_BLOCK_COUNT - 1) {
            x_error(X_STRLOC ": handler block_count overflow, %s", REPORT_BUG);
        }

        handler->block_count += 1;
    } else {
        x_critical("%s: instance '%p' has no handler with id '%lu'", X_STRLOC, instance, handler_id);
    }
}

static void signal_handler_unblock_unlocked(xpointer instance, xulong handler_id);

void x_signal_handler_unblock(xpointer instance, xulong handler_id)
{
    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(handler_id > 0);

    SIGNAL_LOCK();
    signal_handler_unblock_unlocked(instance, handler_id);
    SIGNAL_UNLOCK();
}

static void signal_handler_unblock_unlocked(xpointer instance, xulong handler_id)
{
    Handler *handler;

    handler = handler_lookup(instance, handler_id, NULL, NULL);
    if (handler) {
        if (handler->block_count) {
            handler->block_count -= 1;
        } else {
            x_critical(X_STRLOC ": handler '%lu' of instance '%p' is not blocked", handler_id, instance);
        }
    } else {
        x_critical("%s: instance '%p' has no handler with id '%lu'", X_STRLOC, instance, handler_id);
    }
}

static void signal_handler_disconnect_unlocked(xpointer instance, xulong handler_id);

void x_signal_handler_disconnect(xpointer instance, xulong handler_id)
{
    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(handler_id > 0);

    SIGNAL_LOCK();
    signal_handler_disconnect_unlocked(instance, handler_id);
    SIGNAL_UNLOCK();
}

static void signal_handler_disconnect_unlocked(xpointer instance, xulong handler_id)
{
    Handler *handler;

    handler = handler_lookup(instance, handler_id, 0, 0);
    if (handler) {
        x_hash_table_remove(x_handlers, handler);
        handler->sequential_number = 0;
        handler->block_count = 1;
        remove_invalid_closure_notify(handler, instance);
        handler_unref_R(handler->signal_id, instance, handler);
    } else {
        x_critical("%s: instance '%p' has no handler with id '%lu'", X_STRLOC, instance, handler_id);
    }
}

xboolean x_signal_handler_is_connected(xpointer instance, xulong handler_id)
    {
    Handler *handler;
    xboolean connected;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), FALSE);

    SIGNAL_LOCK();
    handler = handler_lookup(instance, handler_id, NULL, NULL);
    connected = handler != NULL;
    SIGNAL_UNLOCK();

    return connected;
}

void x_signal_handlers_destroy(xpointer instance)
{
    XBSearchArray *hlbsa;

    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));

    SIGNAL_LOCK();
    hlbsa = (XBSearchArray *)x_hash_table_lookup(x_handler_list_bsa_ht, instance);
    if (hlbsa) {
        xuint i;

        x_hash_table_remove(x_handler_list_bsa_ht, instance);

        for (i = 0; i < hlbsa->n_nodes; i++) {
            HandlerList *hlist = (HandlerList *)x_bsearch_array_get_nth(hlbsa, &x_signal_hlbsa_bconfig, i);
            Handler *handler = hlist->handlers;

            while (handler) {
                Handler *tmp = handler;

                handler = tmp->next;
                tmp->block_count = 1;
                tmp->next = NULL;
                tmp->prev = tmp;
                if (tmp->sequential_number) {
                    x_hash_table_remove(x_handlers, tmp);
                    remove_invalid_closure_notify(tmp, instance);
                    tmp->sequential_number = 0;
                    handler_unref_R(0, NULL, tmp);
                }
            }
        }

        x_bsearch_array_free(hlbsa, &x_signal_hlbsa_bconfig);
    }
    SIGNAL_UNLOCK();
}

xulong x_signal_handler_find(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data)
{
    xulong handler_seq_no = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    if (mask & X_SIGNAL_MATCH_MASK) {
        HandlerMatch *mlist;

        SIGNAL_LOCK();
        mlist = handlers_find(instance, mask, signal_id, detail, closure, func, data, TRUE);
        if (mlist) {
            handler_seq_no = mlist->handler->sequential_number;
            handler_match_free1_R(mlist, instance);
        }
        SIGNAL_UNLOCK();
    }

    return handler_seq_no;
}

typedef void (*CallbackHandlerFunc)(xpointer instance, xulong handler_seq_no);

static xuint signal_handlers_foreach_matched_unlocked_R(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data, CallbackHandlerFunc callback)
{
    HandlerMatch *mlist;
    xuint n_handlers = 0;

    mlist = handlers_find(instance, mask, signal_id, detail, closure, func, data, FALSE);
    while (mlist) {
        n_handlers++;
        if (mlist->handler->sequential_number) {
            callback (instance, mlist->handler->sequential_number);
        }

        mlist = handler_match_free1_R(mlist, instance);
    }

    return n_handlers;
}

xuint x_signal_handlers_block_matched(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data)
{
    xuint n_handlers = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    if (mask & (X_SIGNAL_MATCH_ID | X_SIGNAL_MATCH_CLOSURE | X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA)) {
        SIGNAL_LOCK();
        n_handlers = signal_handlers_foreach_matched_unlocked_R(instance, mask, signal_id, detail, closure, func, data, signal_handler_block_unlocked);
        SIGNAL_UNLOCK();
    }

    return n_handlers;
}

xuint x_signal_handlers_unblock_matched(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data)
{
    xuint n_handlers = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    if (mask & (X_SIGNAL_MATCH_ID | X_SIGNAL_MATCH_CLOSURE | X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA)) {
        SIGNAL_LOCK();
        n_handlers = signal_handlers_foreach_matched_unlocked_R(instance, mask, signal_id, detail, closure, func, data, signal_handler_unblock_unlocked);
        SIGNAL_UNLOCK();
    }

    return n_handlers;
}

xuint x_signal_handlers_disconnect_matched(xpointer instance, XSignalMatchType mask, xuint signal_id, XQuark detail, XClosure *closure, xpointer func, xpointer data)
{
    xuint n_handlers = 0;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), 0);
    x_return_val_if_fail((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    if (mask & (X_SIGNAL_MATCH_ID | X_SIGNAL_MATCH_CLOSURE | X_SIGNAL_MATCH_FUNC | X_SIGNAL_MATCH_DATA)) {
        SIGNAL_LOCK();
        n_handlers = signal_handlers_foreach_matched_unlocked_R(instance, mask, signal_id, detail, closure, func, data, signal_handler_disconnect_unlocked);
        SIGNAL_UNLOCK();
    }

    return n_handlers;
}

xboolean x_signal_has_handler_pending(xpointer instance, xuint signal_id, XQuark detail, xboolean may_be_blocked)
{
    SignalNode *node;
    HandlerMatch *mlist;
    xboolean has_pending;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), FALSE);
    x_return_val_if_fail(signal_id > 0, FALSE);

    SIGNAL_LOCK();

    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (detail) {
        if (!(node->flags & X_SIGNAL_DETAILED)) {
            x_critical("%s: signal id '%u' does not support detail (%u)", X_STRLOC, signal_id, detail);
            SIGNAL_UNLOCK();
            return FALSE;
        }
    }

    mlist = handlers_find(instance, (XSignalMatchType)((X_SIGNAL_MATCH_ID | X_SIGNAL_MATCH_DETAIL | (may_be_blocked ? 0 : X_SIGNAL_MATCH_UNBLOCKED))), signal_id, detail, NULL, NULL, NULL, TRUE);
    if (mlist) {
        has_pending = TRUE;
        handler_match_free1_R(mlist, instance);
    } else {
        ClassClosure *class_closure = signal_find_class_closure (node, X_TYPE_FROM_INSTANCE(instance));
        if (class_closure != NULL && class_closure->instance_type != 0) {
            has_pending = TRUE;
        } else {
            has_pending = FALSE;
        }
    }
    SIGNAL_UNLOCK();

    return has_pending;
}

static void signal_emitv_unlocked(const XValue *instance_and_params, xuint signal_id, XQuark detail, XValue *return_value);

void x_signal_emitv(const XValue *instance_and_params, xuint signal_id, XQuark detail, XValue *return_value)
{
    SIGNAL_LOCK();
    signal_emitv_unlocked(instance_and_params, signal_id, detail, return_value);
    SIGNAL_UNLOCK();
}

static void signal_emitv_unlocked(const XValue *instance_and_params, xuint signal_id, XQuark detail, XValue *return_value)
{
    SignalNode *node;
    xpointer instance;

    x_return_if_fail(instance_and_params != NULL);
    instance = x_value_peek_pointer(instance_and_params);
    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(signal_id > 0);

    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (!node || !x_type_is_a(X_TYPE_FROM_INSTANCE(instance), node->itype)) {
        x_critical("%s: signal id '%u' is invalid for instance '%p'", X_STRLOC, signal_id, instance);
        return;
    }

    if (!node->single_va_closure_is_valid) {
        node_update_single_va_closure(node);
    }

    if (node->single_va_closure != NULL && (node->single_va_closure == SINGLE_VA_CLOSURE_EMPTY_MAGIC || _x_closure_is_void(node->single_va_closure, instance))) {
        HandlerList* hlist;

        if (_x_object_has_signal_handler((XObject *)instance)) {
            hlist = handler_list_lookup(node->signal_id, instance);
        } else {
            hlist = NULL;
        }

        if (hlist == NULL || hlist->handlers == NULL) {
            return;
        }
    }

    SignalNode node_copy = *node;
    signal_emit_unlocked_R(&node_copy, detail, instance, return_value, instance_and_params);
}

static inline xboolean accumulate(XSignalInvocationHint *ihint, XValue *return_accu, XValue *handler_return, SignalAccumulator *accumulator)
{
    xboolean continue_emission;

    if (!accumulator) {
        return TRUE;
    }

    continue_emission = accumulator->func(ihint, return_accu, handler_return, accumulator->data);
    x_value_reset(handler_return);
    ihint->run_type = (XSignalFlags)(ihint->run_type & ~X_SIGNAL_ACCUMULATOR_FIRST_RUN);

    return continue_emission;
}

static xboolean signal_emit_valist_unlocked(xpointer instance, xuint signal_id, XQuark detail, va_list var_args);

void x_signal_emit_valist(xpointer instance, xuint signal_id, XQuark detail, va_list var_args)
{
    SIGNAL_LOCK();
    if (signal_emit_valist_unlocked(instance, signal_id, detail, var_args)) {
        SIGNAL_UNLOCK();
    }
}

static xboolean signal_emit_valist_unlocked(xpointer instance, xuint signal_id, XQuark detail, va_list var_args)
{
    xuint i;
    SignalNode *node;
    XValue *param_values;
    XValue *instance_and_params;

    x_return_val_if_fail(X_TYPE_CHECK_INSTANCE(instance), TRUE);
    x_return_val_if_fail(signal_id > 0, TRUE);

    node = LOOKUP_SIGNAL_NODE(signal_id);
    if (!node || !x_type_is_a(X_TYPE_FROM_INSTANCE(instance), node->itype)) {
        x_critical("%s: signal id '%u' is invalid for instance '%p'", X_STRLOC, signal_id, instance);
        return TRUE;
    }

    if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
        x_critical("%s: signal id '%u' does not support detail (%u)", X_STRLOC, signal_id, detail);
        return TRUE;
    }

    if (!node->single_va_closure_is_valid) {
        node_update_single_va_closure(node);
    }

    SignalNode node_copy = *node;

    if (node->single_va_closure != NULL) {
        Handler *l;
        HandlerList *hlist;
        XClosure *closure = NULL;
        xboolean fastpath = TRUE;
        Handler *fastpath_handler = NULL;
        XSignalFlags run_type = X_SIGNAL_RUN_FIRST;

        if (node->single_va_closure != SINGLE_VA_CLOSURE_EMPTY_MAGIC && !_x_closure_is_void(node->single_va_closure, instance)) {
            if (_x_closure_supports_invoke_va(node->single_va_closure)) {
                closure = node->single_va_closure;
                if (node->single_va_closure_is_after) {
                    run_type = X_SIGNAL_RUN_LAST;
                } else {
                    run_type = X_SIGNAL_RUN_FIRST;
                }
            } else {
                fastpath = FALSE;
            }
        }

        if (_x_object_has_signal_handler((XObject *)instance)) {
            hlist = handler_list_lookup(node->signal_id, instance);
        } else {
            hlist = NULL;
        }

        for (l = hlist ? hlist->handlers : NULL; fastpath && l != NULL; l = l->next) {
            if (!l->block_count && (!l->detail || l->detail == detail)) {
                if (closure != NULL || !_x_closure_supports_invoke_va(l->closure)) {
                    fastpath = FALSE;
                    break;
                } else {
                    fastpath_handler = l;
                    closure = l->closure;
                    if (l->after) {
                        run_type = X_SIGNAL_RUN_LAST;
                    } else {
                        run_type = X_SIGNAL_RUN_FIRST;
                    }
                }
            }
        }

        if (fastpath && closure == NULL && node_copy.return_type == X_TYPE_NONE) {
            return TRUE;
        }

        if (closure != NULL && (node_copy.flags & (X_SIGNAL_NO_RECURSE)) != 0) {
            fastpath = FALSE;
        }

        if (fastpath) {
            Emission emission;
            XValue emission_return = X_VALUE_INIT;
            XValue *return_accu, accu = X_VALUE_INIT;
            XType instance_type = X_TYPE_FROM_INSTANCE(instance);
            XType rtype = node_copy.return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE;
            xboolean static_scope = node_copy.return_type & X_SIGNAL_TYPE_STATIC_SCOPE;

            if (rtype == X_TYPE_NONE) {
                return_accu = NULL;
            } else if (node_copy.accumulator) {
                return_accu = &accu;
            } else {
                return_accu = &emission_return;
            }

            emission.instance = instance;
            emission.ihint.signal_id = signal_id;
            emission.ihint.detail = detail;
            emission.ihint.run_type = (XSignalFlags)(run_type | X_SIGNAL_ACCUMULATOR_FIRST_RUN);
            emission.state = EMISSION_RUN;
            emission.chain_type = instance_type;
            emission_push(&emission);

            if (fastpath_handler) {
                handler_ref(fastpath_handler);
            }

            if (closure != NULL) {
                TRACE(XOBJECT_SIGNAL_EMIT(signal_id, detail, instance, instance_type));

                SIGNAL_UNLOCK();

                if (rtype != X_TYPE_NONE) {
                    x_value_init(&emission_return, rtype);
                }

                if (node_copy.accumulator) {
                    x_value_init(&accu, rtype);
                }

#ifndef __COVERITY__
                x_object_ref(instance);
#endif
                _x_closure_invoke_va(closure, return_accu, instance, var_args, node_copy.n_params, node_copy.param_types);
                accumulate(&emission.ihint, &emission_return, &accu, node_copy.accumulator);

                if (node_copy.accumulator) {
                    x_value_unset(&accu);
                }

                SIGNAL_LOCK();
            }

            emission.chain_type = X_TYPE_NONE;
            emission_pop(&emission);

            if (fastpath_handler) {
                handler_unref_R(signal_id, instance, fastpath_handler);
            }

            SIGNAL_UNLOCK();

            if (rtype != X_TYPE_NONE) {
                xchar *error = NULL;
                for (i = 0; i < node_copy.n_params; i++) {
                    XType ptype = node_copy.param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE;
                    X_VALUE_COLLECT_SKIP(ptype, var_args);
                }

                if (closure == NULL) {
                    x_value_init(&emission_return, rtype);
                }

                X_VALUE_LCOPY(&emission_return, var_args, static_scope ? X_VALUE_NOCOPY_CONTENTS : 0, &error);
                if (!error) {
                    x_value_unset(&emission_return);
                } else {
                    x_critical("%s: %s", X_STRLOC, error);
                    x_free(error);
                }
            }

            TRACE(XOBJECT_SIGNAL_EMIT_END(signal_id, detail, instance, instance_type));

#ifndef __COVERITY__
            if (closure != NULL) {
                x_object_unref(instance);
            }
#endif
            return FALSE;
        }
    }
    SIGNAL_UNLOCK();

    instance_and_params = x_newa0(XValue, node_copy.n_params + 1);
    param_values = instance_and_params + 1;

    for (i = 0; i < node_copy.n_params; i++) {
        xchar *error;
        XType ptype = node_copy.param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        xboolean static_scope = node_copy.param_types[i] & X_SIGNAL_TYPE_STATIC_SCOPE;

        X_VALUE_COLLECT_INIT(param_values + i, ptype, var_args, static_scope ? X_VALUE_NOCOPY_CONTENTS : 0, &error);
        if (error) {
            x_critical("%s: %s", X_STRLOC, error);
            x_free(error);

            while (i--) {
                x_value_unset(param_values + i);
            }

            return FALSE;
        }
    }

    x_value_init_from_instance(instance_and_params, instance);
    if (node_copy.return_type == X_TYPE_NONE) {
        SIGNAL_LOCK();
        signal_emit_unlocked_R(&node_copy, detail, instance, NULL, instance_and_params);
        SIGNAL_UNLOCK();
    } else {
        xchar *error = NULL;
        XValue return_value = X_VALUE_INIT;
        XType rtype = node_copy.return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        xboolean static_scope = node_copy.return_type & X_SIGNAL_TYPE_STATIC_SCOPE;

        x_value_init(&return_value, rtype);

        SIGNAL_LOCK();
        signal_emit_unlocked_R(&node_copy, detail, instance, &return_value, instance_and_params);
        SIGNAL_UNLOCK();

        X_VALUE_LCOPY(&return_value, var_args, static_scope ? X_VALUE_NOCOPY_CONTENTS : 0, &error);
        if (!error) {
            x_value_unset(&return_value);
        } else {
            x_critical("%s: %s", X_STRLOC, error);
            x_free(error);
        }
    }

    for (i = 0; i < node_copy.n_params; i++) {
        x_value_unset(param_values + i);
    }
    x_value_unset(instance_and_params);

    return FALSE;
}

void x_signal_emit(xpointer instance, xuint signal_id, XQuark detail, ...)
{
    va_list var_args;

    va_start(var_args, detail);
    x_signal_emit_valist(instance, signal_id, detail, var_args);
    va_end(var_args);
}

void x_signal_emit_by_name(xpointer instance, const xchar *detailed_signal, ...)
{
    XType itype;
    xuint signal_id;
    XQuark detail = 0;

    x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
    x_return_if_fail(detailed_signal != NULL);

    itype = X_TYPE_FROM_INSTANCE(instance);

    SIGNAL_LOCK();
    signal_id = signal_parse_name(detailed_signal, itype, &detail, TRUE);

    if (signal_id) {
        va_list var_args;

        va_start(var_args, detailed_signal);
        if (signal_emit_valist_unlocked(instance, signal_id, detail, var_args)) {
            SIGNAL_UNLOCK();
        }
        va_end(var_args);
    } else {
        SIGNAL_UNLOCK();
        x_critical("%s: signal name '%s' is invalid for instance '%p' of type '%s'", X_STRLOC, detailed_signal, instance, x_type_name(itype));
    }
}

X_ALWAYS_INLINE static inline XValue *maybe_init_accumulator_unlocked(SignalNode *node, XValue *emission_return, XValue *accumulator_value)
{
    if (node->accumulator) {
        if (accumulator_value->x_type) {
            return accumulator_value;
        }

        x_value_init(accumulator_value, node->return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE);
        return accumulator_value;
    }

    return emission_return;
}

static xboolean signal_emit_unlocked_R(SignalNode *node, XQuark detail, xpointer instance, XValue *emission_return, const XValue *instance_and_params)
{
    xuint n_params;
    xuint signal_id;
    Emission emission;
    HandlerList *hlist;
    XClosure *class_closure;
    Handler *handler_list = NULL;
    SignalAccumulator *accumulator;
    xulong max_sequential_handler_number;
    xboolean return_value_altered = FALSE;
    XValue *return_accu, accu = X_VALUE_INIT;

    TRACE(XOBJECT_SIGNAL_EMIT(node->signal_id, detail, instance, X_TYPE_FROM_INSTANCE(instance)));

    signal_id = node->signal_id;
    n_params = node->n_params + 1;

    if (node->flags & X_SIGNAL_NO_RECURSE) {
        Emission *emission_node = emission_find(signal_id, detail, instance);
        if (emission_node) {
            emission_node->state = EMISSION_RESTART;
            return return_value_altered;
        }
    }

    accumulator = node->accumulator;

    emission.instance = instance;
    emission.ihint.signal_id = node->signal_id;
    emission.ihint.detail = detail;
    emission.ihint.run_type = (XSignalFlags)0;
    emission.state = (EmissionState)0;
    emission.chain_type = X_TYPE_NONE;
    emission_push(&emission);
    class_closure = signal_lookup_closure(node, (XTypeInstance *)instance);

EMIT_RESTART:
    if (handler_list) {
        handler_unref_R(signal_id, instance, handler_list);
    }

    max_sequential_handler_number = x_handler_sequential_number;
    hlist = handler_list_lookup(signal_id, instance);
    handler_list = hlist ? hlist->handlers : NULL;
    if (handler_list) {
        handler_ref(handler_list);
    }

    emission.ihint.run_type = (XSignalFlags)(X_SIGNAL_RUN_FIRST | X_SIGNAL_ACCUMULATOR_FIRST_RUN);
    if ((node->flags & X_SIGNAL_RUN_FIRST) && class_closure) {
        emission.state = (EmissionState)EMISSION_RUN;

        emission.chain_type = X_TYPE_FROM_INSTANCE(instance);
        SIGNAL_UNLOCK();
        return_accu = maybe_init_accumulator_unlocked(node, emission_return, &accu);
        x_closure_invoke(class_closure, return_accu, n_params, instance_and_params, &emission.ihint);
        if (!accumulate(&emission.ihint, emission_return, &accu, accumulator) && emission.state == EMISSION_RUN) {
            emission.state = EMISSION_STOP;
        }

        SIGNAL_LOCK();
        emission.chain_type = X_TYPE_NONE;
        return_value_altered = TRUE;

        if (emission.state == EMISSION_STOP) {
            goto EMIT_CLEANUP;
        } else if (emission.state == EMISSION_RESTART) {
            goto EMIT_RESTART;
        }
    }

    if (node->emission_hooks) {
        xuint i;
        XHook *hook;
        size_t n_emission_hooks = 0;
        XHook *static_emission_hooks[3];
        const xboolean may_recurse = TRUE;

        emission.state = EMISSION_HOOK;
        hook = x_hook_first_valid(node->emission_hooks, may_recurse);
        while (hook) {
            SignalHook *signal_hook = SIGNAL_HOOK(hook);

            if (!signal_hook->detail || signal_hook->detail == detail) {
                if (n_emission_hooks < X_N_ELEMENTS(static_emission_hooks)) {
                    static_emission_hooks[n_emission_hooks] = x_hook_ref(node->emission_hooks, hook);
                }

                n_emission_hooks += 1;
            }

            hook = x_hook_next_valid(node->emission_hooks, hook, may_recurse);
        }

        if X_UNLIKELY(n_emission_hooks > 0) {
            xuint8 *hook_returns = NULL;
            XHook **emission_hooks = NULL;
            xuint8 static_hook_returns[X_N_ELEMENTS(static_emission_hooks)];

            if X_LIKELY(n_emission_hooks <= X_N_ELEMENTS(static_emission_hooks)) {
                emission_hooks = static_emission_hooks;
                hook_returns = static_hook_returns;
            } else {
                emission_hooks = x_newa(XHook *, n_emission_hooks);
                hook_returns = x_newa(xuint8, n_emission_hooks);

                i = 0;
                for (hook = x_hook_first_valid(node->emission_hooks, may_recurse); hook != NULL; hook = x_hook_next_valid(node->emission_hooks, hook, may_recurse)) {
                    SignalHook *signal_hook = SIGNAL_HOOK(hook);

                    if (!signal_hook->detail || signal_hook->detail == detail) {
                        if (i < X_N_ELEMENTS(static_emission_hooks)) {
                            emission_hooks[i] = x_steal_pointer(&static_emission_hooks[i]);
                            x_assert(emission_hooks[i] == hook);
                        } else {
                            emission_hooks[i] = x_hook_ref(node->emission_hooks, hook);
                        }

                        i += 1;
                    }
                }

                x_assert(i == n_emission_hooks);
            }

            SIGNAL_UNLOCK();

            for (i = 0; i < n_emission_hooks; ++i) {
                xuint old_flags;
                xboolean need_destroy;
                XSignalEmissionHook hook_func;

                hook = emission_hooks[i];
                hook_func = (XSignalEmissionHook)hook->func;

                old_flags = x_atomic_int_or(&hook->flags, X_HOOK_FLAG_IN_CALL);
                need_destroy = !hook_func(&emission.ihint, n_params, instance_and_params, hook->data);

                if (!(old_flags & X_HOOK_FLAG_IN_CALL)) {
                    x_atomic_int_compare_and_exchange(&hook->flags, old_flags | X_HOOK_FLAG_IN_CALL, old_flags);
                }

                hook_returns[i] = !!need_destroy;
            }

            SIGNAL_LOCK();

            for (i = 0; i < n_emission_hooks; i++) {
                hook = emission_hooks[i];
                x_hook_unref(node->emission_hooks, hook);

                if (hook_returns[i]) {
                    x_hook_destroy_link(node->emission_hooks, hook);
                }
            }
        }

        if (emission.state == EMISSION_RESTART) {
            goto EMIT_RESTART;
        }
    }

    if (handler_list) {
        Handler *handler = handler_list;

        emission.state = EMISSION_RUN;
        handler_ref(handler);

        do {
            Handler *tmp;

            if (handler->after) {
                handler_unref_R(signal_id, instance, handler_list);
                handler_list = handler;
                break;
            } else if (!handler->block_count && (!handler->detail || handler->detail == detail) && handler->sequential_number < max_sequential_handler_number) {
                SIGNAL_UNLOCK();
                return_accu = maybe_init_accumulator_unlocked(node, emission_return, &accu);
                x_closure_invoke(handler->closure, return_accu, n_params, instance_and_params, &emission.ihint);
                if (!accumulate (&emission.ihint, emission_return, &accu, accumulator) && emission.state == EMISSION_RUN) {
                    emission.state = EMISSION_STOP;
                }

                SIGNAL_LOCK();
                return_value_altered = TRUE;
                tmp = emission.state == EMISSION_RUN ? handler->next : NULL;
            } else {
                tmp = handler->next;
            }

            if (tmp) {
                handler_ref(tmp);
            }

            handler_unref_R(signal_id, instance, handler_list);
            handler_list = handler;
            handler = tmp;
        } while (handler);

        if (emission.state == EMISSION_STOP) {
            goto EMIT_CLEANUP;
        } else if (emission.state == EMISSION_RESTART) {
            goto EMIT_RESTART;
        }
    }

    emission.ihint.run_type = (XSignalFlags)(emission.ihint.run_type & ~X_SIGNAL_RUN_FIRST);
    emission.ihint.run_type = (XSignalFlags)(emission.ihint.run_type | X_SIGNAL_RUN_LAST);

    if ((node->flags & X_SIGNAL_RUN_LAST) && class_closure) {
        emission.state = EMISSION_RUN;

        emission.chain_type = X_TYPE_FROM_INSTANCE(instance);
        SIGNAL_UNLOCK();
        return_accu = maybe_init_accumulator_unlocked(node, emission_return, &accu);
        x_closure_invoke(class_closure, return_accu, n_params, instance_and_params, &emission.ihint);
        if (!accumulate(&emission.ihint, emission_return, &accu, accumulator) && emission.state == EMISSION_RUN) {
            emission.state = EMISSION_STOP;
        }
        SIGNAL_LOCK();
        emission.chain_type = X_TYPE_NONE;
        return_value_altered = TRUE;

        if (emission.state == EMISSION_STOP) {
            goto EMIT_CLEANUP;
        } else if (emission.state == EMISSION_RESTART) {
            goto EMIT_RESTART;
        }
    }

    if (handler_list) {
        Handler *handler = handler_list;

        emission.state = EMISSION_RUN;
        handler_ref(handler);

        do {
            Handler *tmp;
        
            if (handler->after && !handler->block_count && (!handler->detail || handler->detail == detail) && handler->sequential_number < max_sequential_handler_number) {
                SIGNAL_UNLOCK();
                return_accu = maybe_init_accumulator_unlocked(node, emission_return, &accu);
                x_closure_invoke(handler->closure, return_accu, n_params, instance_and_params, &emission.ihint);
                if (!accumulate(&emission.ihint, emission_return, &accu, accumulator) && emission.state == EMISSION_RUN) {
                    emission.state = EMISSION_STOP;
                }
                SIGNAL_LOCK();
                return_value_altered = TRUE;

                tmp = emission.state == EMISSION_RUN ? handler->next : NULL;
            } else {
                tmp = handler->next;
            }

            if (tmp) {
                handler_ref(tmp);
            }

            handler_unref_R(signal_id, instance, handler);
            handler = tmp;
        } while (handler);

        if (emission.state == EMISSION_STOP) {
            goto EMIT_CLEANUP;
        } else if (emission.state == EMISSION_RESTART) {
            goto EMIT_RESTART;
        }
    }

EMIT_CLEANUP:
    emission.ihint.run_type = (XSignalFlags)(emission.ihint.run_type & ~X_SIGNAL_RUN_LAST);
    emission.ihint.run_type = (XSignalFlags)(emission.ihint.run_type | X_SIGNAL_RUN_CLEANUP);

    if ((node->flags & X_SIGNAL_RUN_CLEANUP) && class_closure) {
        xboolean need_unset = FALSE;

        emission.state = EMISSION_STOP;
        emission.chain_type = X_TYPE_FROM_INSTANCE(instance);
        SIGNAL_UNLOCK();
        if (node->return_type != X_TYPE_NONE && !accumulator) {
            x_value_init(&accu, node->return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE);
            need_unset = TRUE;
        }

        x_closure_invoke(class_closure, node->return_type != X_TYPE_NONE ? &accu : NULL, n_params, instance_and_params, &emission.ihint);
        if (!accumulate(&emission.ihint, emission_return, &accu, accumulator) && emission.state == EMISSION_RUN) {
            emission.state = EMISSION_STOP;
        }

        if (need_unset) {
            x_value_unset(&accu);
        }
        SIGNAL_LOCK();
        return_value_altered = TRUE;

        emission.chain_type = X_TYPE_NONE;
        if (emission.state == EMISSION_RESTART) {
            goto EMIT_RESTART;
        }
    }

    if (handler_list) {
        handler_unref_R(signal_id, instance, handler_list);
    }

    emission_pop(&emission);
    if (accumulator) {
        x_value_unset(&accu);
    }

    TRACE(XOBJECT_SIGNAL_EMIT_END(node->signal_id, detail, instance, X_TYPE_FROM_INSTANCE(instance)));
    return return_value_altered;
}

static void add_invalid_closure_notify (Handler *handler, xpointer instance)
{
    x_closure_add_invalidate_notifier(handler->closure, instance, invalid_closure_notify);
    handler->has_invalid_closure_notify = 1;
}

static void remove_invalid_closure_notify (Handler *handler, xpointer instance)
{
    if (handler->has_invalid_closure_notify) {
        x_closure_remove_invalidate_notifier(handler->closure, instance, invalid_closure_notify);
        handler->has_invalid_closure_notify = 0;
    }
}

static void invalid_closure_notify(xpointer instance, XClosure *closure)
{
    xuint signal_id;
    Handler *handler;

    SIGNAL_LOCK();
    handler = handler_lookup(instance, 0, closure, &signal_id);

    x_assert(handler != NULL);
    x_assert(handler->closure == closure);

    x_hash_table_remove(x_handlers, handler);
    handler->sequential_number = 0;
    handler->block_count = 1;
    handler_unref_R(signal_id, instance, handler);
    SIGNAL_UNLOCK();
}

static const xchar *type_debug_name(XType type)
{
    if (type) {
        const char *name = x_type_name(type & ~X_SIGNAL_TYPE_STATIC_SCOPE);
        return name ? name : "<unknown>";
    } else {
        return "<invalid>";
    }
}

xboolean x_signal_accumulator_true_handled(XSignalInvocationHint *ihint, XValue *return_accu, const XValue *handler_return, xpointer dummy)
{
    xboolean continue_emission;
    xboolean signal_handled;

    signal_handled = x_value_get_boolean(handler_return);
    x_value_set_boolean(return_accu, signal_handled);
    continue_emission = !signal_handled;

    return continue_emission;
}

xboolean x_signal_accumulator_first_wins(XSignalInvocationHint *ihint, XValue *return_accu, const XValue *handler_return, xpointer dummy)
{
    x_value_copy(handler_return, return_accu);
    return FALSE;
}

void (x_clear_signal_handler)(xulong *handler_id_ptr, xpointer  instance)
{
    x_return_if_fail(handler_id_ptr != NULL);

#ifndef x_clear_signal_handler
#error x_clear_signal_handler() macro is not defined
#endif

    x_clear_signal_handler(handler_id_ptr, instance);
}
