#pragma once

#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "http_header.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class session;
class socket_stream;
class HttpServletRequest;
class HttpServletResponse;

/**
 * 处理 HTTP 客户端请求的基类，子类需要继承该类
 */
class ACL_CPP_API HttpServlet : public noncopyable {
public:
    /**
     * 构造函数
     * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
     *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
     *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
     *  的关闭情况，这样可以方便与 acl_master 架构结合
     * @param session {session*} 每一个 HttpServlet 对象一个 session 对象
     */
    HttpServlet(socket_stream* stream, session* session);
    HttpServlet(socket_stream* stream);

    /**
     * 构造函数（该函数已经废弃，请用其它构造方法）
     * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
     *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
     *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
     *  的关闭情况，这样可以方便与 acl_master 架构结合
     * @param memcache_addr {const char*}
     */
    //@ACL_DEPRECATED
    HttpServlet(socket_stream* stream, const char* memcache_addr);

    HttpServlet(void);
    virtual ~HttpServlet(void) = 0;

    session& getSession(void) const
    {
        return *session_;
    }

    socket_stream* getStream(void) const
    {
        return stream_;
    }

    /**
     * 设置本地字符集，如果设置了本地字符集，则在接收 HTTP 请求数据时，会
     * 自动将请求的字符集转为本地字符集；该函数必须在 doRun 之前调用才有效
     * @param charset {const char*} 本地字符集，如果该指针为空，
     *  则清除本地字符集
     * @return {HttpServlet&}
     */
    HttpServlet& setLocalCharset(const char* charset);

    /**
     * 设置 HTTP 会话过程 IO 读写超时时间；该函数必须在 doRun 前调用才有效
     * @param rw_timeout {int} 读写超时时间(秒)
     * @return {HttpServlet&}
     */
    HttpServlet& setRwTimeout(int rw_timeout);

    /**
     * 针对 POST 方法，该方法设置是否需要解析 Form 数据体数据，默认为解析，
     * 该函数必须在 doRun 之前调用才有效；当数据体为数据流或 MIME 格式，
     * 即使调用本方法设置了解析数据，也不会对数据体进行解析
     * @param yes {bool} 是否需要解析
     * @return {HttpServlet&}
     */
    HttpServlet& setParseBody(bool yes);

    /**
     * 针对 POST 方法，该方法设置解析数据体的最大长度，如果数据体，该函数
     * 必须在 doRun 之前调用才有效
     * @param length {int} 最大长度限制，如果请求的数据体长度过大，则直接
     *  返回 false，如果该值 <= 0 则内部不限制数据体长度，调用该函数前
     *  内部缺省值为 0
     * @return {HttpServlet&}
     */
    HttpServlet& setParseBodyLimit(int length);
    
    /**
     * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数，
     * @return {bool} 返回处理结果，返回 false 表示处理失败，则应关闭连接，
     *  返回 true 表示处理成功，调用此函数后应该继续通过判断请求/响应对象中
     *  是否需要保持长连接来确实最终是否保持长连接
     */
    bool start(void);

    /**
     * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数，
     * 该函数首先会调用 start 过程，然后根据 start 的返回结果及请求/响应
     * 对象是否要求保持长连接来决定是否需要与客户端保持长连接
     * @return {bool} 返回处理结果，返回 false 表示处理失败或处理成功且不保持
     *  长连接，应关闭连接
     */
    virtual bool doRun(void);

    /**
     * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数
     * @param session {session&} 存储 session 数据的对象
     * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
     *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
     *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
     *  的关闭情况，这样可以方便与 acl_master 架构结合
     * @return {bool} 返回处理结果
     */
    virtual bool doRun(session& session, socket_stream* stream = NULL);

    /**
     * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数，
     * 调用本函数意味着采用 memcached 来存储 session 数据
     * @param memcached_addr {const char*} memcached 服务器地址，格式：IP:PORT
     * @param stream {socket_stream*} 含义同上
     * @return {bool} 返回处理结果
     */
    virtual bool doRun(const char* memcached_addr, socket_stream* stream);

protected:
    /**
     * 当 HTTP 请求为 GET 方式时调用的虚函数
     */
    virtual bool doGet(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 websocket 方式时调用的虚函数
     */
    virtual bool doWebSocket(HttpServletRequest&, HttpServletResponse&);

    /**
     * 旧的处理 websocket 接口，请重载上面的 doWebSocket 方法
     */
    virtual bool doWebsocket(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 POST 方式时调用的虚函数
     */
    virtual bool doPost(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 PUT 方式时调用的虚函数
     */
    virtual bool doPut(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 PATCH 方式时调用的虚函数
     */
    virtual bool doPatch(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 CONNECT 方式时调用的虚函数
     */
    virtual bool doConnect(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 PURGE 方式时调用的虚函数，该方法在清除 SQUID 的缓存
     * 时会用到
     */
    virtual bool doPurge(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 DELETE 方式时调用的虚函数
     */
    virtual bool doDelete(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 HEAD 方式时调用的虚函数
     */
    virtual bool doHead(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 OPTION 方式时调用的虚函数
     */
    virtual bool doOptions(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求为 PROPFIND 方式时调用的虚函数
     */
    virtual bool doPropfind(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求方法未知时调用的虚函数
     * @param method {const char*} 其它未知的请求方法
     */
    virtual bool doOther(HttpServletRequest&, HttpServletResponse&,
        const char* method);

    /**
     * 当 HTTP 请求方法未知时调用的虚函数
     */
    virtual bool doUnknown(HttpServletRequest&, HttpServletResponse&);

    /**
     * 当 HTTP 请求出错时调用的虚函数
     */
    virtual bool doError(HttpServletRequest&, HttpServletResponse&);

protected:
    HttpServletRequest* req_;
    HttpServletResponse* res_;
    bool parse_body_;

private:
    session* session_;
    socket_stream* stream_;
    bool  first_;
    char* local_charset_;
    int   rw_timeout_;
    int   parse_body_limit_;
    bool  try_old_ws_;

    void init();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
