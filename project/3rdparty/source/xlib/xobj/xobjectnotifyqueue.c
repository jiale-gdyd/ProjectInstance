#ifndef __X_OBJECT_NOTIFY_QUEUE_H__
#define __X_OBJECT_NOTIFY_QUEUE_H__

#include <string.h>
#include <xlib/xlib/xlib-object.h>

X_BEGIN_DECLS

typedef struct _XObjectNotifyQueue XObjectNotifyQueue;
typedef struct _XObjectNotifyContext XObjectNotifyContext;

typedef void (*XObjectNotifyQueueDispatcher)(XObject *object, xuint n_pspecs, XParamSpec **pspecs);

struct _XObjectNotifyContext {
    XQuark                       quark_notify_queue;
    XObjectNotifyQueueDispatcher dispatcher;
    XTrashStack                  *_nqueue_trash;
};

struct _XObjectNotifyQueue {
    XObjectNotifyContext *context;
    XSList               *pspecs;
    xuint16               n_pspecs;
    xuint16               freeze_count;
};

X_LOCK_DEFINE_STATIC(notify_lock);

static void x_object_notify_queue_free(xpointer data)
{
    XObjectNotifyQueue *nqueue = data;

    x_slist_free(nqueue->pspecs);
    x_slice_free(XObjectNotifyQueue, nqueue);
}

static inline XObjectNotifyQueue *x_object_notify_queue_freeze(XObject *object, XObjectNotifyContext *context)
{
    XObjectNotifyQueue *nqueue;

    X_LOCK(notify_lock);
    nqueue = x_datalist_id_get_data(&object->qdata, context->quark_notify_queue);
    if (!nqueue) {
        nqueue = x_slice_new0(XObjectNotifyQueue);
        nqueue->context = context;
        x_datalist_id_set_data_full(&object->qdata, context->quark_notify_queue, nqueue, x_object_notify_queue_free);
    }

    if (nqueue->freeze_count >= 65535) {
        x_critical("Free queue for %s (%p) is larger than 65535,"
                " called x_object_freeze_notify() too often."
                " Forgot to call x_object_thaw_notify() or infinite loop",
                X_OBJECT_TYPE_NAME(object), object);
    } else {
        nqueue->freeze_count++;
    }
    X_UNLOCK(notify_lock);

    return nqueue;
}

static inline void x_object_notify_queue_thaw(XObject *object, XObjectNotifyQueue *nqueue)
{
    XSList *slist;
    xuint n_pspecs = 0;
    XObjectNotifyContext *context = nqueue->context;
    XParamSpec *pspecs_mem[16], **pspecs, **free_me = NULL;

    x_return_if_fail(nqueue->freeze_count > 0);
    x_return_if_fail(x_atomic_int_get(&object->ref_count) > 0);

    X_LOCK(notify_lock);

    if (X_UNLIKELY(nqueue->freeze_count == 0)) {
        X_UNLOCK(notify_lock);
        x_critical("%s: property-changed notification for %s(%p) is not frozen", X_STRFUNC, X_OBJECT_TYPE_NAME(object), object);
        return;
    }

    nqueue->freeze_count--;
    if (nqueue->freeze_count) {
        X_UNLOCK(notify_lock);
        return;
    }

    pspecs = nqueue->n_pspecs > 16 ? free_me = x_new(XParamSpec *, nqueue->n_pspecs) : pspecs_mem;
    for (slist = nqueue->pspecs; slist; slist = slist->next) {
        pspecs[n_pspecs++] = slist->data;
    }
    x_datalist_id_set_data (&object->qdata, context->quark_notify_queue, NULL);

    X_UNLOCK(notify_lock);

    if (n_pspecs) {
        context->dispatcher(object, n_pspecs, pspecs);
    }
    x_free(free_me);
}

static inline void x_object_notify_queue_clear(XObject *object X_GNUC_UNUSED, XObjectNotifyQueue *nqueue)
{
    x_return_if_fail(nqueue->freeze_count > 0);

    X_LOCK(notify_lock);

    x_slist_free(nqueue->pspecs);
    nqueue->pspecs = NULL;
    nqueue->n_pspecs = 0;

    X_UNLOCK(notify_lock);
}

static inline void x_object_notify_queue_add(XObject *object X_GNUC_UNUSED, XObjectNotifyQueue *nqueue, XParamSpec *pspec)
{
    if (pspec->flags & X_PARAM_READABLE) {
        XParamSpec *redirect;

        X_LOCK(notify_lock);

        x_return_if_fail(nqueue->n_pspecs < 65535);

        redirect = x_param_spec_get_redirect_target(pspec);
        if (redirect) {
            pspec = redirect;
        }

        if (x_slist_find(nqueue->pspecs, pspec) == NULL) {
            nqueue->pspecs = x_slist_prepend(nqueue->pspecs, pspec);
            nqueue->n_pspecs++;
        }

        X_UNLOCK(notify_lock);
    }
}

static inline XObjectNotifyQueue *x_object_notify_queue_from_object(XObject *object, XObjectNotifyContext *context)
{
    return x_datalist_id_get_data(&object->qdata, context->quark_notify_queue);
}

X_END_DECLS

#endif
