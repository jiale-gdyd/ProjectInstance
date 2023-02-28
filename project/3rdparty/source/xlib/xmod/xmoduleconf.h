#ifndef __X_MODULE_CONF_H__
#define __X_MODULE_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define X_MODULE_IMPL_NONE              0
#define X_MODULE_IMPL_DL                1

#define X_MODULE_IMPL                   X_MODULE_IMPL_DL
#undef X_MODULE_HAVE_DLERROR

#define X_MODULE_HAVE_DLERROR
// #define X_MODULE_NEED_USCORE
// #define X_MODULE_BROKEN_RTLD_GLOBAL

#ifdef __cplusplus
}
#endif

#endif
