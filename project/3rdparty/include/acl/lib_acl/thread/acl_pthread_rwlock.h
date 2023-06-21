#ifndef ACL_LIBACL_THREAD_ACL_PTHREAD_RWLOCK_H
#define ACL_LIBACL_THREAD_ACL_PTHREAD_RWLOCK_H

#include "../stdlib/acl_define.h"
#include "acl_pthread.h"

#ifdef ACL_HAVE_NO_RWLOCK

#if !defined(ACL_PTHREAD_PROCESS_PRIVATE)
#define ACL_PTHREAD_PROCESS_PRIVATE     0
#endif

#if !defined(ACL_PTHREAD_PROCESS_SHARED)
#define ACL_PTHREAD_PROCESS_SHARED      1
#endif

#if !defined(ACL_PTHREAD_RWLOCK_INITIALIZER)
#define ACL_PTHREAD_RWLOCK_INITIALIZER  NULL

struct acl_pthread_rwlock {
    acl_pthread_mutex_t lock; /* monitor lock acl_pthread_mutex_t */
    int   state;              /* 0 = idle  >0 = # of readers  -1 = writer */
    acl_pthread_cond_t read_signal;
    acl_pthread_cond_t write_signal;
    int   blocked_writers;
};

struct acl_pthread_rwlockattr {
    int pshared;
};

typedef struct acl_pthread_rwlock *acl_pthread_rwlock_t;
typedef struct acl_pthread_rwlockattr *acl_pthread_rwlockattr_t;

#if defined(__cplusplus)
extern "C" {
#endif

ACL_API int acl_pthread_rwlock_destroy(acl_pthread_rwlock_t *);
ACL_API int acl_pthread_rwlock_init(acl_pthread_rwlock_t *, const acl_pthread_rwlockattr_t *);
ACL_API int acl_pthread_rwlock_rdlock(acl_pthread_rwlock_t *);
ACL_API int acl_pthread_rwlock_tryrdlock(acl_pthread_rwlock_t *);
ACL_API int acl_pthread_rwlock_trywrlock(acl_pthread_rwlock_t *);
ACL_API int acl_pthread_rwlock_unlock(acl_pthread_rwlock_t *);
ACL_API int acl_pthread_rwlock_wrlock(acl_pthread_rwlock_t *);
ACL_API int acl_pthread_rwlockattr_init(acl_pthread_rwlockattr_t *);
ACL_API int acl_pthread_rwlockattr_getpshared(const acl_pthread_rwlockattr_t *, int *);
ACL_API int acl_pthread_rwlockattr_setpshared(acl_pthread_rwlockattr_t *, int);
ACL_API int acl_pthread_rwlockattr_destroy(acl_pthread_rwlockattr_t *);

#if defined(__cplusplus)
}
#endif

#endif

#endif

#endif
