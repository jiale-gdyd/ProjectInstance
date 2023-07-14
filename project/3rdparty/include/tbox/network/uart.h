#ifndef TBOX_NETWORK_UART_H
#define TBOX_NETWORK_UART_H

#include "../base/defines.h"
#include "../event/loop.h"

#include "byte_stream.h"
#include "buffered_fd.h"

namespace tbox {
namespace network {

class Uart : public ByteStream {
  public:
    explicit Uart(event::Loop *wp_loop);
    virtual ~Uart() { }

    NONCOPYABLE(Uart);
    IMMOVABLE(Uart);

  public:
    enum class DataBit { k8bits, k7bits };  //!< 数据位数
    enum class StopBit { k1bits, k2bits };  //!< 停止位数
    enum class ParityEnd { kNoEnd, kOddEnd, kEvenEnd }; //!< 校验位

    //! 串口配置
    struct Mode {
        int baudrate = 115200;  //!< 波特率
        DataBit data_bit = DataBit::k8bits;     //!< 数据位
        ParityEnd parity = ParityEnd::kNoEnd;   //!< 校验位
        StopBit stop_bit = StopBit::k1bits;     //!< 停止位
    };

    //! 打开串口设备文件，并配置模式
    bool initialize(const std::string &dev, const std::string &mode);
    bool initialize(const std::string &dev, const Mode &mode);

  public:
    //! 实现ByteStream的接口
    virtual void setReceiveCallback(const ReceiveCallback &cb, size_t threshold) override;
    virtual bool send(const void *data_ptr, size_t data_size) override;
    virtual void bind(ByteStream *receiver) override;
    virtual void unbind() override;

  public:
    bool enable();
    bool disable();

    Fd fd() const { return fd_; }

  protected:
    bool openDevice(const std::string &dev);

    bool setMode(const Mode &mode);
    bool setMode(const std::string &mode_str);  //! 以字串的形式设置，如:"115200 8n1"

  private:
    Fd fd_;
    BufferedFd buff_fd_;
};

}
}

#endif //TBOX_NETWORK_UART_H_20171104
