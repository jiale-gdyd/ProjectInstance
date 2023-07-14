#ifndef TBOX_NETWORK_BYTE_STREAM_H
#define TBOX_NETWORK_BYTE_STREAM_H

#include <cstddef>
#include <functional>

namespace tbox {
namespace network {

class Buffer;

//! 字节流接口
class ByteStream {
  public:
    //! 函数类型定义
    using ReceiveCallback = std::function<void(Buffer&)>;

    //! 设置接收到数据时的回调函数，threshold 为阈值
    virtual void setReceiveCallback(const ReceiveCallback &cb, size_t threshold = 0) = 0;

    //! 发送数据
    virtual bool send(const void *data_ptr, size_t data_size) = 0;

    //! 绑定一个数据接收者
    //! 当接收到的数据时，数据将流向receiver，作为其输出的数据
    virtual void bind(ByteStream *receiver) = 0;

    //! 解除绑定
    virtual void unbind() = 0;

  protected:
    virtual ~ByteStream() { }
};

}
}

#endif //TBOX_NETWORK_BYTE_STREAM_H_20171102
