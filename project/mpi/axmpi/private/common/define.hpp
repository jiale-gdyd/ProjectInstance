#pragma once

#include <ax_sys_api.h>
#include <ax_ives_api.h>

namespace axpi {
#ifndef ALIGN_UP
#define ALIGN_UP(x, align)              ((((x) + ((align)-1)) / (align)) * (align))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align)            (((x) / (align)) * (align))
#endif

extern int g_majorStreamWidth;
extern int g_majorStreamHeight;
}
