#ifndef ACL_LIB_FIBER_FIBER_SEM_H
#define ACL_LIB_FIBER_FIBER_SEM_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fiber semaphore, thread unsafety, one semaphore can only be used in one
* thread, if used in different threads, result is unpredictable */

typedef struct ACL_FIBER_SEM ACL_FIBER_SEM;

/**
 * Create one fiber semaphore, and binding it with the current thread
 * @param num {int} the initial value of the semaphore, must >= 0
 * @param flags {unsigned} the flags defined as ACL_FIBER_SEM_F_XXX
 * @return {ACL_FIBER_SEM *}
 */
FIBER_API ACL_FIBER_SEM* acl_fiber_sem_create2(int num, unsigned flags);
#define ACL_FIBER_SEM_F_ASYNC   (1 << 0)    /* If notifying in async mode */

FIBER_API ACL_FIBER_SEM* acl_fiber_sem_create(int num);

/**
 * Free fiber semaphore
 * @param {ACL_FIBER_SEM *}
 */
FIBER_API void acl_fiber_sem_free(ACL_FIBER_SEM* sem);

/**
 * Get the thread binding the specificed fiber sem
 * @param sem {ACL_FIBER_SEM*} created by acl_fiber_sem_create
 * @return {unsigned long} thread ID of the thread binding the semaphore
 */
#if !defined(_WIN32) && !defined(_WIN64)
FIBER_API unsigned long acl_fiber_sem_get_tid(ACL_FIBER_SEM* sem);
#endif

/**
 * Set the thread ID the semaphore belongs to, changing the owner of the fiber
 * semaphore, when this function was called, the value of the semaphore must
 * be zero, otherwise fatal will happen.
 * @param sem {ACL_FIBER_SEM*} created by acl_fiber_sem_create
 * @param {unsigned long} the thread ID to be specificed with the semaphore
 */
FIBER_API void acl_fiber_sem_set_tid(ACL_FIBER_SEM* sem, unsigned long tid);

/**
 * Wait for semaphore until > 0, semaphore will be -1 when returned
 * @param sem {ACL_FIBER_SEM *} created by acl_fiber_sem_create
 * @return {int} the semaphore value returned, if the caller's thread isn't
 *  same as the semaphore owner's thread, -1 will be returned
 */
FIBER_API int acl_fiber_sem_wait(ACL_FIBER_SEM* sem);

/**
 * Try to wait semaphore until > 0, if semaphore is 0, -1 returned immediately,
 * otherwise semaphore will be decreased 1 and the semaphore's value is returned
 * @param sem {ACL_FIBER_SEM *} created by acl_fiber_sem_create
 * @return {int} value(>=0) returned when waiting ok, otherwise -1 will be
 *  returned if the caller's thread isn't same as the semaphore thread or the
 *  semaphore's value is 0
 */
FIBER_API int acl_fiber_sem_trywait(ACL_FIBER_SEM* sem);

/**
 * Add 1 to the semaphore, if there are other fibers waiting for semaphore,
 * one waiter will be wakeuped
 * @param sem {ACL_FIBER_SEM *} created by acl_fiber_sem_create
 * @return {int} the current semaphore value returned, -1 returned if the
 *  current thread ID is not same as the semaphore's owner ID
 */
FIBER_API int acl_fiber_sem_post(ACL_FIBER_SEM* sem);

/**
 * Get the specificed semaphore's value
 * @param sem {ACL_FIBER_SEM*} created by acl_fiber_sem_create
 * @return {int} current semaphore's value returned
 */
FIBER_API int acl_fiber_sem_num(ACL_FIBER_SEM* sem);

/**
 * Get the number of the waiters for the semaphore.
 * @param sem {ACL_FIBER_SEM*} created by acl_fiber_sem_create
 * @return {int} the waiters' number.
 */
FIBER_API int acl_fiber_sem_waiters_num(ACL_FIBER_SEM *sem);

#ifdef __cplusplus
}
#endif

#endif
