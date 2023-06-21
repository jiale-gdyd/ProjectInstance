#pragma once

#ifdef ACL_CPP_LIB
#ifndef ACL_CPP_API
#define ACL_CPP_API
#endif
#elif defined(ACL_CPP_DLL)
#if defined(ACL_CPP_EXPORTS) || defined(acl_cpp_EXPORTS)
#ifndef ACL_CPP_API
#define ACL_CPP_API                             __declspec(dllexport)
#endif
#elif !defined(ACL_CPP_API)
#define ACL_CPP_API                             __declspec(dllimport)
#endif
#elif !defined(ACL_CPP_API)
#define ACL_CPP_API
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_MEMCACHED
#undef HAVE_MEMCACHED
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define ACL_CPP_PRINTF(format_idx, arg_idx)     __attribute__((__format__ (__printf__, (format_idx), (arg_idx))))
#define ACL_CPP_SCANF(format_idx, arg_idx)      __attribute__((__format__ (__scanf__, (format_idx), (arg_idx))))
#define ACL_CPP_NORETURN                        __attribute__((__noreturn__))
#define ACL_CPP_UNUSED                          __attribute__((__unused__))
#else
#define ACL_CPP_PRINTF(format_idx, arg_idx)
#define ACL_CPP_SCANF
#define ACL_CPP_NORETURN
#define ACL_CPP_UNUSED
#endif

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define ACL_CPP_DEPRECATED                      __attribute__((__deprecated__))
#else
#define ACL_CPP_DEPRECATED
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define ACL_CPP_DEPRECATED_FOR(f)               __attribute__((deprecated("Use " #f " instead")))
#else
#define ACL_CPP_DEPRECATED_FOR(f)               ACL_CPP_DEPRECATED
#endif

#if defined(__GNUC__) && (__GNUC__ > 6 ||(__GNUC__ == 6 && __GNUC_MINOR__ >= 0))
#ifndef ACL_USE_CPP11
#define ACL_USE_CPP11
#endif
#endif
