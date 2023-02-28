#ifndef __X_DATASET_H__
#define __X_DATASET_H__

#include "xquark.h"

X_BEGIN_DECLS

#define X_DATALIST_FLAGS_MASK       0x3

typedef struct _XData XData;

typedef xpointer (*XDuplicateFunc)(xpointer data, xpointer user_data);
typedef void (*XDataForeachFunc)(XQuark key_id, xpointer data, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_datalist_init(XData **datalist);

XLIB_AVAILABLE_IN_ALL
void x_datalist_clear(XData **datalist);

XLIB_AVAILABLE_IN_ALL
xpointer x_datalist_id_get_data(XData **datalist, XQuark key_id);

XLIB_AVAILABLE_IN_ALL
void x_datalist_id_set_data_full(XData **datalist, XQuark key_id, xpointer data, XDestroyNotify destroy_func);

XLIB_AVAILABLE_IN_2_74
void x_datalist_id_remove_multiple(XData **datalist, XQuark *keys, xsize n_keys);

XLIB_AVAILABLE_IN_2_34
xpointer x_datalist_id_dup_data(XData **datalist, XQuark key_id, XDuplicateFunc dup_func, xpointer user_data);

XLIB_AVAILABLE_IN_2_34
xboolean x_datalist_id_replace_data(XData **datalist, XQuark key_id, xpointer oldval, xpointer newval, XDestroyNotify destroy, XDestroyNotify *old_destroy);

XLIB_AVAILABLE_IN_ALL
xpointer x_datalist_id_remove_no_notify(XData **datalist, XQuark key_id);

XLIB_AVAILABLE_IN_ALL
void x_datalist_foreach(XData **datalist, XDataForeachFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
void x_datalist_set_flags(XData **datalist, xuint flags);

XLIB_AVAILABLE_IN_ALL
void x_datalist_unset_flags(XData **datalist, xuint flags);

XLIB_AVAILABLE_IN_ALL
xuint x_datalist_get_flags(XData **datalist);

#define x_datalist_id_set_data(dl, q, d)            x_datalist_id_set_data_full((dl), (q), (d), NULL)
#define x_datalist_id_remove_data(dl, q)            x_datalist_id_set_data((dl), (q), NULL)
#define x_datalist_set_data_full(dl, k, d, f)       x_datalist_id_set_data_full((dl), x_quark_from_string(k), (d), (f))
#define x_datalist_remove_no_notify(dl, k)          x_datalist_id_remove_no_notify((dl), x_quark_try_string(k))
#define x_datalist_set_data(dl, k, d)               x_datalist_set_data_full((dl), (k), (d), NULL)
#define x_datalist_remove_data(dl, k)               x_datalist_id_set_data((dl), x_quark_try_string(k), NULL)

XLIB_AVAILABLE_IN_ALL
void x_dataset_destroy(xconstpointer dataset_location);

XLIB_AVAILABLE_IN_ALL
xpointer x_dataset_id_get_data(xconstpointer dataset_location, XQuark key_id);

XLIB_AVAILABLE_IN_ALL
xpointer x_datalist_get_data(XData **datalist, const xchar *key);

XLIB_AVAILABLE_IN_ALL
void x_dataset_id_set_data_full(xconstpointer dataset_location, XQuark key_id, xpointer data, XDestroyNotify destroy_func);

XLIB_AVAILABLE_IN_ALL
xpointer x_dataset_id_remove_no_notify(xconstpointer dataset_location, XQuark key_id);

XLIB_AVAILABLE_IN_ALL
void x_dataset_foreach(xconstpointer dataset_location, XDataForeachFunc func, xpointer user_data);

#define x_dataset_id_set_data(l, k, d)        x_dataset_id_set_data_full((l), (k), (d), NULL)
#define x_dataset_id_remove_data(l, k)        x_dataset_id_set_data((l), (k), NULL)
#define x_dataset_get_data(l, k)              (x_dataset_id_get_data((l), x_quark_try_string(k)))
#define x_dataset_set_data_full(l, k, d, f)   x_dataset_id_set_data_full((l), x_quark_from_string(k), (d), (f))
#define x_dataset_remove_no_notify(l, k)      x_dataset_id_remove_no_notify((l), x_quark_try_string(k))
#define x_dataset_set_data(l, k, d)           x_dataset_set_data_ful((l), (k), (d), NULL)
#define x_dataset_remove_data(l, k)           x_dataset_id_set_data((l), x_quark_try_string(k), NULL)

X_END_DECLS

#endif
