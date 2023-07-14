#ifndef ACL_LIB_FIBER_FIBER_COND_H
#define ACL_LIB_FIBER_FIBER_COND_H

#include "fiber_define.h"
#include "fiber_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* fiber_cond.h */

/**
 * Fiber_cond object look like pthread_cond_t which is used between threads
 * and fibers
 */
typedef struct ACL_FIBER_COND ACL_FIBER_COND;

/**
 * Create fiber cond which can be used in fibers more or threads mode
 * @param flag {unsigned} current not used, just for the future extend
 * @return {ACL_FIBER_COND *}
 */
FIBER_API ACL_FIBER_COND *acl_fiber_cond_create(unsigned flag);

/**
 * Free cond created by acl_fiber_cond_create
 * @param cond {ACL_FIBER_COND *}
 */
FIBER_API void acl_fiber_cond_free(ACL_FIBER_COND *cond);

/**
 * Wait for cond event to be signaled
 * @param cond {ACL_FIBER_COND *}
 * @param mutex {ACL_FIBER_MUTEX *} must be owned by the current caller
 * @return {int} return 0 if ok or return error value
 */
FIBER_API int acl_fiber_cond_wait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex);

/**
 * Wait for cond event to be signaled with the specified timeout
 * @param cond {ACL_FIBER_COND *}
 * @param mutex {ACL_FIBER_MUTEX *} must be owned by the current caller
 * @param delay_ms {int}
 * @return {int} return 0 if ok or return error value, when timedout ETIMEDOUT
 *  will be returned
 */
FIBER_API int acl_fiber_cond_timedwait(ACL_FIBER_COND *cond,
    ACL_FIBER_MUTEX *mutex, int delay_ms);

/**
 * Signal the cond which will wakeup one waiter for the cond to be signaled
 * @param cond {ACL_FIBER_COND *}
 * @return {int} return 0 if ok or return error value
 */
FIBER_API int acl_fiber_cond_signal(ACL_FIBER_COND *cond);

#ifdef __cplusplus
}
#endif

#endif // !defined(_WIN32) && !defined(_WIN64)
