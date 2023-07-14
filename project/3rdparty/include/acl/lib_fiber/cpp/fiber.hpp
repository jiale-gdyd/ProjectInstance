#pragma once
#include <stddef.h>
#include <string>
#include <vector>
#include "fiber_cpp_define.hpp"

struct ACL_FIBER;

namespace acl {

typedef enum {
    FIBER_EVENT_T_KERNEL,   // Linux: epoll, FreeBSD: kquque, Windows: iocp
    FIBER_EVENT_T_POLL,     // Linux, FreeBSD, MacOS, Windows
    FIBER_EVENT_T_SELECT,   // Linux, FreeBSD, MacOS, Windows
    FIBER_EVENT_T_WMSG,     // Windows
    FIBER_EVENT_T_IO_URING, // Linux
} fiber_event_t;

struct FIBER_CPP_API fiber_frame {
    fiber_frame(void) : pc(0), off(0) {}
    ~fiber_frame(void) {}

    std::string func;
    long pc;
    long off;
};

/**
 * 协程类定义，纯虚类，需要子类继承并实现纯虚方法
 */
class FIBER_CPP_API fiber {
public:
    fiber(void);
    fiber(ACL_FIBER *fb);

    /**
     * 构造函数
     * @param running {bool} 当为 true 时，则表示当前协程已启动，仅是声明
     *  了一个协程对象而已，以便于与 ACL_FIBER 对象绑定，此时禁止调用本对
     *  象的 start 方法启动新协程; 当为 false 时，则需要调用 start 方法来
     *  启动新协程
     */
    fiber(bool running);

    virtual ~fiber(void);

    /**
     * 在创建一个协程类对象且构造参数 running 为 false 时，需要本函数启动
     * 协程，然后子类的重载的 run 方法将被回调，如果 running 为 true 时，
     * 则禁止调用 start 方法
     * @param stack_size {size_t} 创建的协程对象的栈大小
     * @param share_stack {bool} 是否采用共享栈方式(若要采用共享栈方式，
     *  必须在编译 libfiber.a 时将编译开关 SHARE_STACK 打开)
     */
    void start(size_t stack_size = 320000, bool share_stack = false);

    /**
     * 在本协程运行时调用此函数通知该协程退出
     * @return {bool} 返回 false 表示本协程未启动或已经退出
     */
    bool kill(void);

    /**
     * 判断当前协程是否被通知退出
     * @return {bool} 本协程是否被通知退出
     */
    bool killed(void) const;

    /**
     * 判断当前正在运行的协程是否被通知退出，该方法与 killed 的区别为，
     * killed 首先必须有 acl::fiber 对象依托，且该协程对象有可能正在运行，
     * 也有可能被挂起，而 self_killed 不需要 acl::fiber 对象依托且一定表示
     * 当前正在运行的协程
     * @return {bool}
     */
    static bool self_killed(void);

    /**
     * 获得本协程对象的 ID 号
     * @return {unsigned int}
     */
    unsigned int get_id(void) const;

    /**
     * 获得当前运行的协程对象的 ID 号
     * @return {unsigned int}
     */
    static unsigned int self(void);

    /**
     * 获得指定协程对象的ID号
     * @return {unsigned int}
     */
    static unsigned int fiber_id(const fiber& fb);

    /**
     * 获得当前协程在执行某个系统 API 出错时的错误号
     * return {int}
     */
    int get_errno(void) const;

    /**
     * 设置当前协程的错误号
     * @param errnum {int}
     */
    void set_errno(int errnum);

    /**
     * 清除当前协程的错误号及标记位
     */
    static void clear(void);

public:
    /**
     * 获得本次操作的出错信息
     * @return {const char*}
     */
    static const char* last_serror(void);

    /**
     * 获得本次操作的出错号
     * @return {int}
     */
    static int last_error(void);

    /**
     * 将所给错误号转成描述信息
     * @param errnum {int} 错误号
     * @param buf {char*} 存储结果
     * @param size {size_t} buf 空间大小
     * @return {const char*} buf 地址
     */
    static const char* strerror(int errnum, char* buf, size_t size);

