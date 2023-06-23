#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#ifndef ACL_PREPARE_COMPILE
#include "acl/lib_acl_cpp/stream/aio_handle.hpp"
#include "acl/lib_acl_cpp/stream/aio_socket_stream.hpp"
#endif

namespace acl
{

aio_socket_stream::aio_socket_stream(aio_handle* handle,
    ACL_ASTREAM* stream, bool opened /* = false */)
: aio_stream(handle), aio_istream(handle), aio_ostream(handle)
, open_callbacks_(NULL)
{
    acl_assert(handle);
    acl_assert(stream);

    if (opened) {
        status_ |= STATUS_CONN_OPENED;
    }

    stream_ = stream;

    // 调用基类的 enable_error 以向 handle 中增加异步流计数,
    // 同时注册关闭及超时回调过程
    this->enable_error();

    // 只有当流连接成功后才可 hook IO 读写状态
    if (opened) {
        // 注册读回调过程
        this->enable_read();

        // 注册写回调过程
        this->enable_write();
    }
}

aio_socket_stream::aio_socket_stream(aio_handle* handle, ACL_SOCKET fd)
: aio_stream(handle), aio_istream(handle), aio_ostream(handle)
, open_callbacks_(NULL)
{
    acl_assert(handle);

    status_ |= STATUS_CONN_OPENED;

    ACL_VSTREAM* vstream = acl_vstream_fdopen(fd, O_RDWR, 8192, -1,
                    ACL_VSTREAM_TYPE_SOCK);
    stream_ = acl_aio_open(handle->get_handle(), vstream);

    // 调用基类的 enable_error 以向 handle 中增加异步流计数,
    // 同时注册关闭及超时回调过程
    this->enable_error();

    // 只有当流连接成功后才可 hook IO 读状态
    // 注册读回调过程
    this->enable_read();

    // 注册写回调过程
    this->enable_write();
}

aio_socket_stream::~aio_socket_stream(void)
{
    if (open_callbacks_) {
        std::list<AIO_OPEN_CALLBACK*>::iterator
            it = open_callbacks_->begin();
        for (; it != open_callbacks_->end(); ++it) {
            acl_myfree(*it);
        }
        delete open_callbacks_;
    }
}

void aio_socket_stream::destroy(void)
{
    delete this;
}

void aio_socket_stream::add_open_callback(aio_open_callback* callback)
{
    if (open_callbacks_ == NULL) {
        open_callbacks_ = NEW std::list<AIO_OPEN_CALLBACK*>;
    }

    // 先查询该回调对象已经存在
    std::list<AIO_OPEN_CALLBACK*>::iterator it = open_callbacks_->begin();
    for (; it != open_callbacks_->end(); ++it) {
        if ((*it)->callback == callback) {
            if ((*it)->enable == false) {
                (*it)->enable = true;
            }
            return;
        }
    }

    // 找一个空位
    it = open_callbacks_->begin();
    for (; it != open_callbacks_->end(); ++it) {
        if ((*it)->callback == NULL) {
            (*it)->enable = true;
            (*it)->callback = callback;
            return;
        }
    }

    // 分配一个新的位置
    AIO_OPEN_CALLBACK* ac = (AIO_OPEN_CALLBACK*)
        acl_mycalloc(1, sizeof(AIO_OPEN_CALLBACK));
    ac->enable   = true;
    ac->callback = callback;

    // 添加进回调对象队列中
    open_callbacks_->push_back(ac);
}

int aio_socket_stream::del_open_callback(aio_open_callback* callback /* = NULL */)
{
    if (open_callbacks_ == NULL) {
        return 0;
    }

    std::list<AIO_OPEN_CALLBACK*>::iterator it = open_callbacks_->begin();
    int   n = 0;

    if (callback == NULL) {
        for (; it != open_callbacks_->end(); ++it) {
            if ((*it)->callback == NULL) {
                continue;
            }
            (*it)->enable = false;
            (*it)->callback = NULL;
            n++;
        }
    } else {
        for (; it != open_callbacks_->end(); ++it) {
            if ((*it)->callback != callback) {
                continue;
            }
            (*it)->enable = false;
            (*it)->callback = NULL;
            n++;
            break;
        }
    }

    return n;
}

int aio_socket_stream::disable_open_callback(aio_open_callback* callback /* = NULL */)
{
    if (open_callbacks_ == NULL) {
        return 0;
    }

    std::list<AIO_OPEN_CALLBACK*>::iterator it = open_callbacks_->begin();
    int   n = 0;

    if (callback == NULL) {
        for (; it != open_callbacks_->end(); ++it) {
            if ((*it)->callback == NULL || !(*it)->enable) {
                continue;
            }
            (*it)->enable = false;
            n++;
        }
    } else {
        for (; it != open_callbacks_->end(); ++it) {
            if ((*it)->callback != callback || !(*it)->enable) {
                continue;
            }
            (*it)->enable = false;
            n++;
            break;
        }
    }

    return n;
}

int aio_socket_stream::enable_open_callback(aio_open_callback* callback /* = NULL */)
{
    if (open_callbacks_ == NULL) {
        return 0;
    }

    std::list<AIO_OPEN_CALLBACK*>::iterator it = open_callbacks_->begin();
    int   n = 0;

    if (callback == NULL) {
        for (; it != open_callbacks_->end(); ++it) {
            if (!(*it)->enable && (*it)->callback != NULL) {
                (*it)->enable = true;
                n++;
            }
        }
    } else {
        for (; it != open_callbacks_->end(); ++it) {
            if (!(*it)->enable && (*it)->callback == callback) {
                (*it)->enable = true;
                n++;
            }
        }
    }

    return n;
}

aio_socket_stream* aio_socket_stream::open(aio_handle* handle,
    const char* addr, int timeout)
{
    acl_assert(handle);

    ACL_ASTREAM* astream =
        acl_aio_connect(handle->get_handle(), addr, timeout);
    if (astream == NULL) {
        return NULL;
    }

    aio_socket_stream* stream = NEW aio_socket_stream(handle, astream, false);

    // 调用基类的 enable_error 以向 handle 中增加异步流计数,
    // 同时注册关闭及超时回调过程
    stream->enable_error();
    // 注册连接成功的回调过程
    stream->enable_open();

    return stream;
}

aio_socket_stream* aio_socket_stream::bind(aio_handle* handle, const char * addr)
{
    acl_assert(handle);
    unsigned flag = ACL_INET_FLAG_NBLOCK | ACL_INET_FLAG_REUSEPORT;

    ACL_VSTREAM* vstream = acl_vstream_bind(addr, 0, flag);
    if (vstream == NULL) {
        logger_error("bind %s error %s", addr, last_serror());
        return NULL;
    }

    ACL_AIO* aio = handle->get_handle();
    ACL_ASTREAM* astream = acl_aio_open(aio, vstream);
    aio_socket_stream* stream = NEW aio_socket_stream(handle, astream, true);
    return stream;
}

bool aio_socket_stream::is_opened(void) const
{
    return (status_ & STATUS_CONN_OPENED) ? true : false;
}

void aio_socket_stream::enable_open(void)
{
    acl_assert(stream_);

    if ((status_ & STATUS_HOOKED_OPEN)) {
        return;
    }
    status_ |= STATUS_HOOKED_OPEN;

    acl_aio_ctl(stream_,
        ACL_AIO_CTL_CONNECT_HOOK_ADD, open_callback, this,
        ACL_AIO_CTL_END);
}

int aio_socket_stream::open_callback(ACL_ASTREAM* stream acl_unused, void* ctx)
{
    aio_socket_stream* ss = (aio_socket_stream*) ctx;

    // 设置状态，表明已经连接成功
    ss->status_ |= STATUS_CONN_OPENED;

    // 注册读回调过程
    ss->enable_read();

    // 注册写回调过程
    ss->enable_write();

    if (ss->open_callbacks_ == NULL) {
        return 0;
    }

    // 遍历所有的打开回调对象，并调用之
    std::list<AIO_OPEN_CALLBACK*>::iterator it = ss->open_callbacks_->begin();
    for (; it != ss->open_callbacks_->end(); ++it) {
        if (!(*it)->enable || (*it)->callback == NULL) {
            continue;
        }

        if ((*it)->callback->open_callback() == false) {
            return -1;
        }
    }
    return 0;
}

}  // namespace acl
