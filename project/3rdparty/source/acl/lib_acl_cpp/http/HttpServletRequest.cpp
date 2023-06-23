#include "acl/lib_acl_cpp/acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl/lib_acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl/lib_acl_cpp/stdlib/snprintf.hpp"
#include "acl/lib_acl_cpp/stdlib/log.hpp"
#include "acl/lib_acl_cpp/stdlib/string.hpp"
#include "acl/lib_acl_cpp/stream/istream.hpp"
#include "acl/lib_acl_cpp/session/session.hpp"
#include "acl/lib_acl_cpp/stream/socket_stream.hpp"
#include "acl/lib_acl_cpp/stdlib/charset_conv.hpp"
#include "acl/lib_acl_cpp/stdlib/xml.hpp"
#include "acl/lib_acl_cpp/stdlib/xml1.hpp"
#include "acl/lib_acl_cpp/stdlib/json.hpp"
#include "acl/lib_acl_cpp/http/http_header.hpp"
#include "acl/lib_acl_cpp/http/HttpCookie.hpp"
#include "acl/lib_acl_cpp/http/http_client.hpp"
#include "acl/lib_acl_cpp/http/http_mime.hpp"
#include "acl/lib_acl_cpp/http/HttpSession.hpp"
#include "acl/lib_acl_cpp/http/HttpServletResponse.hpp"
#include "acl/lib_acl_cpp/http/HttpServletRequest.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

#define SKIP_SPACE(x) { while (*x == ' ' || *x == '\t') x++; }
#define ADDR_LEN	64