    /**
     * 将错误信息输出至标准输出
     * @param on {bool} 为 true 时，内部出错信息将输出至标准输出
     */
    static void stdout_open(bool on);

    /**
     * 设置本进程最大可创建的句柄数量
     * @param max {int} >= 0 时有效
     * @return {int} 返回当前可用的最大句柄数量
     */
    static int set_fdlimit(int max);

    /**
     * 显式设置协程调度事件引擎类型，同时设置协程调度器为自启动模式，即当
     * 创建协程后不必显式调用 schedule 或 schedule_with 来启动协程调度器
     * @param type {fiber_event_t} 事件引擎类型，参见：FIBER_EVENT_T_XXX
     * @param schedule_auto {bool} 若为 true，则创建协程对象后并运行该协程
     *  对象后不必显式调用 schedule/schedule_with 来启动所有的协程过程，内
     *  部会自动启动协程调度器；否则，在创建并启动协程后，必须显式地调用
     *  schedule 或 schedule_with 方式来启动协程调度器以运行所的协程过程；
     *  内部缺省状态为 false
     */
    static void init(fiber_event_t type, bool schedule_auto = false);

    /**
     * 启动协程运行的调度过程
     * @param type {fiber_event_t} 事件引擎类型，参见：FIBER_EVENT_T_XXX
     */
    static void schedule(fiber_event_t type = FIBER_EVENT_T_KERNEL);

    /**
     * 启动协程调度时指定事件引擎类型，调用本方法等于同时调用了 schedule_init
     * 及 schedule 两个方法
     * @param type {fiber_event_t} 事件引擎类型，参见：FIBER_EVENT_T_XXX
     */
    static void schedule_with(fiber_event_t type);

    /**
     * 判断当前线程是否处于协程调度状态
     * @return {bool}
     */
    static bool scheduled(void);

    /**
     *  停止协程调度过程
     */
    static void schedule_stop(void);

public:
    /**
     * 将当前正在运行的协程(即本协程) 挂起
     */
    static void yield(void);

    /**
     * 挂起当前协程，执行等待队列中的下一个协程
     */
    static void switch_to_next(void);

    /**
     * 将指定协程对象置入待运行队列中
     * @param f {fiber&}
     */
    static void ready(fiber& f);

    /**
     * 使当前运行的协程休眠指定毫秒数
     * @param milliseconds {unsigned int} 指定要休眠的毫秒数
     * @return {unsigned int} 本协程休眠后再次被唤醒后剩余的毫秒数
     */
    static unsigned int delay(unsigned int milliseconds);

    /**
     * 获得处于存活状态的协程数量
     * @return {unsigned}
     */
    static unsigned alive_number(void);

    /**
     * 获得处于退出状态的协程对象数量
     * @return {unsigned}
     */
    static unsigned dead_number(void);

    /**
     * 设置本线程中所有协程在连接服务端时都采用了带超时的非阻塞方式（仅限Windows)
     * @param yes {bool}
     */
    static void set_non_blocking(bool yes);

    /**
     * 在启用共享栈模式下设置共享栈的大小,内部缺省值为 1024000 字节
     * @param size {size_t} 共享栈内存大小
     */
    static void set_shared_stack_size(size_t size);

    /**
     * 在启用共享栈模式下获得共享栈大小
     * @return {size_t} 如果返回 0 则表示未启用共享栈方式
     */
    static size_t get_shared_stack_size(void);

    /**
     * 显式调用本函数使 acl 基础库的 IO 过程协程化，在 UNIX 平台下不必显式
     * 调用本函数，因为内部会自动 HOOK IO API
     */
    static void acl_io_hook(void);

    /**
     * 调用本函数取消 acl基础库中的 IO 协程化
     */
    static void acl_io_unlock(void);

