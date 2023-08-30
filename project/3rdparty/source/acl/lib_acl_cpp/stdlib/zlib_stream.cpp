#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#include "zlib/zlib.h"

#ifndef ACL_PREPARE_COMPILE
#include "acl/lib_acl_cpp/stdlib/log.hpp"
#include "acl/lib_acl_cpp/stdlib/string.hpp"
#include "acl/lib_acl_cpp/stdlib/zlib_stream.hpp"
#endif

#ifndef HAS_ZLIB
# define HAS_ZLIB
#endif

#if defined(HAS_ZLIB) || defined(HAS_ZLIB_DLL)
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)

//typedef int (*deflateInit_fn)(z_stream*, int, const char*, int);
typedef int (*deflateInit2_fn)(z_stream*, int, int, int, int, int, const char*, int);
typedef int (*deflate_fn)(z_stream*, int);
typedef int (*deflateReset_fn)(z_stream*);
typedef int (*deflateEnd_fn)(z_stream*);

typedef int (*inflateInit2_fn)(z_stream*, int, const char*, int);
typedef int (*inflate_fn)(z_stream*, int);
typedef int (*inflateReset_fn)(z_stream*);
typedef int (*inflateEnd_fn)(z_stream*);

typedef unsigned long (*crc32_fn)(unsigned long, const Bytef*, unsigned);

//static deflateInit_fn __deflateInit = NULL;
static deflateInit2_fn __deflateInit2 = NULL;
static deflate_fn __deflate = NULL;
static deflateReset_fn __deflateReset = NULL;
static deflateEnd_fn __deflateEnd = NULL;

static inflateInit2_fn __inflateInit2 = NULL;
static inflate_fn __inflate = NULL;
static inflateReset_fn __inflateReset = NULL;
static inflateEnd_fn __inflateEnd = NULL;

static crc32_fn __crc32 = NULL;

static acl_pthread_once_t __zlib_once = ACL_PTHREAD_ONCE_INIT;
static ACL_DLL_HANDLE __zlib_dll = NULL;
static acl::string __zlib_path;

// �����˳�ʱ�ͷŶ�̬���ص� zlib.dll ��
#ifndef HAVE_NO_ATEXIT
static void __zlib_dll_unload(void)
{
    if (__zlib_dll != NULL) {
        acl_dlclose(__zlib_dll);
        __zlib_dll = NULL;
        //logger("%s unload ok", __zlib_path.c_str());
    }
}
#endif