namespace acl {

#define COPY(x, y) ACL_SAFE_STRNCPY((x), (y), sizeof((x)))

HttpServletRequest::HttpServletRequest(HttpServletResponse& res,
    session* sess, socket_stream& stream,
    const char* charset /* = NULL */, int body_limit /* = 102400 */)
: req_error_(HTTP_REQ_OK)
, res_(res)
, sess_(sess)
, http_session_(NULL)
, stream_(stream)
, body_limit_(body_limit)
, body_parsed_(false)
, cookies_inited_(false)
, client_(NULL)
, method_(HTTP_METHOD_UNKNOWN)
, localAddr_(NULL)
, remoteAddr_(NULL)
, request_type_(HTTP_REQUEST_NORMAL)
, parse_body_(true)
, mime_(NULL)
, body_(NULL)
, json_(NULL)
, xml_(NULL)
, readHeaderCalled_(false)
{
    dbuf_internal_ = NEW dbuf_guard(1, 100);
    dbuf_ = dbuf_internal_;

    cookie_name_ = dbuf_->dbuf_strdup("ACL_SESSION_ID");
    ACL_VSTREAM* in = stream.get_vstream();
    if (in == ACL_VSTREAM_IN) {
        cgi_mode_ = true;
    } else {
        cgi_mode_ = false;
    }
    if (charset && *charset) {
        localCharset_ = dbuf_->dbuf_strdup(charset);
    } else {
        localCharset_ = NULL;
    }
    rw_timeout_ = 60;
}

HttpServletRequest::~HttpServletRequest(void)
{
    if (client_)
        client_->~http_client();
    delete dbuf_internal_;
    delete body_; // 该对象不是在 dbuf 上分配的，所以需要单独释放
}

http_method_t HttpServletRequest::getMethod(string* method_s /* = NULL */) const
{
    // HttpServlet 类对象的 doRun 运行时 readHeader 必须先被调用，
    // 而 HttpSevlet 类在初始化请求时肯定会调用 getMethod 方法，
    // 所以在此函数中触发 readHeader 方法比较好，这样一方面可以
    // 将 readHeader 方法隐藏起来，免得用户误调用；另一方面，又
    // 能保证 readHeader 肯定会被调用；同时又不必把 HttpServlet
    // 类声明本类的友元类

    if (!readHeaderCalled_) {
        const_cast<HttpServletRequest*>(this)->readHeader(method_s);
    } else if (method_s) {
        methodString(method_, *method_s);
    }
    return method_;
}

void HttpServletRequest::setParseBody(bool yes)
{
    parse_body_ = yes;
}

void HttpServletRequest::add_cookie(char* data)
{
    SKIP_SPACE(data);
    if (*data == 0 || *data == '=') {
        return;
    }
    char* ptr = strchr(data, '=');
    if (ptr == NULL) {
        return;
    }
    *ptr++ = 0;
    SKIP_SPACE(ptr)
    if (*ptr == 0) {
        return;
    }
    char* end = ptr + strlen(ptr) - 1;
    while (end > ptr && (*end == ' ' || *end == '\t')) {
        *end-- = 0;
    }
    setCookie(data, ptr);
}

void HttpServletRequest::setCookie(const char* name, const char* value)
{
    if (name == NULL || *name == 0 || value == NULL) {
        return;
    }
    HttpCookie* cookie = dbuf_->create<HttpCookie, const char*,
        const char*, dbuf_guard*> (name, value, dbuf_);
    cookies_.push_back(cookie);
}

const std::vector<HttpCookie*>& HttpServletRequest::getCookies(void) const
{
    if (cookies_inited_) {
        return cookies_;
    }

    // 设置标记表明已经分析过cookie了，以便于下次重复调用时节省时间
    const_cast<HttpServletRequest*>(this)->cookies_inited_ = true;

    if (cgi_mode_) {
        const char* ptr = acl_getenv("HTTP_COOKIE");
        if (ptr == NULL || *ptr == 0) {
            return cookies_;
        }
        ACL_ARGV* argv = acl_argv_split(ptr, ";");
        ACL_ITER iter;
        acl_foreach(iter, argv) {
            const_cast<HttpServletRequest*>
                (this)->add_cookie((char*) iter.data);
        }
        acl_argv_free(argv);
        return cookies_;
    }

    if (client_ == NULL) {
        return cookies_;
    }
    const HTTP_HDR_REQ* req = client_->get_request_head(NULL);
    if (req == NULL) {
        return cookies_;
    }
    if (req->cookies_table == NULL) {
        return cookies_;
    }

    ACL_HTABLE_ITER iter;

    // 遍历 HTTP  请求头中的 cookie 项
    acl_htable_foreach(iter, req->cookies_table) {
        const char* name  = acl_htable_iter_key(iter);
        const char* value = (char*) acl_htable_iter_value(iter);
        if (name == NULL || *name == 0
            || value == NULL || *value == 0) {

            continue;
        }
        // 创建 cookie 对象并将之加入数组中
        HttpCookie* cookie = dbuf_->create<HttpCookie, const char*,
            const char*, dbuf_guard*>(name, value, dbuf_);
        const_cast<HttpServletRequest*>
            (this)->cookies_.push_back(cookie);
    }

    return cookies_;
}

const char* HttpServletRequest::getCookieValue(const char* name) const
{
    (void) getCookies();

    std::vector<HttpCookie*>::const_iterator cit = cookies_.begin();
    for (; cit != cookies_.end(); ++cit) {
        const char* ptr = (*cit)->getName();
        if (ptr && strcmp(name, ptr) == 0) {
            return (*cit)->getValue();
        }
    }
    return NULL;
}

const char* HttpServletRequest::getHeader(const char* name) const
{
    if (cgi_mode_) {
        return acl_getenv(name);
    }

    if (client_ == NULL) {
        return NULL;
    }
    return client_->header_value(name);
}

const char* HttpServletRequest::getQueryString(void) const
{
    if (cgi_mode_) {
        return acl_getenv("QUERY_STRING");
    }
    if (client_ == NULL) {
        return "";
    }
    const char* ptr = client_->request_params();
    return ptr ? ptr : "";
}

const char* HttpServletRequest::getPathInfo(void) const
{
    if (cgi_mode_) {
        const char* ptr = acl_getenv("SCRIPT_NAME");
        if (ptr != NULL) {
            return ptr;
        }
        ptr = acl_getenv("PATH_INFO");
        return ptr ? ptr : "";
    }
    if (client_ == NULL) {
        return "";
    }
    const char* ptr = client_->request_path();
    return ptr ? ptr : "";
}

const char* HttpServletRequest::getRequestUri(void) const
{
    if (cgi_mode_) {
        return acl_getenv("REQUEST_URI");
    }
    if (client_ == NULL) {
        return "";
    } else {
        const char* ptr = client_->request_url();
        return ptr ? ptr : "";
    }
}

HttpSession& HttpServletRequest::getSession(bool create /* = true */,
    const char* sid_in /* = NULL */)
{
    if (http_session_ != NULL) {
        return *http_session_;
    }

    if (sess_ == NULL) {
        sess_ = dbuf_->create<memcache_session>("127.0.0.1|11211");
    }

    http_session_ = dbuf_->create<HttpSession, session&>(*sess_);
    const char* sid;

    if ((sid = getCookieValue(cookie_name_)) != NULL) {
        sess_->set_sid(sid);
    } else if (create) {
        // 获得唯一 ID 标识符
        sid = sess_->get_sid();
        // 生成 cookie 对象，并分别向请求对象和响应对象添加 cookie
        HttpCookie* cookie = dbuf_->create<HttpCookie, const char*,
            const char*, dbuf_guard*>(cookie_name_, sid, dbuf_);
        res_.addCookie(cookie);
        setCookie(cookie_name_, sid);
    } else if (sid_in != NULL && *sid_in != 0) {
        sess_->set_sid(sid_in);
        // 生成 cookie 对象，并分别向请求对象和响应对象添加 cookie
        HttpCookie* cookie = dbuf_->create<HttpCookie, const char*,
            const char*, dbuf_guard*>(cookie_name_, sid_in, dbuf_);
        res_.addCookie(cookie);
        setCookie(cookie_name_, sid_in);
    }

    return *http_session_;
}

acl_int64 HttpServletRequest::getContentLength(void) const
{
    if (cgi_mode_) {
        const char* ptr = acl_getenv("CONTENT_LENGTH");
        if (ptr == NULL) {
            return -1;
        }
        return acl_atoui64(ptr);
    }
    if (client_ == NULL) {
        return -1;
    }
    return client_->body_length();
}

bool HttpServletRequest::getRange(http_off_t& range_from, http_off_t& range_to)
{
    if (cgi_mode_) {
        logger_error("cant' support CGI mode");
        return false;
    }
    if (client_ == NULL) {
        logger_error("client_ null");
        return false;
    }
    return client_->request_range(range_from, range_to);
}

const char* HttpServletRequest::getContentType(
    bool part /* = true */, http_ctype* ctype /* = NULL */) const
{
    if (ctype != NULL) {
        *ctype = content_type_;
    }
    if (part) {
        return content_type_.get_ctype();
    }
    if (cgi_mode_) {
        return acl_getenv("CONTENT_TYPE");
    }
    if (client_ == NULL) {
        logger_error("client_ null");
        return "";
    }
    return client_->header_value("Content-Type");
}

const char* HttpServletRequest::getCharacterEncoding(void) const
{
    return content_type_.get_charset();
}

const char* HttpServletRequest::getLocalCharset(void) const
{
    return localCharset_ ? localCharset_ : NULL;
}

const char* HttpServletRequest::getLocalAddr(void) const
{
    if (cgi_mode_) {
        return NULL;
    }

    if (client_ == NULL) {
        return NULL;
    }
    const char* ptr = client_->get_stream().get_local();
    if (*ptr == 0) {
        return NULL;
    }


    if (localAddr_ == NULL) {
        const_cast<HttpServletRequest*>(this)->localAddr_ = (char*)
            dbuf_->dbuf_alloc(ADDR_LEN);
    }
    safe_snprintf(const_cast<HttpServletRequest*>(this)->localAddr_,
        ADDR_LEN, "%s", ptr);
    char* p = (char*) strchr(localAddr_, ':');
    if (p) {
        *p = 0;
    }
    return localAddr_;
}

unsigned short HttpServletRequest::getLocalPort(void) const
{
    if (cgi_mode_) {
        return 0;
    }

    if (client_ == NULL) {
        return 0;
    }

    const char* ptr = client_->get_stream().get_local(true);
    if (*ptr == 0) {
        return 0;
    }

    char* p = (char*) strchr(ptr, ':');
    if (p == NULL || *(++p) == 0) {
        return 0;
    }

    return atoi(p);
}

const char* HttpServletRequest::getRemoteAddr(void) const
{
    if (cgi_mode_) {
        const char* ptr = acl_getenv("REMOTE_ADDR");
        if (ptr && *ptr) {
            return ptr;
        }
        logger_warn("no REMOTE_ADDR from acl_getenv");
        return NULL;
    }
    if (client_ == NULL) {
        return NULL;
    }
    const char* ptr = client_->get_stream().get_peer();
    if (*ptr == 0) {
        logger_warn("get_peer return empty string");
        return NULL;
    }

    if (remoteAddr_ == NULL) {
        const_cast<HttpServletRequest*>(this)->remoteAddr_ = (char*)
            dbuf_->dbuf_alloc(ADDR_LEN);
    }

    safe_snprintf(const_cast<HttpServletRequest*>(this)->remoteAddr_,
        ADDR_LEN, "%s", ptr);
    char* p = (char*) strchr(remoteAddr_, ':');
    if (p) {
        *p = 0;
    }
    return remoteAddr_;
}

unsigned short HttpServletRequest::getRemotePort(void) const
{
    if (cgi_mode_) {
        const char* ptr = acl_getenv("REMOTE_PORT");
        if (ptr && *ptr) {
            return atoi(ptr);
        }
        logger_warn("no REMOTE_PORT from acl_getenv");
        return 0;
    }
    if (client_ == NULL) {
        return 0;
    }
    const char* ptr = client_->get_stream().get_peer(true);
    if (*ptr == 0) {
        logger_warn("get_peer return empty string");
        return 0;
    }
    char* port = (char*) strchr(ptr, ':');
    if (port == NULL || *(++port) == 0) {
        logger_warn("no port in addr: %s", ptr);
        return 0;
    }

    return atoi(port);
}

const char* HttpServletRequest::getParameter(const char* name,
    bool case_sensitive /* = false */) const
{
    std::vector<HTTP_PARAM*>::const_iterator cit = params_.begin();
    if (case_sensitive) {
        for (; cit != params_.end(); ++cit) {
            if (strcmp((*cit)->name, name) == 0) {
                return (*cit)->value;
            }
        }
    } else {
        for (; cit != params_.end(); ++cit) {
            if (strcasecmp((*cit)->name, name) == 0) {
                return (*cit)->value;
            }
        }
    }

    // 如果是 MIME 格式，则尝试从 mime_ 对象中查询参数
    if (mime_ == NULL) {
        return NULL;
    }
    const http_mime_node* node = mime_->get_node(name);
    if (node == NULL) {
        return NULL;
    }
    return node->get_value();
}

http_mime* HttpServletRequest::getHttpMime(void)
{
    return mime_;
}

bool HttpServletRequest::getJson(json& out, size_t body_limit /* 1024000 */)
{
    if (request_type_ != HTTP_REQUEST_TEXT_JSON) {
        return false;
    }

    acl_int64 dlen = (acl_int64) getContentLength();
    if (dlen <= 0 || dlen > (acl_int64) body_limit) {
        return false;
    }

    body_parsed_ = true;
    istream& in  = getInputStream();

    ssize_t n;
    char buf[8192];

    while (dlen > 0) {
        n = (ssize_t) sizeof(buf) - 1 > (ssize_t) dlen ?
            (ssize_t) dlen : (ssize_t) sizeof(buf) - 1;
        n = in.read(buf, (size_t) n);
        if (n == -1) {
            return false;
        }

        dlen  -= n;
        buf[n] = 0;
        out.update(buf);
    }

    return true;
}

json* HttpServletRequest::getJson(size_t body_limit /* 1024000 */)
{
    if (json_ || body_parsed_) {
        return json_;
    } else if (request_type_ != HTTP_REQUEST_TEXT_JSON) {
        return NULL;
    }

    acl_int64 dlen = getContentLength();
    if (dlen <= 0 || dlen > (acl_int64) body_limit) {
        return NULL;
    }

    json_ = dbuf_->create<json>();
    if (getJson(*json_)) {
        return json_;
    }  else {
        json_ = NULL;
        return NULL;
    }
}

bool HttpServletRequest::getXml(xml& out, size_t body_limit /* 1024000 */)
{
    if (request_type_ != HTTP_REQUEST_TEXT_XML) {
        return false;
    }

    acl_int64 dlen = (acl_int64) getContentLength();
    if (dlen <= 0 || dlen > (acl_int64) body_limit) {
        return false;
    }

    body_parsed_ = true;
    istream& in  = getInputStream();

    ssize_t n;
    char buf[8192];

    while (dlen > 0) {
        n = (ssize_t) sizeof(buf) - 1 > (ssize_t) dlen ?
            (ssize_t) dlen : (ssize_t) sizeof(buf) - 1;
        n = in.read(buf, (size_t) n);
        if (n == -1) {
            return false;
        }

        dlen  -= n;
        buf[n] = 0;
        out.update(buf);
    }

    return true;
}

xml* HttpServletRequest::getXml(size_t body_limit /* 1024000 */)
{
    if (xml_ && body_parsed_) {
        return xml_;
    } else if (request_type_ != HTTP_REQUEST_TEXT_XML) {
        return NULL;
    }

    acl_int64 dlen = (acl_int64) getContentLength();
    if (dlen <= 0 || dlen > (acl_int64) body_limit) {
        return NULL;
    }

    xml_ = dbuf_->create<xml1>();
    if (getXml(*xml_)) {
        return xml_;
    } else {
        xml_ = NULL;
        return NULL;
    }
}

bool HttpServletRequest::getBody(string& out, size_t body_limit /* 1024000 */)
{
    acl_int64 dlen = (acl_int64) getContentLength();
    if (dlen <= 0 || dlen > (acl_int64) body_limit) {
        return false;
    }

    out.space((size_t) dlen + 1);
    body_parsed_ = true;
    istream& in  = getInputStream();

    ssize_t n;
    char buf[8192];

    while (dlen > 0) {
        n = (ssize_t) sizeof(buf) - 1 > (ssize_t) dlen ?
            (ssize_t) dlen : (ssize_t) sizeof(buf) - 1;
        n = in.read(buf, (size_t) n);
        if (n == -1) {
            return false;
        }

        dlen  -= n;
        buf[n] = 0;
        out.append(buf, (size_t) n);
    }

    return true;
}

string* HttpServletRequest::getBody(size_t body_limit /* 1024000*/)
{
    if (body_ && body_parsed_) {
        return body_;
    }

    acl_int64 dlen = (acl_int64) getContentLength();
    if (dlen <= 0 || dlen > (acl_int64) body_limit) {
        return NULL;
    }

    body_ = NEW string((size_t) dlen + 1);
    if (getBody(*body_)) {
        return body_;
    } else {
        delete body_;
        body_ = NULL;
        return NULL;
    }
}

http_request_t HttpServletRequest::getRequestType(void) const
{
    return request_type_;
}

istream& HttpServletRequest::getInputStream(void) const
{
    return stream_;
}

socket_stream& HttpServletRequest::getSocketStream(void) const
{
    return stream_;
}

void HttpServletRequest::parseParameters(const char* str)
{
    const char* requestCharset = getCharacterEncoding();
    charset_conv conv;
    string buf;
    ACL_ARGV* tokens = acl_argv_split(str, "&");
    ACL_ITER iter;
    acl_foreach(iter, tokens) {
        char* name = (char*) iter.data;
        char* value = strchr(name, '=');
        if (value == NULL || *(value + 1) == 0) {
            continue;
        }
        *value++ = 0;

        name = acl_url_decode(name, NULL);
        value = acl_url_decode(value, NULL);

        HTTP_PARAM* param = (HTTP_PARAM*)
            dbuf_->dbuf_calloc(sizeof(HTTP_PARAM));

        if (localCharset_ && requestCharset
            && strcasecmp(requestCharset, localCharset_)) {

            buf.clear();
            if (conv.convert(requestCharset, localCharset_,
                name, strlen(name), &buf)) {

                param->name = dbuf_->dbuf_strdup(buf.c_str());
            } else {
                param->name = dbuf_->dbuf_strdup(name);
            }

            buf.clear();
            if (conv.convert(requestCharset, localCharset_,
                value, strlen(value), &buf)) {

                param->value =  dbuf_->dbuf_strdup(buf.c_str());
            } else {
                param->value = dbuf_->dbuf_strdup(value);
            }
        } else {
            param->name = dbuf_->dbuf_strdup(name);
            param->value = dbuf_->dbuf_strdup(value);
        }

        acl_myfree(name);
        acl_myfree(value);

        params_.push_back(param);
    }

    acl_argv_free(tokens);
}

void HttpServletRequest::methodString(http_method_t type, string &buf)
{
    switch (type) {
    case HTTP_METHOD_GET:
        buf = "GET";
        break;
    case HTTP_METHOD_POST:
        buf = "POST";
        break;
    case HTTP_METHOD_PUT:
        buf = "PUT";
        break;
    case HTTP_METHOD_CONNECT:
        buf = "CONNECT";
        break;
    case HTTP_METHOD_PURGE:
        buf = "PURGE";
        break;
    case HTTP_METHOD_DELETE:
        buf = "DELETE";
        break;
    case HTTP_METHOD_HEAD:
        buf = "HEAD";
        break;
    case HTTP_METHOD_OPTION:
        buf = "OPTION";
        break;
    case HTTP_METHOD_PROPFIND:
        buf = "PROPFIND";
        break;
    case HTTP_METHOD_PATCH:
        buf = "PATCH";
        break;
    default:
        buf = "OTHER";
        break;
    }
}

// Content-Type: application/x-www-form-urlencoded; charset=utf-8
// Content-Type: multipart/form-data; boundary=---------------------------41184676334
// Content-Type: application/octet-stream

bool HttpServletRequest::readHeader(string* method_s)
{
    acl_assert(readHeaderCalled_ == false);
    readHeaderCalled_ = true;

    const char* method;

    if (cgi_mode_) {
        const char* ptr = acl_getenv("CONTENT_TYPE");
        if (ptr && *ptr) {
            content_type_.parse(ptr);
        }

        // 必须是后获得 method，因为 acl_getenv 内部的内存区用的
        // 是线程局部变量，该内存区在同一线程中会被重复使用，这样
        // 后获得 method 可以保证 method 可以由下面的过程确定具体
        // 的请求方法
        method = acl_getenv("REQUEST_METHOD");
    } else {
        client_ = new (dbuf_->dbuf_alloc(sizeof(http_client)))
            http_client(&stream_, false, true);
        if (!client_->read_head()) {
            req_error_ = HTTP_REQ_ERR_IO;
            return false;
        }

        method = client_->request_method();
        const char* ptr = client_->header_value("Content-Type");
        if (ptr && *ptr) {
            content_type_.parse(ptr);
        }
    }

    if (method == NULL || *method == 0) {
        req_error_ = HTTP_REQ_ERR_METHOD;
        logger_error("method null");
        return false;
    }

    // 缓存字符串类型的请求方法
    method_s->copy(method);

    if (strcasecmp(method, "GET") == 0) {
        method_ = HTTP_METHOD_GET;
    } else if (strcasecmp(method, "POST") == 0) {
        method_ = HTTP_METHOD_POST;
    } else if (strcasecmp(method, "PUT") == 0) {
        method_ = HTTP_METHOD_PUT;
    } else if (strcasecmp(method, "CONNECT") == 0) {
        method_ = HTTP_METHOD_CONNECT;
    } else if (strcasecmp(method, "PURGE") == 0) {
        method_ = HTTP_METHOD_PURGE;
    } else if (strcasecmp(method, "DELETE") == 0) {
        method_ = HTTP_METHOD_DELETE;
    } else if (strcasecmp(method, "HEAD") == 0) {
        method_ = HTTP_METHOD_HEAD;
    }else if (strcasecmp(method, "OPTIONS") == 0) {
        method_ = HTTP_METHOD_OPTION;
    } else if (strcasecmp(method, "PROPFIND") == 0) {
        method_ = HTTP_METHOD_PROPFIND;
    } else if (strcasecmp(method, "PATCH") == 0) {
        method_ = HTTP_METHOD_PATCH;
    } else {
        method_ = HTTP_METHOD_OTHER;
    }

    const char* ptr = getQueryString();
    if (ptr && *ptr) {
        parseParameters(ptr);
    }

    if (method_ != HTTP_METHOD_POST) {
        request_type_ = HTTP_REQUEST_NORMAL;
        return true;
    }

    acl_int64 len = getContentLength();
    if (len <= 0) {
        request_type_ = HTTP_REQUEST_NORMAL;
        return true;
    }

    const char* ctype = getContentType();
    const char* stype = content_type_.get_stype();

    // 数据体为文件上传的 MIME 类型
    if (ctype == NULL || stype == NULL) {
        request_type_ = HTTP_REQUEST_OTHER;
        return true;
    }

#define EQ	!strcasecmp

    // 当数据体为 HTTP MIME 上传数据时
    if (EQ(ctype, "multipart") && EQ(stype, "form-data")) {
        const char* bound = content_type_.get_bound();
        if (bound == NULL) {
            request_type_ = HTTP_REQUEST_NORMAL;
        } else {
            request_type_ = HTTP_REQUEST_MULTIPART_FORM;
            mime_ = dbuf_->create<http_mime, const char*,
                    const char*>(bound, localCharset_);
        }

        return true;
    }

    // 数据体为数据流类型
    if (EQ(ctype, "application") && EQ(stype, "octet-stream")) {
        request_type_ = HTTP_REQUEST_OCTET_STREAM;
        return true;
    }

    // 如果需要分析数据体的参数时的数据体长度过大，则直接返回错误
    if (body_limit_ > 0 && len >= body_limit_) {
        logger_error("request body too large, len=%lld, limit=%d",
            len, body_limit_);
        return false;
    }

    // 当数据体为 form 格式时：
    if (EQ(ctype, "application") && EQ(stype, "x-www-form-urlencoded")) {
        request_type_ = HTTP_REQUEST_NORMAL;
        if (!parse_body_) {
            return true;
        }

        char* query = (char*) dbuf_->dbuf_alloc((size_t) len + 1);
        int ret = getInputStream().read(query, (size_t) len);
        if (ret > 0) {
            query[ret] = 0;
            parseParameters(query);
        }

        body_parsed_ = true;
        return ret == -1 ? false : true;
    }

    // 当数据类型为 application/json 或 text/json 格式时：
    if (EQ(stype, "json") && (EQ(ctype, "application") || EQ(ctype, "text"))) {
        request_type_ = HTTP_REQUEST_TEXT_JSON;
        return true;
    }

    // 当数据类型为 application/xml 或 text/xml 格式时：
    if (EQ(stype, "xml") && (EQ(ctype, "application") || EQ(ctype, "text"))) {
        request_type_ = HTTP_REQUEST_TEXT_XML;
        return true;
    }

    request_type_ = HTTP_REQUEST_OTHER;
    return true;
}

const char* HttpServletRequest::getRequestReferer(void) const
{
    if (cgi_mode_) {
        return acl_getenv("HTTP_REFERER");
    }
    if (client_ == NULL) {
        return NULL;
    }
    return client_->header_value("Referer");
}

const http_ctype& HttpServletRequest::getHttpCtype(void) const
{
    return content_type_;
}

const char* HttpServletRequest::getRemoteHost(void) const
{
    if (cgi_mode_) {
        return acl_getenv("HTTP_HOST");
    }
    if (client_ == NULL) {
        return NULL;
    }
    return client_->request_host();
}

const char* HttpServletRequest::getUserAgent(void) const
{
    if (cgi_mode_) {
        return acl_getenv("HTTP_USER_AGENT");
    }
    if (client_ == NULL) {
        return NULL;
    }
    return client_->header_value("User-Agent");
}

bool HttpServletRequest::isKeepAlive(void) const
{
    if (cgi_mode_) {
        const char* ptr = acl_getenv("HTTP_CONNECTION");
        if (ptr == NULL || strcasecmp(ptr, "keep-alive") != 0) {
            return false;
        } else {
            return true;
        }
    }
    if (client_ == NULL) {
        return false;
    }
    return client_->keep_alive();
}

int HttpServletRequest::getKeepAlive(void) const
{
    if (cgi_mode_) {
        return -1;
    }

    if (client_ == NULL) {
        return -1;
    }
    const char* ptr = client_->header_value("Keep-Alive");
    if (ptr == NULL || *ptr == 0) {
        return -1;
    }
    return atoi(ptr);
}

void HttpServletRequest::getAcceptEncoding(std::vector<string>& out) const
{
    out.clear();
    const char* ptr;

    if (cgi_mode_) {
        ptr = acl_getenv("HTTP_ACCEPT_ENCODING");
    } else if (client_) {
        ptr = client_->header_value("Accept-Encoding");
    } else {
        return;
    }

    if (ptr == NULL || *ptr == 0) {
        return;
    }

    ACL_ARGV* tokens = acl_argv_split(ptr, ",; \t");
    ACL_ITER iter;
    acl_foreach(iter, tokens) {
        const char* token = (const char*) iter.data;
        out.push_back(token);
    }
    acl_argv_free(tokens);
}

void HttpServletRequest::setRwTimeout(int rw_timeout)
{
    rw_timeout_ = rw_timeout;
}

http_request_error_t HttpServletRequest::getLastError(void) const
{
    return req_error_;
}

http_client* HttpServletRequest::getClient(void) const
{
    if (client_ == NULL) {
        logger_error("client_ NULL in CGI mode");
    }
    return client_;
}

bool HttpServletRequest::getVersion(unsigned& major, unsigned& minor) const
{
    major = 0;
    minor = 0;

    if (client_ == NULL) {
        return false;
    }

    return client_->get_version(major, minor);
}

void HttpServletRequest::fprint_header(ostream& out, const char* prompt)
{
    if (client_) {
        client_->fprint_header(out, prompt);
    } else {
        const char* ptr = acl_getenv_list();
        if (ptr) {
            out.format("%s", ptr);
        }
    }
}

void HttpServletRequest::sprint_header(string& out, const char* prompt)
{
    if (client_) {
        client_->sprint_header(out, prompt);
    } else {
        const char* ptr = acl_getenv_list();
        if (ptr) {
            out.format("%s", ptr);
        }
    }
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
