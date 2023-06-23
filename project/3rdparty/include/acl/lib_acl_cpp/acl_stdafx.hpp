#pragma once

#include "acl_cpp_define.hpp"

#include "../lib_acl/lib_acl.h"
#include "../lib_protocol/lib_protocol.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <vector>
#include <map>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "stdlib/malloc.hpp"

#ifndef ACL_CPP_DEBUG_MEM

#define NEW new

#endif

#if defined(ACL_UNIX)
#include <pthread.h>
#include <unistd.h>
#include "zlib/zlib.h"
#endif

// 加入下面一行可以加快在 VC 下的编译速度
//#if defined(_WIN32) || defined(_WIN64)
#include "lib_acl.hpp"
//#endif

#define ACL_CPP_DEBUG_MIN               40
#define ACL_CPP_DEBUG_CONN_MANAGER      41
#define ACL_CPP_DEBUG_HTTP_NET          42
#define ACL_CPP_DEBUG_MAX               70

