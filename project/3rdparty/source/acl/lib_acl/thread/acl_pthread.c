#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"

#include <errno.h>
#include <string.h>
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_malloc.h"
#include "acl/lib_acl/stdlib/acl_ring.h"
#include "acl/lib_acl/thread/acl_pthread.h"
#include "acl/lib_acl/init/acl_init.h"

#endif

#include "../private/private_fifo.h"

/*----------------- 跨平台的通用函数集，是 Posix 标准的扩展 ----------------*/

/*--------------------------------------------------------------------------*/

typedef struct pthread_atexit {
    void   (*free_fn)(void *);
    void   *arg;
} pthread_atexit_t;

static acl_pthread_key_t __pthread_atexit_key = (acl_pthread_key_t ) ACL_TLS_OUT_OF_INDEXES;
static acl_pthread_once_t __pthread_atexit_control_once = ACL_PTHREAD_ONCE_INIT;

static void pthread_atexit_done(void *arg) 
{
    ACL_FIFO *id_list = (ACL_FIFO*) arg;
    pthread_atexit_t *id_ptr;

    while (1) {
        id_ptr = (pthread_atexit_t*) private_fifo_pop(id_list);
        if (id_ptr == NULL)
            break;
        if (id_ptr->free_fn)
            id_ptr->free_fn(id_ptr->arg);
        acl_default_free(__FILE__, __LINE__, id_ptr);
    }
    private_fifo_free(id_list, NULL);
}

static void pthread_atexit_init(void)
{
    acl_pthread_key_create(&__pthread_atexit_key, pthread_atexit_done);
}

int acl_pthread_atexit_add(void *arg, void (*free_fn)(void *))
{
    const char *myname = "acl_pthread_atexit_add";
    pthread_atexit_t *id;
    ACL_FIFO *id_list;

    if (arg == NULL) {
        acl_set_error(ACL_EINVAL);
        return ACL_EINVAL;
    }
    acl_pthread_once(&__pthread_atexit_control_once, pthread_atexit_init);
    if (__pthread_atexit_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
        acl_msg_error("%s(%d): __pthread_atexit_key(%ld) invalid",
            myname, __LINE__, (long int) __pthread_atexit_key);
        return -1;
    }

    id = (pthread_atexit_t*) acl_default_malloc(__FILE__,
        __LINE__, sizeof(pthread_atexit_t));
    if (id == NULL) {
        acl_msg_error("%s(%d): malloc error(%s)",
            myname, __LINE__, acl_last_serror());
        acl_set_error(ACL_ENOMEM);
        return ACL_ENOMEM;
    }
    id->free_fn = free_fn;
    id->arg = arg;

    id_list = (ACL_FIFO*) acl_pthread_getspecific(__pthread_atexit_key);
    if (id_list == NULL) {
        id_list = private_fifo_new();
        if (acl_pthread_setspecific(__pthread_atexit_key, id_list) != 0) {
            acl_msg_error("%s(%d): pthread_setspecific: %s, key(%ld)",
                myname, __LINE__, acl_last_serror(),
                (long int) __pthread_atexit_key);
            return -1;
        }
    }
    private_fifo_push(id_list, id);
    return 0;
}

int acl_pthread_atexit_remove(void *arg, void (*free_fn)(void*))
{
    const char *myname = "acl_pthread_atexit_remove";
    ACL_FIFO *id_list;
    ACL_ITER iter;

    if (arg == NULL) {
        acl_set_error(ACL_EINVAL);
        return -1;
    }
    if (__pthread_atexit_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
        acl_msg_error("%s(%d): __pthread_atexit_key(%ld)  invalid",
            myname, __LINE__, (long int) __pthread_atexit_key);
        acl_set_error(ACL_EINVAL);
        return -1;
    }
    id_list = (ACL_FIFO*) acl_pthread_getspecific(__pthread_atexit_key);
    if (id_list == NULL) {
        acl_msg_error("%s(%d): __pthread_atexit_key(%ld) no exist"
            " in tid(%lu)", myname, __LINE__,
            (long int) __pthread_atexit_key,
            (unsigned long) acl_pthread_self());
        return -1;
    }

    acl_foreach(iter, id_list) {
        pthread_atexit_t *id_ptr = (pthread_atexit_t*) iter.data;

        if (id_ptr->free_fn == free_fn && id_ptr->arg == arg) {
            ACL_FIFO_INFO *id_info = acl_iter_info(iter, id_list);
            private_delete_info(id_list, id_info);
            acl_default_free(__FILE__, __LINE__, id_ptr);
            break;
        }
    }
    return 0;
}

