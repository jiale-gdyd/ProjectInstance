#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_dbuf_pool.h"
#include "acl/lib_acl/stdlib/acl_ring.h"
#include "acl/lib_acl/stdlib/acl_mymalloc.h"
#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/thread/acl_pthread.h"
#endif

typedef struct acl_pthread_nested_mutex_t acl_pthread_nested_mutex_t;

struct acl_pthread_nested_mutex_t {
    acl_pthread_mutex_t *mutex;
    ACL_RING ring;
    int   nrefer;
    char *ptr;
};

static acl_pthread_key_t __header_key;

static void free_header(void *arg)
{
    ACL_RING *header_ptr = (ACL_RING*) arg;
    ACL_RING_ITER iter;
    acl_pthread_nested_mutex_t *tmp;

    if (header_ptr == NULL)
        return;

    acl_ring_foreach(iter, header_ptr) {
        tmp = ACL_RING_TO_APPL(iter.ptr,
            acl_pthread_nested_mutex_t, ring);
        if (tmp->mutex != NULL && tmp->nrefer > 0)
            acl_pthread_mutex_unlock(tmp->mutex);
    }
}

static void acl_thread_mutex_init_once(void)
{
    acl_pthread_key_create(&__header_key, free_header);
}

static acl_pthread_once_t thread_mutex_once_control = ACL_PTHREAD_ONCE_INIT;

int acl_thread_mutex_lock(acl_pthread_mutex_t *mutex)
{
    ACL_RING_ITER iter;
    acl_pthread_nested_mutex_t *tmp, *nested_mutex = NULL;
    ACL_RING *header_ptr;

    if (mutex == NULL)
        return -1;

    acl_pthread_once(&thread_mutex_once_control,
        acl_thread_mutex_init_once);

    header_ptr = acl_pthread_getspecific(__header_key);

    if (header_ptr == NULL) {
        header_ptr = (ACL_RING*) acl_mymalloc(sizeof(ACL_RING));
        acl_ring_init(header_ptr);
        acl_pthread_setspecific(__header_key, header_ptr);
    }

    acl_ring_foreach(iter, header_ptr) {
        tmp = ACL_RING_TO_APPL(iter.ptr,
            acl_pthread_nested_mutex_t, ring);
        if (tmp->mutex == mutex) {
            nested_mutex = tmp;
            break;
        }
    }

    if (nested_mutex == NULL) {
        nested_mutex = (acl_pthread_nested_mutex_t*)
            acl_mymalloc(sizeof(acl_pthread_nested_mutex_t));
        acl_pthread_mutex_lock(mutex);
        nested_mutex->mutex = mutex;
        nested_mutex->nrefer = 1;
        ACL_RING_APPEND(header_ptr, &nested_mutex->ring);
    } else
        nested_mutex->nrefer++;
    return 0;
}

int acl_thread_mutex_unlock(acl_pthread_mutex_t *mutex)
{
    ACL_RING_ITER iter;
    acl_pthread_nested_mutex_t *tmp, *nested_mutex = NULL;
    ACL_RING *header_ptr = acl_pthread_getspecific(__header_key);

    if (mutex == NULL || header_ptr == NULL)
        return -1;

    acl_ring_foreach(iter, header_ptr) {
        tmp = ACL_RING_TO_APPL(iter.ptr,
            acl_pthread_nested_mutex_t, ring);
        if (tmp->mutex == mutex) {
            nested_mutex = tmp;
            break;
        }
    }
    
    if (nested_mutex == NULL)
        return -1;
    if (--nested_mutex->nrefer == 0) {
        ACL_RING_DETACH(&nested_mutex->ring);
        acl_myfree(nested_mutex);
        acl_pthread_mutex_unlock(mutex);
    }

    return 0;
}

int acl_thread_mutex_nested(acl_pthread_mutex_t *mutex)
{
    ACL_RING_ITER iter;
    acl_pthread_nested_mutex_t *tmp, *nested_mutex = NULL;
    ACL_RING *header_ptr = acl_pthread_getspecific(__header_key);

    if (mutex == NULL || header_ptr == NULL)
        return -1;

    acl_ring_foreach(iter, header_ptr) {
        tmp = ACL_RING_TO_APPL(iter.ptr,
            acl_pthread_nested_mutex_t, ring);
        if (tmp->mutex == mutex) {
            nested_mutex = tmp;
            break;
        }
    }

    if (nested_mutex == NULL)
        return 0;
    return nested_mutex->nrefer;
}
