#include "acl/lib_fiber/stdafx.hpp"
#include "acl/lib_fiber/cpp/fiber_sem.hpp"

namespace acl {

fiber_sem::fiber_sem(int max, fiber_sem_attr_t attr)
{
    unsigned flags = 0;

    if (attr & fiber_sem_t_async) {
        flags |= ACL_FIBER_SEM_F_ASYNC;
    }

    sem_ = acl_fiber_sem_create2(max, flags);
}

fiber_sem::~fiber_sem(void)
{
    acl_fiber_sem_free(sem_);
}

int fiber_sem::wait(void)
{
    return acl_fiber_sem_wait(sem_);
}

int fiber_sem::trywait(void)
{
    return acl_fiber_sem_trywait(sem_);
}

int fiber_sem::post(void)
{
    return acl_fiber_sem_post(sem_);
}

size_t fiber_sem::num(void) const
{
    return (size_t) acl_fiber_sem_num(sem_);
}

}
