#ifndef __X_ASYNCQUEUEPRIVATE_H__
#define __X_ASYNCQUEUEPRIVATE_H__

#include "xasyncqueue.h"

X_BEGIN_DECLS

XMutex *_x_async_queue_get_mutex(XAsyncQueue *queue);

X_END_DECLS

#endif
