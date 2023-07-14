/**
 * 异步管道
 *
 * 在日志输出到文件的应用场景，如果每写一条日志都保存到文件，而写文件是非常耗时的
 * 阻塞性操作。如果不想阻塞，那写文件的操作必须要交给后台的线程去处理。
 * 本类就用于满足该场景的，使用方法：
 *
 * AsyncPipe async_pipe;
 * AsyncPipe::Config cfg;
 * cfg.sync_cb = \
 *     [](const void *data_ptr, size_t data_size) {
 *         //! 由后台线程调用，保存数据到文件的操作
 *     };
 * async_pipe.initialize(cfg);
 * async_pipe 对象会创建子线程，负责调用cfg.cb所指回调函数。
 *
 * 当有数据要保存的时候，只需要：
 * async_pipe.append(...);
 *
 * 在以下两种情况下 async_pipe 会回调函数：
 * 1）缓冲写满；2）距上次同步数据超过cfg.interval毫秒数
 *
 * 当对象被销毁或cleanup()时，会自动停止后台的线程，并将所有缓冲的数据同步调用预设置的回调
 */
#ifndef TBOX_UTIL_ASYNC_PIPLE_H
#define TBOX_UTIL_ASYNC_PIPLE_H

#include <cstddef>
#include <functional>

namespace tbox {
namespace util {

class AsyncPipe {
  public:
    AsyncPipe();
    ~AsyncPipe();

  public:
    using Callback = std::function<void(const void *, size_t)>;
    struct Config {
        size_t buff_size = 1024;    //!< 缓冲大小，默认1KB
        size_t buff_min_num = 2;    //!< 缓冲保留个数，默认2
        size_t buff_max_num = 10;   //!< 缓冲最大个数，默认5
        size_t interval = 1000;     //!< 同步间隔，单位ms，默认1秒
    };

    bool initialize(const Config &cfg);     //! 初始化
    void setCallback(const Callback &cb);   //! 设置回调

    void append(const void *data_ptr, size_t data_size); //! 异步写入
    void cleanup(); //! 清理

  private:
    class Impl;
    Impl *impl_ = nullptr;
};

}
}

#endif //TBOX_ASYNC_PIPLE_H_20211219
