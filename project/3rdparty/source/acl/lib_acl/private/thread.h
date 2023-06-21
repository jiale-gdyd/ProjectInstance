#ifndef ACL_LIB_ACL_PRIVATE_THREAD_H
#define ACL_LIB_ACL_PRIVATE_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl/lib_acl/stdlib/acl_define.h"
#include "acl/lib_acl/thread/acl_pthread.h"

#ifdef ACL_HAS_PTHREAD
int thread_mutex_destroy(acl_pthread_mutex_t *mutex);
# define thread_mutex_init pthread_mutex_init
# define thread_mutex_lock pthread_mutex_lock
# define thread_mutex_unlock pthread_mutex_unlock
#else

/* in thread_mutex.c */
int thread_mutex_destroy(acl_pthread_mutex_t *mutex);
int thread_mutex_init(acl_pthread_mutex_t *mutex, const acl_pthread_mutexattr_t *mattr);
int thread_mutex_lock(acl_pthread_mutex_t *mutex);
int thread_mutex_unlock(acl_pthread_mutex_t *mutex);

#define	thread_mutex_trylock thread_mutex_lock

#endif  /* ACL_HAS_PTHREAD */

/* general functions */

/* in acl_pthread_mutex.c */
acl_pthread_mutex_t *thread_mutex_create(void);

#ifdef __cplusplus
}
#endif

#endif
