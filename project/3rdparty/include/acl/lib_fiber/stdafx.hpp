#pragma once

#if defined(__linux__) || defined(MINGW)
#include <sys/select.h>
#include <strings.h>
#endif

#include "../lib_acl/lib_acl.h"
#include "../lib_acl_cpp/lib_acl.hpp"
#include "c/libfiber.h"

#define FIBER_DEBUG_MIN     71
#define FIBER_DEBUG_KEEPER  (FIBER_DEBUG_MIN + 1)
