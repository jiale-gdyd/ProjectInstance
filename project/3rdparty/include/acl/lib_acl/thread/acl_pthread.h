#ifndef ACL_LIBACL_THREAD_ACL_PTHREAD_H
#define ACL_LIBACL_THREAD_ACL_PTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

#define ACL_MUTEX_MAXWAIT               (~(unsigned int)0)

#ifdef ACL_HAS_PTHREAD

#ifdef ACL_UNIX
#ifndef  _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <pthread.h>

#define ACL_PTHREAD_MUTEX_RECURSIVE     PTHREAD_MUTEX_RECURSIVE_NP

typedef pthread_t acl_pthread_t;
typedef pthread_attr_t acl_pthread_attr_t;
typedef pthread_mutex_t acl_pthread_mutex_t;
typedef pthread_cond_t acl_pthread_cond_t;
typedef pthread_mutexattr_t acl_pthread_mutexattr_t;
typedef pthread_condattr_t acl_pthread_condattr_t;
typedef pthread_key_t acl_pthread_key_t;
typedef pthread_once_t acl_pthread_once_t;

#define acl_pthread_attr_init           pthread_attr_init
#define acl_pthread_attr_setstacksize   pthread_attr_setstacksize
#define acl_pthread_attr_setdetachstate pthread_attr_setdetachstate
#define acl_pthread_attr_destroy        pthread_attr_destroy
#define acl_pthread_self                pthread_self
#define acl_pthread_create              pthread_create
#define acl_pthread_detach              pthread_detach
#define acl_pthread_once                pthread_once
#define acl_pthread_join                pthread_join
#define	acl_pthread_mutexattr_init	pthread_mutexattr_init
#define	acl_pthread_mutexattr_destroy	pthread_mutexattr_destroy
#define	acl_pthread_mutexattr_settype	pthread_mutexattr_settype
#define acl_pthread_mutex_init          pthread_mutex_init
#define acl_pthread_mutex_destroy       pthread_mutex_destroy
#define acl_pthread_mutex_lock          pthread_mutex_lock
#define acl_pthread_mutex_unlock        pthread_mutex_unlock
#define acl_pthread_mutex_trylock       pthread_mutex_trylock
#define acl_pthread_cond_init           pthread_cond_init
#define acl_pthread_cond_destroy        pthread_cond_destroy
#define acl_pthread_cond_signal         pthread_cond_signal
#define acl_pthread_cond_broadcast      pthread_cond_broadcast
#define acl_pthread_cond_timedwait      pthread_cond_timedwait
#define acl_pthread_cond_wait           pthread_cond_wait
#define	acl_pthread_key_create          pthread_key_create
#define	acl_pthread_getspecific         pthread_getspecific
#define	acl_pthread_setspecific         pthread_setspecific

#define ACL_PTHREAD_CREATE_DETACHED     PTHREAD_CREATE_DETACHED
#define ACL_PTHREAD_CREATE_JOINABLE     PTHREAD_CREATE_JOINABLE
#define ACL_TLS_OUT_OF_INDEXES          0xffffffff
#define ACL_PTHREAD_KEYS_MAX            PTHREAD_KEYS_MAX

#define ACL_PTHREAD_ONCE_INIT           PTHREAD_ONCE_INIT
#endif

ACL_API int acl_thread_mutex_lock(acl_pthread_mutex_t *mutex);
ACL_API int acl_thread_mutex_unlock(acl_pthread_mutex_t *mutex);
ACL_API int acl_thread_mutex_nested(acl_pthread_mutex_t *mutex);

ACL_API int acl_pthread_atexit_add(void *arg, void (*free_callback)(void*));
ACL_API int acl_pthread_atexit_remove(void *arg, void (*free_callback)(void*));

ACL_API int acl_pthread_tls_set_max(int max);
ACL_API int acl_pthread_tls_get_max(void);
ACL_API void *acl_pthread_tls_get(acl_pthread_key_t *key_ptr);
ACL_API int acl_pthread_tls_set(acl_pthread_key_t key, void *ptr, void (*free_fn)(void *));
ACL_API int acl_pthread_tls_del(acl_pthread_key_t key);
ACL_API void acl_pthread_tls_once_get(acl_pthread_once_t *control_once);
ACL_API void acl_pthread_tls_once_set(acl_pthread_once_t control_once);
ACL_API acl_pthread_key_t acl_pthread_tls_key_get(void);
ACL_API void acl_pthread_tls_key_set(acl_pthread_key_t key);

#ifdef __cplusplus
}
#endif

#endif
