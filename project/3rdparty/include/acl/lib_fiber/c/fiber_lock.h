#ifndef ACL_LIB_FIBER_FIBER_LOCK_H
#define ACL_LIB_FIBER_FIBER_LOCK_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fiber locking */

/**
 * Fiber mutex, thread unsafety, one fiber mutex can only be used in the
 * same thread, otherwise the result is unpredictable
 */
typedef struct ACL_FIBER_LOCK ACL_FIBER_LOCK;

/**
 * Fiber read/write mutex, thread unsafety, can only be used in the sampe thread
 */
typedef struct ACL_FIBER_RWLOCK ACL_FIBER_RWLOCK;

/**
 * Create one fiber mutex, can only be used in the same thread
 * @return {ACL_FIBER_LOCK*} fiber mutex returned
 */
FIBER_API ACL_FIBER_LOCK* acl_fiber_lock_create(void);

/**
 * Free fiber mutex created by acl_fiber_lock_create
 * @param l {ACL_FIBER_LOCK*} created by acl_fiber_lock_create
 */
FIBER_API void acl_fiber_lock_free(ACL_FIBER_LOCK* l);

/**
 * Lock the specified fiber mutex, return immediately when locked, or will
 * wait until the mutex can be used
 * @param l {ACL_FIBER_LOCK*} created by acl_fiber_lock_create
 * @return {int} successful when return 0, or error return -1
 */
FIBER_API int acl_fiber_lock_lock(ACL_FIBER_LOCK* l);

/**
 * Try lock the specified fiber mutex, return immediately no matter the mutex
 * can be locked.
 * @param l {ACL_FIBER_LOCK*} created by acl_fiber_lock_create
 * @return {int} 0 returned when locking successfully, -1 when locking failed
 */
FIBER_API int acl_fiber_lock_trylock(ACL_FIBER_LOCK* l);

/**
 * The fiber mutex be unlock by its owner fiber, fatal will happen when others
 * release the fiber mutex
 * @param l {ACL_FIBER_LOCK*} created by acl_fiber_lock_create
 */
FIBER_API void acl_fiber_lock_unlock(ACL_FIBER_LOCK* l);

/****************************************************************************/

/**
 * Create one fiber rwlock, can only be operated in the sampe thread
 * @return {ACL_FIBER_RWLOCK*}
 */
FIBER_API ACL_FIBER_RWLOCK* acl_fiber_rwlock_create(void);

/**
 * Free rw mutex created by acl_fiber_rwlock_create
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_free(ACL_FIBER_RWLOCK* l);

/**
 * Lock the rwlock, if there is no any write locking on it, the
 * function will return immediately; otherwise, the caller will wait for all
 * write locking be released. Read lock on it will successful when returning
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 * @return {int} successful when return 0, or error if return -1
 */
FIBER_API int acl_fiber_rwlock_rlock(ACL_FIBER_RWLOCK* l);

/**
 * Try to locking the Readonly lock, return immediately no matter locking
 * is successful.
 * @param l {ACL_FIBER_RWLOCK*} crated by acl_fiber_rwlock_create
 * @retur {int} 0 returned when successfully locked, or -1 returned if locking
 *  operation is failed.
 */
FIBER_API int acl_fiber_rwlock_tryrlock(ACL_FIBER_RWLOCK* l);

/**
 * Lock the rwlock in Write Lock mode, return until no any one locking it
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 * @return {int} return 0 if successful, -1 if error.
 */
FIBER_API int acl_fiber_rwlock_wlock(ACL_FIBER_RWLOCK* l);

/**
 * Try to lock the rwlock in Write Lock mode. return immediately no matter
 * locking is successful.
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 * @return {int} 0 returned when locking successfully, or -1 returned when
 *  locking failed
 */
FIBER_API int acl_fiber_rwlock_trywlock(ACL_FIBER_RWLOCK* l);

/**
 * The rwlock's Read-Lock owner unlock the rwlock
 * @param l {ACL_FIBER_RWLOCK*} crated by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_runlock(ACL_FIBER_RWLOCK* l);

/**
 * The rwlock's Write-Lock owner unlock the rwlock
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_wunlock(ACL_FIBER_RWLOCK* l);

#ifdef __cplusplus
}
#endif

#endif
