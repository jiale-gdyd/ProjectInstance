#ifndef __X_DATASETPRIVATE_H__
#define __X_DATASETPRIVATE_H__

#include "xatomic.h"

X_BEGIN_DECLS

#define X_DATALIST_GET_FLAGS(datalist)      ((xsize)x_atomic_pointer_get(datalist) & X_DATALIST_FLAGS_MASK)

X_END_DECLS

#endif
