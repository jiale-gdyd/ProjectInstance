#include <tbox/network/stdio_stream.h>

#include <cstdio>

namespace tbox {
namespace network {

StdinStream::StdinStream(event::Loop *wp_loop) :
    buff_fd_(wp_loop)
{
    buff_fd_.initialize(STDIN_FILENO, BufferedFd::kReadOnly);
}

void StdinStream::setReceiveCallback(const ReceiveCallback &cb, size_t threshold)
{
    buff_fd_.setReceiveCallback(cb, threshold);
}

bool StdinStream::enable()
{
    return buff_fd_.enable();
}

bool StdinStream::disable()
{
    return buff_fd_.disable();
}

StdoutStream::StdoutStream(event::Loop *wp_loop) :
    buff_fd_(wp_loop)
{
    buff_fd_.initialize(STDOUT_FILENO, BufferedFd::kWriteOnly);
}

bool StdoutStream::send(const void *data_ptr, size_t data_size)
{
    return buff_fd_.send(data_ptr, data_size);
}

bool StdoutStream::enable()
{
    return buff_fd_.enable();
}

bool StdoutStream::disable()
{
    return buff_fd_.disable();
}

StdioStream::StdioStream(event::Loop *wp_loop) :
    in_buff_fd_(wp_loop),
    out_buff_fd_(wp_loop)
{
    in_buff_fd_.initialize(STDIN_FILENO, BufferedFd::kReadOnly);
    out_buff_fd_.initialize(STDOUT_FILENO, BufferedFd::kWriteOnly);
}

void StdioStream::setReceiveCallback(const ReceiveCallback &cb, size_t threshold)
{
    in_buff_fd_.setReceiveCallback(cb, threshold);
}

bool StdioStream::send(const void *data_ptr, size_t data_size)
{
    return out_buff_fd_.send(data_ptr, data_size);
}

bool StdioStream::enable()
{
    return in_buff_fd_.enable() && out_buff_fd_.enable();
}

bool StdioStream::disable()
{
    return in_buff_fd_.disable() && out_buff_fd_.disable();
}

}
}
