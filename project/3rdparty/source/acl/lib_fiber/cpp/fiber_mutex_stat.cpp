#include "acl/lib_fiber/stdafx.hpp"
#include "acl/lib_fiber/cpp/fiber.hpp"
#include "acl/lib_fiber/cpp/fiber_mutex.hpp"
#include "acl/lib_fiber/cpp/fiber_mutex_stat.hpp"

namespace acl {

fiber_mutex_stats::~fiber_mutex_stats(void)
{
    for (std::vector<fiber_mutex_stat>::iterator it = stats.begin();
        it != stats.end(); ++it) {
        delete (*it).fb;
    }
}

} // namespace acl
