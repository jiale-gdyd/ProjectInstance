#pragma once
#include "fiber_cpp_define.hpp"
#include <thread>
#include <functional>
#include "fiber.hpp"
#include "fiber_tbox.hpp"

// __cplusplus:
//    199711L (C++98 or C++03)
//    201103L (C++11)
//    201402L (C++14)
//    201703L (C++17)
//    202002L (C++20)

#if __cplusplus >= 201103L      // Support c++11 ?

struct ACL_FIBER;

namespace acl {

class fiber_ctx {
public:
    fiber_ctx(std::function<void()> fn) {
        fn_ = std::move(fn);
    }

    ~fiber_ctx() = default;

    std::function<void()> fn_;
};

#define	go                  acl::go_fiber()>
#define	go_stack(size)      acl::go_fiber(size, false)>
#define	go_share(size)      acl::go_fiber(size, true)>

#define	go_wait_fiber       acl::go_fiber()<
#define	go_wait_thread      acl::go_fiber()<<
#define	go_wait             go_wait_thread

class go_fiber {
public:
    go_fiber(void) {}
    go_fiber(size_t stack_size, bool on) : stack_size_(stack_size), stack_share_(on) {}

    ACL_FIBER* operator > (std::function<void()> fn) {
        fiber_ctx* ctx = new fiber_ctx(fn);
        return fiber::fiber_create(fiber_main, (void*) ctx, stack_size_, stack_share_);
    }

    void operator < (std::function<void()> fn) {
        fiber_tbox<int> box;

        go[&] {
            fn();
            box.push(NULL);
        };
        (void) box.pop();
    }

    void operator << (std::function<void()> fn) {
        fiber_tbox<int> box;

        std::thread thread([&]() {
            fn();
            box.push(NULL);
        });

        thread.detach();
        (void) box.pop();
    }

private:
    size_t stack_size_  = 320000;
    bool   stack_share_ = false;

    static void fiber_main(ACL_FIBER*, void* ctx) {
        fiber_ctx* fc = (fiber_ctx *) ctx;
        std::function<void()> fn = fc->fn_;
        delete fc;

        fn();
    }
};

} // namespace acl

#endif // __cplusplus >= 201103L