/*----------------------------------------------------------------------------*/

typedef struct {
    acl_pthread_key_t key;
    void *ptr;
    void (*free_fn)(void*);
} TLS_CTX;

static int acl_tls_ctx_max = 1024;
static acl_pthread_once_t __tls_ctx_control_once = ACL_PTHREAD_ONCE_INIT;
static acl_pthread_key_t __tls_ctx_key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
static TLS_CTX *__main_tls_ctx = NULL;

int acl_pthread_tls_set_max(int max)
{
    if (max <= 0) {
        acl_set_error(ACL_EINVAL);
        return ACL_EINVAL;
    } else {
        acl_tls_ctx_max = max;
        return 0;
    }
}

int acl_pthread_tls_get_max(void)
{
    return acl_tls_ctx_max;
}

/* 线程退出时调用此函数释放属于本线程的局部变量 */

static void tls_ctx_free(void *ctx)
{
    TLS_CTX *tls_ctxes = (TLS_CTX*) ctx;
    int   i;

    for (i = 0; i < acl_tls_ctx_max; i++) {
        if (tls_ctxes[i].ptr != NULL && tls_ctxes[i].free_fn != NULL) {
            tls_ctxes[i].free_fn(tls_ctxes[i].ptr);
        }
    }
    acl_default_free(__FILE__, __LINE__, tls_ctxes);
}

/* 主线程退出时释放局部变量 */

#ifndef HAVE_NO_ATEXIT
static void main_tls_ctx_free(void)
{
    if (__main_tls_ctx)
        tls_ctx_free(__main_tls_ctx);
}
#endif

static void dummy_free(void *ctx acl_unused)
{
}

static void tls_ctx_once_init(void)
{
    if ((unsigned long) acl_pthread_self() ==
        (unsigned long) acl_main_thread_self())
    {
        acl_pthread_key_create(&__tls_ctx_key, dummy_free);
#ifndef HAVE_NO_ATEXIT
        atexit(main_tls_ctx_free);
#endif
    } else
        acl_pthread_key_create(&__tls_ctx_key, tls_ctx_free);
}

void *acl_pthread_tls_get(acl_pthread_key_t *key_ptr)
{
    const char *myname = "acl_pthread_tls_get";
    TLS_CTX *tls_ctxes;
    long  i;

    acl_pthread_once(&__tls_ctx_control_once, tls_ctx_once_init);
    if (__tls_ctx_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
        acl_msg_error("%s(%d): __tls_ctx_key invalid, tid(%lu)",
            myname, __LINE__, (unsigned long) acl_pthread_self());
        return NULL;
    }
    tls_ctxes = (TLS_CTX*) acl_pthread_getspecific(__tls_ctx_key);
    if (tls_ctxes == NULL) {
        /* 因为该线程中不存在该线程局部变量，所以需要分配一个新的 */
        tls_ctxes = (TLS_CTX*) acl_default_malloc(__FILE__, __LINE__,
                acl_tls_ctx_max * sizeof(TLS_CTX));
        if (acl_pthread_setspecific(__tls_ctx_key, tls_ctxes) != 0) {
            acl_default_free(__FILE__, __LINE__, tls_ctxes);
            acl_msg_error("%s(%d): pthread_setspecific: %s, tid(%lu)",
                myname, __LINE__, acl_last_serror(),
                (unsigned long) acl_pthread_self());
            return NULL;
        }
        /* 初始化 */
        for (i = 0; i < acl_tls_ctx_max; i++) {
            tls_ctxes[i].key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
            tls_ctxes[i].ptr = NULL;
            tls_ctxes[i].free_fn = NULL;
        }

        if ((unsigned long) acl_pthread_self()
            == (unsigned long) acl_main_thread_self())
        {
            __main_tls_ctx = tls_ctxes;
        }
    }

    /* 如果该键已经存在则取出对应数据 */
    if ((long) (*key_ptr) > 0 && (long) (*key_ptr) < acl_tls_ctx_max) {
        if (tls_ctxes[(long) (*key_ptr)].key == *key_ptr)
            return tls_ctxes[(long) (*key_ptr)].ptr;
        if (tls_ctxes[(long) (*key_ptr)].key
            == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES)
        {
            tls_ctxes[(long) (*key_ptr)].key = *key_ptr;
            return tls_ctxes[(long) (*key_ptr)].ptr;
        }
        acl_msg_warn("%s(%d): tls_ctxes[%ld].key(%ld)!= key(%ld)",
            myname, __LINE__, (long) (*key_ptr),
            (long) tls_ctxes[(long) (*key_ptr)].key, (long) (*key_ptr));
        return NULL;
    }

    /* 找出一个空位 */
    for (i = 0; i < acl_tls_ctx_max; i++) {
        if (tls_ctxes[i].key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES)
            break;
    }

    /* 如果没有空位可用则返回空并置错误标志位 */
    if (i == acl_tls_ctx_max) {
        acl_msg_error("%s(%d): no space for tls key", myname, __LINE__);
        *key_ptr = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
        acl_set_error(ACL_ENOMEM);
        return NULL;
    }

    /* 为新分配的键初始化线程局部数据对象 */
    tls_ctxes[i].key = (acl_pthread_key_t) i;
    tls_ctxes[i].free_fn = NULL;
    tls_ctxes[i].ptr = NULL;
    *key_ptr = (acl_pthread_key_t) i;
    return NULL;
}