    /**
     * Windows 平台下可以显式地调用此函数 Hook 一些与网络协程相关的系统 API
     * @return {bool}
     */
    static bool winapi_hook(void);

    /**
     * 获得当前系统级错误号
     * @return {int}
     */
    static int  get_sys_errno(void);

    /**
     * 设置当前系统级错误号
     * @param errnum {int}
     */
    static void set_sys_errno(int errnum);

public:
    /**
     * 返回本协程对象对应的 C 语言的协程对象
     * @return {ACL_FIBER* }
     */
    ACL_FIBER* get_fiber(void) const;

    /**
     * 底层调用 C API 创建协程
     * @param fn {void (*)(ACL_FIBER*, void*)} 协程函数执行入口
     * @param ctx {void*} 传递给协程执行函数的参数
     * @param size {size_t} 协程栈大小
     * @param share_stack {bool} 是否创建共享栈协程
     * @return {ACL_FIBER*}
     */
    static ACL_FIBER* fiber_create(void (*fn)(ACL_FIBER*, void*),
            void* ctx, size_t size, bool share_stack = false);

    /**
     * 获得指定协程的堆栈
     * @param fb {const fiber&}
     * @param out {std::vector<stack_frame>&} 存放结果数据
     * @param max {size_t} 指定获取栈的最大深度
     */
    static void stacktrace(const fiber& fb, std::vector<fiber_frame>& out,
            size_t max = 50);

    /**
     * 输出指定协程的栈至标准输出
     * @param fb {const fiber&}
     * @param max {size_t} 可以显示栈的最大深度
     */
    static void stackshow(const fiber& fb, size_t max = 50);

protected:
    /**
     * 虚函数，子类须实现本函数，当通过调用 start 方法启动协程后，本
     * 虚函数将会被调用，从而通知子类协程已启动; 如果在构造函数中的参数
     * running 为 true ，则 start 将被禁止调用，故本虚方法也不会被调用
     */
    virtual void run(void);

private:
    ACL_FIBER* f_;

    fiber(const fiber&);
    void operator = (const fiber&);

    static void fiber_callback(ACL_FIBER* f, void* ctx);
};

/**
 * 可用作定时器的协程类
 */
class FIBER_CPP_API fiber_timer {
public:
    fiber_timer(void);
    virtual ~fiber_timer(void) {}

    /**
     * 启动一个协程定时器
     * @param milliseconds {unsigned int} 毫秒级时间
     * @param stack_size {size_t} 协程的栈空间大小
     */
    void start(unsigned int milliseconds, size_t stack_size = 320000);

protected:
    /**
     * 子类必须实现该纯虚方法，当定时器启动时会回调该方法
     */
    virtual void run(void) = 0;

private:
    ACL_FIBER* f_;

    fiber_timer(const fiber_timer&);
    void operator = (const fiber_timer&);

    static void timer_callback(ACL_FIBER* f, void* ctx);
};

#if defined(ACL_CPP_API)

/**
 * 定时器管理协程
 */
template <typename T>
class fiber_trigger : public fiber {
public:
    fiber_trigger(timer_trigger<T>& timer)
    : delay_(100)
    , stop_(false)
    , timer_(timer)
    {
    }

    virtual ~fiber_trigger(void) {}

    void add(T* o) {
        mbox_.push(o);
    }

    void del(T* o) {
        timer_.del(o);
    }

    timer_trigger<T>& get_trigger(void) {
        return timer_;
    }

    // @override
    void run(void) {
        while (!stop_) {
            T* o = mbox_.pop(delay_);
            if (o) {
                timer_.add(o);
            }

            long long next = timer_.trigger();
            long long curr = get_curr_stamp();
            if (next == -1) {
                delay_ = 100;
            } else {
                delay_ = next - curr;
                if (delay_ <= 0) {
                    delay_ = 1;
                }
            }
        }
    }

private:
    long long delay_;
    bool stop_;

    timer_trigger<T>& timer_;
    mbox<T> mbox_;
};

#endif // ACL_CPP_API

} // namespace acl
