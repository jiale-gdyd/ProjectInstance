#ifndef __XLIB_TYPEOF_H__
#define __XLIB_TYPEOF_H__

#include "xversionmacros.h"

#undef xlib_typeof

#if !X_CXX_STD_CHECK_VERSION(11) && (X_GNUC_CHECK_VERSION(4, 8) || defined(__clang__))
#define xlib_typeof(t)      __typeof__ (t)
#elif X_CXX_STD_CHECK_VERSION(11) && XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_68
#include <type_traits>
#define xlib_typeof(t)      typename std::remove_reference<decltype (t)>::type
#endif

#endif
