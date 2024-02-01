#ifndef __X_DATASETPRIVATE_H__
#define __X_DATASETPRIVATE_H__

#include "xatomic.h"

X_BEGIN_DECLS

#define X_DATALIST_GET_FLAGS(datalist)      ((xsize)x_atomic_pointer_get(datalist) & X_DATALIST_FLAGS_MASK)

typedef xpointer (*XDataListUpdateAtomicFunc)(XQuark key_id, xpointer *data, XDestroyNotify *destroy_notify, xpointer user_data);

xpointer x_datalist_id_update_atomic(XData **datalist, XQuark key_id, XDataListUpdateAtomicFunc callback, xpointer user_data);

X_END_DECLS

#endif
