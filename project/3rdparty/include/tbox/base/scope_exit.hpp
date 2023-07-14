#ifndef TBOX_BASE_SCOPE_EXIT_HPP
#define TBOX_BASE_SCOPE_EXIT_HPP

#include <functional>
#include "defines.h"

namespace tbox {

//! 退出区域的时候执行的动作
class ScopeExitActionGuard {
    using ScopeExitFunc = std::function<void()>;
    ScopeExitFunc func_;

  public:
    ScopeExitActionGuard(const ScopeExitFunc &func) : func_(func) { }

    NONCOPYABLE(ScopeExitActionGuard);
    IMMOVABLE(ScopeExitActionGuard);

    ~ScopeExitActionGuard() { if (func_) func_(); }

    void cancel() { func_ = nullptr; }
};

}

#define _ScopeExitActionName_1(line) tbox::ScopeExitActionGuard _scope_exit_action_guard_##line
#define _ScopeExitActionName_0(line) _ScopeExitActionName_1(line)

#define SetScopeExitAction(...) _ScopeExitActionName_0(__LINE__) (__VA_ARGS__)

#endif //TBOX_BASE_SCOPE_EXIT_H_20171104
