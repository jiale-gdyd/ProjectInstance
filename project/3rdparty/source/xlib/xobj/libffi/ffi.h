#ifndef __PRIVATE_LIBFFI_H__
#define __PRIVATE_LIBFFI_H__

#include <linux/kconfig.h>

#if defined(CONFIG_AARCH32)
#include "aarch32/ffi.h"
#elif defined(CONFIG_AARCH64)
#include "aarch64/ffi.h"
#else
#include "x86_64/ffi.h"
#endif

#endif