// ���� zlib ��̬���еĺ�������
static bool __zlib_dll_load_symbols(void)
{
    if (__zlib_dll == NULL) {
        logger_error("__zlib_dll null, zlib hasn't been loaded");
        return false;
    }

    const char* path = __zlib_path.c_str();

    //__deflateInit = (deflateInit_fn) acl_dlsym(__zlib_dll, "deflateInit_");
    //if (__deflateInit == NULL) {
    //    logger_fatal("load deflateInit from %s error: %s",
    //        path, acl_dlerror());
    //    return false;
    //}

    __deflateInit2 = (deflateInit2_fn) acl_dlsym(__zlib_dll, "deflateInit2_");
    if (__deflateInit2 == NULL) {
        logger_error("load deflateInit from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __deflate = (deflate_fn) acl_dlsym(__zlib_dll, "deflate");
    if (__deflate == NULL) {
        logger_error("load deflate from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __deflateReset = (deflateReset_fn) acl_dlsym(__zlib_dll, "deflateReset");
    if (__deflateReset == NULL) {
        logger_error("load deflateReset from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __deflateEnd = (deflateEnd_fn) acl_dlsym(__zlib_dll, "deflateEnd");
    if (__deflateEnd == NULL) {
        logger_error("load deflateEnd from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __inflateInit2 = (inflateInit2_fn) acl_dlsym(__zlib_dll, "inflateInit2_");
    if (__inflateInit2 == NULL) {
        logger_error("load inflateInit from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __inflate = (inflate_fn) acl_dlsym(__zlib_dll, "inflate");
    if (__inflate == NULL) {
        logger_error("load inflate from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __inflateReset = (inflateReset_fn) acl_dlsym(__zlib_dll, "inflateReset");
    if (__inflateReset == NULL) {
        logger_error("load inflateReset from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __inflateEnd = (inflateEnd_fn) acl_dlsym(__zlib_dll, "inflateEnd");
    if (__inflateEnd == NULL) {
        logger_error("load inflateEnd from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    __crc32 = (crc32_fn) acl_dlsym(__zlib_dll, "crc32");
    if (__crc32 == NULL) {
        logger_error("load __crc32 from %s error: %s",
            path, acl_dlerror());
        return false;
    }

    return true;
}

// ��̬���� zlib.dll ��
static void __zlib_dll_load(void)
{
    if (__zlib_dll != NULL) {
        logger_warn("__zlib_dll not null, zlib has been loaded");
        return;
    }

    const char* path;
    const char* ptr = acl::zlib_stream::get_loadpath();
    if (ptr) {
        path = ptr;
    } else {
#ifdef ACL_WINDOWS
        path = "zlib.dll";
#else
        path = "libz.so";
#endif
    }

    __zlib_dll = acl_dlopen(path);
    if (__zlib_dll == NULL) {
        logger_error("load %s error: %s", path, acl_dlerror());
        return;
    }

    // ��¼��̬��·�����Ա����ڶ�̬��ж��ʱ�����·����
    __zlib_path = path;

    if (!__zlib_dll_load_symbols()) {
        logger_error("load zlib functions symbols error");
        __zlib_dll_unload();
        return;
    }

    logger("%s loaded", path);
#ifndef HAVE_NO_ATEXIT
    atexit(__zlib_dll_unload);
#endif
}

# else
//#  define __deflateInit         deflateInit_
#  define __deflateInit2        deflateInit2_
#  define __deflate             deflate
#  define __deflateReset    deflateReset
#  define __deflateEnd          deflateEnd
#  define __inflateInit2        inflateInit2_
#  define __inflate             inflate
#  define __inflateReset    inflateReset
#  define __inflateEnd          inflateEnd
#  define __crc32               crc32
# endif

namespace acl
{

static void* __zlib_calloc(void* ctx acl_unused,
    unsigned int nitem, unsigned int size)
{
    return (acl_mymalloc(nitem * size));
}

static void __zlib_free(void* ctx acl_unused, void* ptr)
{
    acl_myfree(ptr);
}

static string __loadpath;

void zlib_stream::set_loadpath(const char* path)
{
    if (path && *path) {
        __loadpath = path;
    }
}

const char* zlib_stream::get_loadpath(void)
{
    return __loadpath.empty() ? NULL : __loadpath.c_str();
}

bool zlib_stream::zlib_load_once(void)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    // ����Ҫ���� zlib ��̬��ʱ����Ҫ��֤���ع��̵�Ψһ��
    acl_pthread_once(&__zlib_once, __zlib_dll_load);
    if (__zlib_dll != NULL) {
        return true;
    }
    return false;
# else
    return true;  // ���ھ�̬�⣬��ֱ�ӷ��سɹ�����
# endif
    logger_warn("zlib not support!");
    return false;
#endif
}

zlib_stream::zlib_stream(void)
{
    zlib_flags_      = 0;
    finished_        = false;
    zstream_         = (z_stream*) acl_mycalloc(1, sizeof(z_stream));
    zstream_->zalloc = __zlib_calloc;
    zstream_->zfree  = __zlib_free;
    zstream_->opaque = (void*) this;

    is_compress_     = true;  // Ĭ��Ϊѹ��״̬
    flush_           = zlib_flush_off;

#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    zlib_load_once();
# endif
#endif
}

zlib_stream::~zlib_stream(void)
{
    if ((zlib_flags_ & zlib_flags_zip_begin)
          && !(zlib_flags_ & zlib_flags_zip_end)) {
        string dummy(256);
        (void) zip_finish(&dummy);
    }

    if ((zlib_flags_ & zlib_flags_unzip_begin)
          && !(zlib_flags_ & zlib_flags_unzip_end)) {
        string dummy(256);
        (void) unzip_finish(&dummy);
    }

    acl_myfree(zstream_);
}

bool zlib_stream::zlib_compress(const char* in, int len, string* out,
    zlib_level_t level /* = zlib_default */)
{
    if (!zip_begin(level)) {
        zip_reset();
        return false;
    }

    if (!zip_update(in, len, out, zlib_flush_sync)) {
        zip_reset();
        return false;
    }

    return zip_finish(out);
}

bool zlib_stream::zlib_uncompress(const char* in, int len, string* out,
    bool have_zlib_header /* = true */, int wsize /* = 15 */)
{
    if (!unzip_begin(have_zlib_header, wsize)) {
        unzip_reset();
        return false;
    }

    if (!unzip_update(in, len, out, zlib_flush_sync)) {
        unzip_reset();
        return false;
    }

    return unzip_finish(out);
}

#define BUF_MIN    4000

bool zlib_stream::update(int (*func)(z_stream*, int),
    zlib_flush_t flag, const char* in, int len, string* out)
{
    if (func == NULL) {
        logger_warn("func null, zlib maybe not loaded yet");
        return false;
    }

    if (finished_) {
        return true;
    }

    if (in == NULL || len < 0 || out == NULL) {
        logger_warn("invalid args, in=%p, len=%d, out=%p", in, len, out);
        return false;
    }

    int   pos = 0;
    int   dlen, nbuf, ret;

    zstream_->avail_out = 0;

    while (true) {
        if (len < 0) {
            logger_error("when update, invalid len=%d", len);
            return false;
        }

        nbuf = (int) (out->capacity() - out->length());

        // ��Ҫ��֤����������Ŀ��ÿռ�
        if (nbuf < BUF_MIN) {
            nbuf = (int) out->length() + BUF_MIN;
            out->space(nbuf);
        }

        dlen = (int) out->length();
        nbuf = (int) out->capacity() - dlen;
        if (nbuf < BUF_MIN) {
            logger_error("no space available, nbuf: %d < %d",
                nbuf, BUF_MIN);
            return false;
        }

        zstream_->next_in   = (unsigned char*) in + pos;
        zstream_->avail_in  = (unsigned int) len;
        zstream_->next_out  = (unsigned char*) out->c_str() + dlen;
        zstream_->avail_out = (unsigned int) nbuf;

        ret = func(zstream_, flag);
        if (ret == Z_STREAM_END) {
            finished_ = true;

            if (flag != Z_FINISH && func != __inflate) {
                logger_error("flag=%d, func=%p, inflate=%p",
                    flag, func, __inflate);
                return false;
            }

            // �޸������������ָ��λ��
            if (nbuf < (int) zstream_->avail_out) {
                logger_error("Pos err, nbuf=%d, avail_out=%d",
                    nbuf, (int) zstream_->avail_out);
                return false;
            }

            dlen += nbuf - zstream_->avail_out;
            out->set_offset((ssize_t) dlen);

            if (zstream_->avail_in == 0) {
                zstream_->next_in = NULL;
            }

            return true;
        } else if (ret != Z_OK) {
            logger_error("update(%s) error",
                func == __deflate ? "deflate" : "inflate");
            return false;
        }

        // �޸������������ָ��λ��
        if (nbuf < (int) zstream_->avail_out) {
            logger_error("nbuf=%d, avail_out=%d",
                nbuf, (int) zstream_->avail_out);
            return false;
        }

        dlen += nbuf - zstream_->avail_out;
        out->set_offset((ssize_t) dlen);

        // ����������������˳�ѭ��
        if (zstream_->avail_in == 0) {
            zstream_->next_in = NULL;
            break;
        }

        // �����������ݵ���һ��λ��
        if (len < (int) zstream_->avail_in) {
            logger_error("Pos err, len=%d, avail_in=%d",
                len, (int) zstream_->avail_in);
            return false;
        }

        pos += len - zstream_->avail_in;

        // ����ʣ�����ݳ���
        len = zstream_->avail_in;
    }

    return true;
}

bool zlib_stream::flush_out(int (*func)(z_stream*, int),
    zlib_flush_t flag, string* out)
{
    if (func == NULL) {
        logger_warn("func null, zlib maybe not loaded yet");
        return false;
    }

    if (finished_) {
        return true;
    }

    if (zstream_->avail_in != 0) {
        logger_warn("not completed, avali_in not 0");
        return false;
    }

    if (zstream_->next_in != NULL) {
        logger_warn("not completed, next_in not NULL");
        return false;
    }

    int   dlen, nbuf, ret;

    nbuf = 0;
    while (true) {
        nbuf = (int) (out->capacity() - out->length());

        // ��Ҫ��֤����������Ŀ��ÿռ�
        if (nbuf < BUF_MIN) {
            nbuf = (int) out->length() + BUF_MIN;
            out->space(nbuf);
        }

        dlen = (int) out->length();
        nbuf = (int) out->capacity() - dlen;
        if (nbuf < BUF_MIN) {
            logger_error("no space available, nbuf: %d < %d",
                nbuf, BUF_MIN);
            return false;
        }

        zstream_->next_out = (unsigned char*) out->c_str() + dlen;
        zstream_->avail_out = (unsigned int) nbuf;

        ret = func(zstream_, flag);
        if (ret == Z_STREAM_END) {
            finished_ = true;
            if (flag != Z_FINISH && func != __inflate) {
                logger_error("flag=%d, fnc=%p, inflate=%p",
                    flag, func, __inflate);
                return false;
            }

            // �޸������������ָ��λ��
            if (nbuf < (int) zstream_->avail_out) {
                logger_error("Pos err, nbuf=%d, avail_out=%d",
                    nbuf, (int) zstream_->avail_out);
                return false;
            }

            dlen += nbuf - zstream_->avail_out;
            out->set_offset((ssize_t) dlen);

            if (zstream_->avail_in == 0) {
                zstream_->next_in = NULL;
            }
            break;
        } else if (ret == Z_BUF_ERROR) {
            if (zstream_->avail_out > 0) {
                logger_error("flush_out(%s) error",
                    func == __deflate ?
                    "deflate" : "inflate");
                return false;
            }

            // �޸������������ָ��λ��
            if (nbuf < (int) zstream_->avail_out) {
                logger_error("Pos err, nbuf=%d, avail_out=%d",
                    nbuf, (int) zstream_->avail_out);
                return false;
            }

            dlen += nbuf - zstream_->avail_out;
            out->set_offset((ssize_t) dlen);
        } else if (ret != Z_OK) {
            logger_error("update(%s) error", func == __deflate ?
                "deflate" : "inflate");
            return false;
        } else if (zstream_->avail_out == 0) {
            // �޸������������ָ��λ��
            if (nbuf < (int) zstream_->avail_out) {
                logger_error("Pos err, nbuf=%d, avail_out=%d",
                    nbuf, (int) zstream_->avail_out);
                return false;
            }

            dlen += nbuf - zstream_->avail_out;
            out->set_offset((size_t) dlen);
        } else {
            break;
        }
    }

    return true;
}

bool zlib_stream::zip_begin(zlib_level_t level /* = zlib_default */,
    int wbits /* = zlib_wbits_15 */,
    zlib_mlevel_t mlevel /* = zlib_memlevel_9 */)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__deflateInit2 == NULL) {
        logger_warn("__deflateInit2 null, zlib maybe not loaded yet");
        return false;
    }
# endif
#endif

    zlib_flags_  = zlib_flags_zip_begin;
    finished_    = false;
    is_compress_ = true;
//    int ret = __deflateInit(zstream_, level, ZLIB_VERSION, sizeof(z_stream));

    int ret = __deflateInit2(zstream_, level, Z_DEFLATED,
            wbits, mlevel, Z_DEFAULT_STRATEGY, ZLIB_VERSION,
            (int) sizeof(z_stream));
    if (ret != Z_OK) {
        logger_error("deflateInit error");
        return false;
    }
    return true;
}

bool zlib_stream::zip_update(const char* in, int len, string* out,
    zlib_flush_t flag /* = zlib_flush_off */)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__deflate == NULL) {
        logger_warn("__deflate null, zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    return update(__deflate, flag, in, len, out);
}

bool zlib_stream::zip_finish(string* out)
{
    if (zlib_flags_ & zlib_flags_zip_end) {
        return true;
    }

    zlib_flags_ &= ~zlib_flags_zip_begin;
    zlib_flags_ |= zlib_flags_zip_end;

#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__deflate == NULL || __deflateEnd == NULL) {
        logger_warn("zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    bool ret = flush_out(__deflate, zlib_flush_finish, out);
    //(void) __deflateReset(zstream_);
    __deflateEnd(zstream_);
    return ret;
}

bool zlib_stream::zip_reset(void)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__deflateEnd == NULL) {
        logger_warn("__deflateEnd null, zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    return __deflateEnd(zstream_) == Z_OK ? true : false;
}

unsigned zlib_stream::crc32_update(unsigned n, const void* buf, size_t dlen)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__crc32 == NULL) {
        logger_warn("__crc32 null, zlib maybe not loaded yet");
        return 0;
    }
# endif
#endif
    return (unsigned) __crc32(n, (const Bytef*) buf, (unsigned) dlen);
}

bool zlib_stream::unzip_begin(bool have_zlib_header /* = true */,
    int wsize /* = 15 */)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__inflateInit2 == NULL) {
        logger_warn("__inflateInit2 null, zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    zlib_flags_  = zlib_flags_unzip_begin;
    finished_    = false;
    is_compress_ = false;

    int   ret = __inflateInit2(zstream_, have_zlib_header ?
        wsize : -wsize, ZLIB_VERSION, sizeof(z_stream));
    if (ret != Z_OK) {
        logger_error("inflateInit error");
        return (false);
    }
    return true;
}

bool zlib_stream::unzip_update(const char* in, int len, string* out,
    zlib_flush_t flag /* = zlib_flush_off */)
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__inflate == NULL) {
        logger_warn("__inflate null, zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    return update(__inflate, flag, in, len, out);
}

bool zlib_stream::unzip_finish(string* out)
{
    if (zlib_flags_ & zlib_flags_unzip_end) {
        return true;
    }

    zlib_flags_ &= ~zlib_flags_unzip_begin;
    zlib_flags_ |= zlib_flags_unzip_end;

#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__inflate == NULL || __inflateEnd == NULL) {
        logger_warn("zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    bool ret = flush_out(__inflate, zlib_flush_finish, out);
    //(void) __inflateReset(zstream_);
    __inflateEnd(zstream_);
    return ret;
}

bool zlib_stream::unzip_reset()
{
#ifdef  HAS_ZLIB
# if defined(ACL_CPP_DLL) || defined(HAS_ZLIB_DLL)
    if (__inflateEnd == NULL) {
        logger_warn("__inflate null, zlib maybe not loaded yet");
        return false;
    }
# endif
#endif
    return __inflateEnd(zstream_) == Z_OK ? true : false;
}

bool zlib_stream::pipe_zip_begin(zlib_level_t level /* = zlib_default */,
    zlib_flush_t flag /* = zlib_flush_off */)
{
    flush_ = flag;
    return zip_begin(level);
}

bool zlib_stream::pipe_unzip_begin(zlib_flush_t flag /* = zlib_flush_off */)
{
    flush_ = flag;
    return unzip_begin();
}

int zlib_stream::push_pop(const char* in, size_t len,
    string* out, size_t max /* = 0 */ acl_unused)
{
    if (out == NULL) {
        return 0;
    }

    size_t n = out->length();

    if (is_compress_) {
        if (!zip_update(in, (int) len, out, flush_)) {
            return -1;
        }
    } else {
        if (!unzip_update(in, (int) len, out, flush_)) {
            return -1;
        }
    }

    return (int) (out->length() - n);
}

int zlib_stream::pop_end(string* out, size_t max /* = 0 */ acl_unused)
{
    if (out == NULL) {
        return 0;
    }

    size_t n = out->length();
    if (is_compress_) {
        if (!zip_finish(out)) {
            return -1;
        }
    } else {
        if (!unzip_finish(out)) {
            return -1;
        }
    }

    return (int) (out->length() - n);
}

void zlib_stream::clear(void)
{
    if (is_compress_) {
        zip_reset();
    } else {
        unzip_reset();
    }
}

} // namespace acl

#else  // !HAS_ZLIB && !HAS_ZLIB_DLL
# error "You should define HAS_ZLIB or HAS_ZLIB_DLL first!"
#endif