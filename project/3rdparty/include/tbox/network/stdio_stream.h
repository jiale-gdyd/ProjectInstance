#ifndef TBOX_NETWORK_STDIO_STREAM_H
#define TBOX_NETWORK_STDIO_STREAM_H

#include "../base/defines.h"
#include "../event/loop.h"

#include "byte_stream.h"
#include "buffered_fd.h"

namespace tbox {
namespace network {

class StdinStream : public ByteStream {
  public:
    explicit StdinStream(event::Loop *wp_loop);

    NONCOPYABLE(StdinStream);
    IMMOVABLE(StdinStream);

  public:
    virtual void setReceiveCallback(const ReceiveCallback &cb, size_t threshold) override;
    virtual bool send(const void *data_ptr, size_t data_size) override { return false; (void)data_ptr; (void)data_size; }
    virtual void bind(ByteStream *receiver) override { buff_fd_.bind(receiver); }
    virtual void unbind() override { buff_fd_.unbind(); }

    bool enable();
    bool disable();

  private:
    BufferedFd buff_fd_;
};

class StdoutStream : public ByteStream {
  public:
    explicit StdoutStream(event::Loop *wp_loop);

    NONCOPYABLE(StdoutStream);
    IMMOVABLE(StdoutStream);

  public:
    virtual void setReceiveCallback(const ReceiveCallback &cb, size_t threshold) override { (void)cb; (void)threshold; }
    virtual bool send(const void *data_ptr, size_t data_size) override;
    virtual void bind(ByteStream *receiver) override { (void)receiver; }
    virtual void unbind() override { }

    bool enable();
    bool disable();

  private:
    BufferedFd buff_fd_;
};

class StdioStream : public ByteStream {
  public:
    explicit StdioStream(event::Loop *wp_loop);

    NONCOPYABLE(StdioStream);
    IMMOVABLE(StdioStream);

  public:
    virtual void setReceiveCallback(const ReceiveCallback &cb, size_t threshold) override;
    virtual bool send(const void *data_ptr, size_t data_size) override;
    virtual void bind(ByteStream *receiver) override { in_buff_fd_.bind(receiver); }
    virtual void unbind() override { in_buff_fd_.unbind(); }

    bool enable();
    bool disable();

  private:
    BufferedFd in_buff_fd_;
    BufferedFd out_buff_fd_;
};

}
}

#endif //TBOX_NETWORK_STDIO_STREAM_H_20171103