int acl_pthread_tls_set(acl_pthread_key_t key, void *ptr,
    void (*free_fn)(void *))
{
    const char *myname = "acl_pthread_tls_set";
    TLS_CTX *tls_ctxes;

    if ((long) key >= acl_tls_ctx_max) {
        acl_msg_error("%s(%d): key(%ld) invalid",
            myname, __LINE__, (long) key);
        acl_set_error(ACL_EINVAL);
        return ACL_EINVAL;
    }

    if (__tls_ctx_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
        acl_msg_error("%s(%d): __tls_ctx_key invalid, tid(%lu)",
            myname, __LINE__, (unsigned long) acl_pthread_self());
        acl_set_error(ACL_ENOMEM);
        return ACL_ENOMEM;
    }
    tls_ctxes = (TLS_CTX*) acl_pthread_getspecific(__tls_ctx_key);
    if (tls_ctxes == NULL) {
        acl_msg_error("%s(%d): __tls_ctx_key(%ld) no exist",
            myname, __LINE__, (long) __tls_ctx_key);
        return -1;
    }
    if (tls_ctxes[(long) key].key != key) {
        acl_msg_error("%s(%d): key(%ld) invalid",
            myname, __LINE__, (long) key);
        acl_set_error(ACL_EINVAL);
        return ACL_EINVAL;
    }
    /* 如果该键值存在旧数据则首先需要释放掉旧数据 */
    if (tls_ctxes[(long) key].ptr != NULL && tls_ctxes[(long) key].free_fn != NULL)
        tls_ctxes[(long) key].free_fn(tls_ctxes[(long) key].ptr);

    tls_ctxes[(long) key].free_fn = free_fn;
    tls_ctxes[(long) key].ptr = ptr;
    return 0;
}

int acl_pthread_tls_del(acl_pthread_key_t key)
{
    const char *myname = "acl_pthread_tls_del";
    TLS_CTX *tls_ctxes;

    if ((long) key >= acl_tls_ctx_max) {
        acl_msg_error("%s(%d): key(%ld) invalid",
            myname, __LINE__, (long) key);
        acl_set_error(ACL_EINVAL);
        return ACL_EINVAL;
    }

    if (__tls_ctx_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
        acl_msg_error("%s(%d): __tls_ctx_key invalid, tid(%lu)",
            myname, __LINE__, (unsigned long) acl_pthread_self());
        acl_set_error(ACL_ENOMEM);
        return ACL_ENOMEM;
    }

    tls_ctxes = (TLS_CTX*) acl_pthread_getspecific(__tls_ctx_key);
    if (tls_ctxes == NULL) {
        acl_msg_error("%s(%d): __tls_ctx_key(%ld) no exist",
            myname, __LINE__, (long int) __tls_ctx_key);
        return -1;
    }

    if (tls_ctxes[(long) key].key != key) {
        acl_msg_error("%s(%d): key(%ld) invalid",
            myname, __LINE__, (long int) key);
        acl_set_error(ACL_EINVAL);
        return ACL_EINVAL;
    }

    tls_ctxes[(long) key].free_fn = NULL;
    tls_ctxes[(long) key].ptr = NULL;
    tls_ctxes[(long) key].key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
    return 0;
}

void acl_pthread_tls_once_get(acl_pthread_once_t *control_once)
{
    memcpy((void *)control_once, (void *)&__tls_ctx_control_once,
        sizeof(acl_pthread_once_t));
}

void acl_pthread_tls_once_set(acl_pthread_once_t control_once)
{
    __tls_ctx_control_once = control_once;
}

acl_pthread_key_t acl_pthread_tls_key_get(void)
{
    return __tls_ctx_key;
}

void acl_pthread_tls_key_set(acl_pthread_key_t key)
{
    __tls_ctx_key = key;
}
