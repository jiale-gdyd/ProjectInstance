#include <iostream>

#include <tbox/base/log.h>
#include <tbox/base/version.h>

#include <tbox/main/module.h>
#include <tbox/main/context.h>

namespace tbox {
namespace main {

std::function<void()> error_exit_func;  //!< 出错异常退出前要做的事件

void GetVersion(int &major, int &minor, int &rev, int &build)
{
    major = 1;
    minor = 0;
    rev   = 0;
    build = 0;
}

__attribute__((weak))
//! 定义为弱定义，默认运行时报错误提示，避免编译错误
void RegisterApps(Module &apps, Context &ctx)
{
    const char *src_text = R"(
#include <tbox/main/main.h>
#include "your_app.h"

namespace tbox {
namespace main {
void RegisterApps(Module &apps, Context &ctx)
{
    apps.add(new YourApp(ctx));
}
}
}
)";
    std::cerr << "WARN: You should implement tbox::main::RegisterApps().\nExp:" << std::endl
         << src_text << std::endl;

    (void)apps;
    (void)ctx;
}

__attribute__((weak))
std::string GetAppBuildTime()
{
    return "Unknown";
}

__attribute__((weak))
std::string GetAppDescribe()
{
    return "Author didn't specify";
}

__attribute__((weak))
void GetAppVersion(int &major, int &minor, int &rev, int &build)
{
    major = minor = rev = build = 0;
}

void SayHello()
{
    LogInfo("=== TBOX MAIN STARTUP ===");
    LogInfo("App Describe: %s", GetAppDescribe().c_str());

    int major, minor, rev, build;
    GetAppVersion(major, minor, rev, build);
    LogInfo("App  Version: %d.%d.%d_%d", major, minor, rev, build);

    GetTboxVersion(major, minor, rev);
    LogInfo("Tbox Version: %d.%d.%d", major, minor, rev);
}

}
}
