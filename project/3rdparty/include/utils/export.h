#ifndef UTILS_EXPORT_H
#define UTILS_EXPORT_H

#if defined(__GNUC__)
#define API_HIDDEN                   __attribute__((visibility("hidden")))
#define API_EXPORT                   __attribute__((visibility("default")))
#define API_INTERNAL                 __attribute__((visibility("internal")))
#define API_PROTECTED                __attribute__((visibility("protected")))
#else
#define API_HIDDEN
#define API_EXPORT
#define API_INTERNAL
#define API_PROTECTED
#endif

#if defined(__GNUC__)
#define API_DEPERATED                __attribute__((deprecated))
#else
#define API_DEPERATED
#endif

#if defined(__GNUC__)
#define API_UNUSED                   __attribute__((unused))
#else
#define API_UNUSED
#endif

#define API_INLINE                   inline __attribute__((__always_inline__))

#ifdef __cplusplus

#ifndef API_EXTERN_C
#define API_EXTERN_C                 extern "C"
#endif

#ifndef API_BEGIN_EXTERN_C
#define API_BEGIN_EXTERN_C           extern "C" {
#endif

#ifndef API_END_EXTERN_C
#define API_END_EXTERN_C             }
#endif

#ifndef API_BEGIN_NAMESPACE
#define API_BEGIN_NAMESPACE(ns)      namespace ns {
#endif

#ifndef API_END_NAMESPACE
#define API_END_NAMESPACE(ns)        }
#endif

#ifndef API_USING_NAMESPACE
#define API_USING_NAMESPACE(ns)      using namespace ns;
#endif

#ifndef API_DEFAULT
#define API_DEFAULT(x)               = x
#endif

#ifndef API_ENUM
#define API_ENUM(e)                  enum e
#endif

#ifndef API_STRUCT
#define API_STRUCT(s)                struct s
#endif

#else

#ifndef API_EXTERN_C
#define API_EXTERN_C                 extern
#endif

#ifndef API_BEGIN_EXTERN_C
#define API_BEGIN_EXTERN_C
#endif

#ifndef API_END_EXTERN_C
#define API_END_EXTERN_C
#endif

#ifndef API_BEGIN_NAMESPACE
#define API_BEGIN_NAMESPACE(ns)
#endif

#ifndef API_END_NAMESPACE
#define API_END_NAMESPACE(ns)
#endif

#ifndef API_USING_NAMESPACE
#define API_USING_NAMESPACE(ns)
#endif

#ifndef API_DEFAULT
#define API_DEFAULT(x)
#endif

#ifndef API_ENUM
#define API_ENUM(e)                 \
    typedef enum e e;               \
    enum e
#endif

#ifndef API_STRUCT
#define API_STRUCT(s)               \
    typedef struct s s;             \
    struct s
#endif

#endif

#ifndef __cplusplus
#ifndef inline
#define inline                      __inline
#endif
#endif

#endif
