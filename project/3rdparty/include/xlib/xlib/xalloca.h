#ifndef __X_ALLOCA_H__
#define __X_ALLOCA_H__

#include <string.h>
#include "xtypes.h"

#undef alloca
#define alloca(size)                        __builtin_alloca(size)

#define x_alloca(size)                      alloca(size)
#define x_alloca0(size)                     ((size) == 0 ? NULL : memset(x_alloca(size), 0, (size)))

#define x_newa(struct_type, n_structs)      ((struct_type *)x_alloca(sizeof(struct_type) * (xsize)(n_structs)))
#define x_newa0(struct_type, n_structs)     ((struct_type *)x_alloca0(sizeof(struct_type) * (xsize)(n_structs)))

#endif
