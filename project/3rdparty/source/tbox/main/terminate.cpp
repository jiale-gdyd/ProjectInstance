#include <cxxabi.h>
#include <signal.h>

#include <functional>
#include <exception>
#include <tbox/base/log.h>
#include <tbox/base/backtrace.h>

int tbox_main(int argc, char **argv);

namespace tbox {
namespace main {

extern std::function<void()> error_exit_func;

namespace {

void Terminate()
{
    static bool terminating = false;
    if (terminating) {
        LogFatal("terminate called recursively");
        std::abort();
    }
    terminating = true;

    // Make sure there was an exception; terminate is also called for an
    // attempt to rethrow when there is no suitable exception.
    std::type_info *t = abi::__cxa_current_exception_type();
    if (t) {
        // Note that "name" is the mangled name.
        const char *name = t->name();
        const char *readable_name = name;
        int status = -1;
        char *demangled = abi::__cxa_demangle(name, 0, 0, &status);
        if (demangled != nullptr)
            readable_name = demangled;

        LogFatal("terminate called after throwing an instance of '%s'", readable_name);

        if (status == 0)
            free(demangled);

        // If the exception is derived from std::exception, we can
        // give more information.
        __try { __throw_exception_again; }
#ifdef __EXCEPTIONS
        __catch(const std::exception& e) {
            LogFatal("what(): %s", e.what());
        }
#endif
        __catch(...) { }
    } else {
        LogFatal("terminate called without an active exception");
    }

#if 1
    const std::string &stack_str = DumpBacktrace();
    LogFatal("tbox_main: <%p>\n-----call stack-----\n%s", ::tbox_main, stack_str.c_str());
#endif

    LogFatal("Process abort!");

    if (error_exit_func)    //! 执行一些善后处理
        error_exit_func();

    signal(SIGABRT, SIG_DFL);
    std::abort();
}

}

void InstallTerminate()
{
    std::set_terminate(Terminate);
}

}
}
