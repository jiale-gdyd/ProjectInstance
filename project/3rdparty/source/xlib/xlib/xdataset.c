#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xquark.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xdataset.h>
#include <xlib/xlib/xbitlock.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xlib_trace.h>
#include <xlib/xlib/xdatasetprivate.h>

#define DATALIST_LOCK_BIT                       2
#define X_DATALIST_FLAGS_MASK_INTERNAL          0x7
#define X_DATALIST_GET_POINTER(datalist)        ((XData *)((xsize)x_atomic_pointer_get(datalist) & ~(xsize)X_DATALIST_FLAGS_MASK_INTERNAL))

#define X_DATALIST_SET_POINTER(datalist, pointer)                                                       \
    X_STMT_START {                                                                                      \
        xpointer _oldv = x_atomic_pointer_get(datalist);                                                \
        xpointer _newv;                                                                                 \
        do {                                                                                            \
            _oldv = x_atomic_pointer_get(datalist);                                                     \
            _newv = (xpointer)(((xsize)_oldv & X_DATALIST_FLAGS_MASK_INTERNAL) | (xsize)pointer);       \
        } while (!x_atomic_pointer_compare_and_exchange_full((void **)datalist, _oldv, _newv, &_oldv)); \
    } X_STMT_END

typedef struct {
    XQuark         key;
    xpointer       data;
    XDestroyNotify destroy;
} XDataElt;

typedef struct _XDataset XDataset;
struct _XData {
    xuint32  len;
    xuint32  alloc;
    XDataElt data[1];
};

struct _XDataset {
    xconstpointer location;
    XData         *datalist;
};

static void x_data_initialize(void);
static inline void x_datalist_clear_i(XData **datalist);
static void x_dataset_destroy_internal(XDataset *dataset);
static inline XDataset *x_dataset_lookup(xconstpointer dataset_location);
static inline xpointer x_data_set_internal(XData **datalist, XQuark key_id, xpointer data, XDestroyNotify destroy_func, XDataset *dataset);

X_LOCK_DEFINE_STATIC(x_dataset_global);

static XDataset *x_dataset_cached = NULL;
static XHashTable *x_dataset_location_ht = NULL;

static void x_datalist_lock(XData **datalist)
{
    x_pointer_bit_lock((void **)datalist, DATALIST_LOCK_BIT);
}

static void x_datalist_unlock(XData **datalist)
{
    x_pointer_bit_unlock((void **)datalist, DATALIST_LOCK_BIT);
}

static void x_datalist_clear_i(XData **datalist)
{
    xuint i;
    XData *data;

    data = X_DATALIST_GET_POINTER (datalist);
    X_DATALIST_SET_POINTER (datalist, NULL);

    if (data) {
        X_UNLOCK(x_dataset_global);
        for (i = 0; i < data->len; i++) {
            if (data->data[i].data && data->data[i].destroy) {
                data->data[i].destroy(data->data[i].data);
            }
        }
        X_LOCK(x_dataset_global);

        x_free(data);
     }
}

void x_datalist_clear(XData **datalist)
{
    xuint i;
    XData *data;

    x_return_if_fail(datalist != NULL);

    x_datalist_lock(datalist);

    data = X_DATALIST_GET_POINTER(datalist);
    X_DATALIST_SET_POINTER(datalist, NULL);

    x_datalist_unlock(datalist);

    if (data) {
        for (i = 0; i < data->len; i++) {
            if (data->data[i].data && data->data[i].destroy) {
                data->data[i].destroy(data->data[i].data);
            }
        }

        x_free(data);
    }
}

static inline XDataset *x_dataset_lookup(xconstpointer	dataset_location)
{
    XDataset *dataset;

    if (x_dataset_cached && x_dataset_cached->location == dataset_location) {
        return x_dataset_cached;
    }

    dataset = (XDataset *)x_hash_table_lookup(x_dataset_location_ht, dataset_location);
    if (dataset) {
        x_dataset_cached = dataset;
    }

    return dataset;
}

static void x_dataset_destroy_internal(XDataset *dataset)
{
    xconstpointer dataset_location;

    dataset_location = dataset->location;
    while (dataset) {
        if (X_DATALIST_GET_POINTER(&dataset->datalist) == NULL) {
            if (dataset == x_dataset_cached) {
                x_dataset_cached = NULL;
            }

            x_hash_table_remove(x_dataset_location_ht, dataset_location);
            x_slice_free(XDataset, dataset);
            break;
        }

        x_datalist_clear_i(&dataset->datalist);
        dataset = x_dataset_lookup(dataset_location);
    }
}

void x_dataset_destroy(xconstpointer dataset_location)
{
    x_return_if_fail(dataset_location != NULL);

    X_LOCK(x_dataset_global);
    if (x_dataset_location_ht) {
        XDataset *dataset;

        dataset = x_dataset_lookup(dataset_location);
        if (dataset) {
            x_dataset_destroy_internal(dataset);
        }
    }
    X_UNLOCK(x_dataset_global);
}

static inline xpointer x_data_set_internal(XData **datalist, XQuark key_id, xpointer new_data, XDestroyNotify new_destroy_func, XDataset *dataset)
{
    XData *d, *old_d;
    XDataElt old, *data, *data_last, *data_end;

    x_datalist_lock(datalist);

    d = X_DATALIST_GET_POINTER(datalist);

    if (new_data == NULL) {
        if (d) {
            data = d->data;
            data_last = data + d->len - 1;
            while (data <= data_last) {
                if (data->key == key_id) {
                    old = *data;
                    if (data != data_last) {
                        *data = *data_last;
                    }

                    d->len--;
                    if (d->len == 0) {
                        X_DATALIST_SET_POINTER(datalist, NULL);
                        x_free(d);
                        x_datalist_unlock(datalist);

                        if (dataset) {
                            x_dataset_destroy_internal(dataset);
                        }
                    } else {
                        x_datalist_unlock(datalist);
                    }

                    if (old.destroy && !new_destroy_func) {
                        if (dataset) {
                            X_UNLOCK(x_dataset_global);
                        }

                        old.destroy (old.data);
                        if (dataset) {
                            X_LOCK(x_dataset_global);
                        }

                        old.data = NULL;
                    }

                    return old.data;
                }

                data++;
            }
        }
    } else {
        old.data = NULL;
        if (d) {
            data = d->data;
            data_end = data + d->len;
            while (data < data_end) {
                if (data->key == key_id) {
                    if (!data->destroy) {
                        data->data = new_data;
                        data->destroy = new_destroy_func;
                        x_datalist_unlock(datalist);
                    } else {
                        old = *data;
                        data->data = new_data;
                        data->destroy = new_destroy_func;

                        x_datalist_unlock(datalist);

                        if (dataset) {
                            X_UNLOCK(x_dataset_global);
                        }

                        old.destroy(old.data);
                        if (dataset) {
                            X_LOCK(x_dataset_global);
                        }
                    }

                    return NULL;
                }

                data++;
            }
        }

        old_d = d;
        if (d == NULL) {
            d = (XData *)x_malloc(sizeof(XData));
            d->len = 0;
            d->alloc = 1;
        } else if (d->len == d->alloc) {
            d->alloc = d->alloc * 2;
            d = (XData *)x_realloc(d, sizeof(XData) + (d->alloc - 1) * sizeof(XDataElt));
        }

        if (old_d != d) {
            X_DATALIST_SET_POINTER(datalist, d);
        }

        d->data[d->len].key = key_id;
        d->data[d->len].data = new_data;
        d->data[d->len].destroy = new_destroy_func;
        d->len++;
     }

    x_datalist_unlock(datalist);
    return NULL;
}

static inline void x_data_remove_internal(XData **datalist, XQuark *keys, xsize n_keys)
{
    XData *d;

    x_datalist_lock(datalist);

    d = X_DATALIST_GET_POINTER(datalist);
    if (d) {
        xsize found_keys;
        XDataElt *old, *data, *data_end;

        old = x_newa0(XDataElt, n_keys);

        data = d->data;
        data_end = data + d->len;
        found_keys = 0;

        while (data < data_end && found_keys < n_keys) {
            xboolean remove = FALSE;

            for (xsize i = 0; i < n_keys; i++) {
                if (data->key == keys[i]) {
                    old[i] = *data;
                    remove = TRUE;
                    break;
                }
            }

            if (remove) {
                XDataElt *data_last = data_end - 1;

                found_keys++;
                if (data < data_last) {
                    *data = *data_last;
                }

                data_end--;
                d->len--;
                if (d->len == 0) {
                    X_DATALIST_SET_POINTER(datalist, NULL);
                    x_free(d);
                    break;
                }
            } else {
                data++;
            }
        }

        if (found_keys > 0) {
            x_datalist_unlock(datalist);

            for (xsize i = 0; i < n_keys; i++) {
                if (old[i].destroy) {
                    old[i].destroy(old[i].data);
                }
            }

            return;
        }
    }

    x_datalist_unlock(datalist);
}

void x_dataset_id_set_data_full(xconstpointer dataset_location, XQuark key_id, xpointer data, XDestroyNotify destroy_func)
{
    XDataset *dataset;

    x_return_if_fail(dataset_location != NULL);
    if (!data) {
        x_return_if_fail(destroy_func == NULL);
    }

    if (!key_id) {
        if (data) {
            x_return_if_fail(key_id > 0);
        } else {
            return;
        }
    }

    X_LOCK(x_dataset_global);
    if (!x_dataset_location_ht) {
        x_data_initialize();
    }

    dataset = x_dataset_lookup(dataset_location);
    if (!dataset) {
        dataset = x_slice_new(XDataset);
        dataset->location = dataset_location;
        x_datalist_init(&dataset->datalist);
        x_hash_table_insert(x_dataset_location_ht, (xpointer)dataset->location, dataset);
    }

    x_data_set_internal(&dataset->datalist, key_id, data, destroy_func, dataset);
    X_UNLOCK(x_dataset_global);
}

void x_datalist_id_set_data_full(XData **datalist, XQuark key_id, xpointer data, XDestroyNotify destroy_func)
{
    x_return_if_fail(datalist != NULL);
    if (!data) {
        x_return_if_fail(destroy_func == NULL);
    }

    if (!key_id) {
        if (data) {
            x_return_if_fail(key_id > 0);
        } else {
            return;
        }
    }

    x_data_set_internal(datalist, key_id, data, destroy_func, NULL);
}

void x_datalist_id_remove_multiple(XData **datalist, XQuark *keys, xsize n_keys)
{
    x_return_if_fail(n_keys <= 16);
    x_data_remove_internal(datalist, keys, n_keys);
}

xpointer x_dataset_id_remove_no_notify(xconstpointer dataset_location, XQuark key_id)
{
    xpointer ret_data = NULL;

    x_return_val_if_fail(dataset_location != NULL, NULL);

    X_LOCK(x_dataset_global);
    if (key_id && x_dataset_location_ht) {
        XDataset *dataset;

        dataset = x_dataset_lookup(dataset_location);
        if (dataset) {
            ret_data = x_data_set_internal(&dataset->datalist, key_id, NULL, (XDestroyNotify)42, dataset);
        }
    }
    X_UNLOCK(x_dataset_global);

    return ret_data;
}

xpointer x_datalist_id_remove_no_notify(XData **datalist, XQuark key_id)
{
    xpointer ret_data = NULL;

    x_return_val_if_fail(datalist != NULL, NULL);

    if (key_id) {
        ret_data = x_data_set_internal(datalist, key_id, NULL, (XDestroyNotify)42, NULL);
    }

    return ret_data;
}

xpointer x_dataset_id_get_data(xconstpointer dataset_location, XQuark key_id)
{
    xpointer retval = NULL;

    x_return_val_if_fail(dataset_location != NULL, NULL);

    X_LOCK(x_dataset_global);
    if (key_id && x_dataset_location_ht) {
        XDataset *dataset;

        dataset = x_dataset_lookup(dataset_location);
        if (dataset) {
            retval = x_datalist_id_get_data(&dataset->datalist, key_id);
        }
    }
    X_UNLOCK(x_dataset_global);

    return retval;
}

xpointer x_datalist_id_get_data(XData **datalist, XQuark key_id)
{
    return x_datalist_id_dup_data(datalist, key_id, NULL, NULL);
}

xpointer x_datalist_id_dup_data(XData **datalist, XQuark key_id, XDuplicateFunc dup_func, xpointer user_data)
{
    XData *d;
    xpointer val = NULL;
    xpointer retval = NULL;
    XDataElt *data, *data_end;

    x_datalist_lock(datalist);

    d = X_DATALIST_GET_POINTER(datalist);
    if (d) {
        data = d->data;
        data_end = data + d->len;

        do {
            if (data->key == key_id) {
                val = data->data;
                break;
            }

            data++;
        } while (data < data_end);
    }

    if (dup_func) {
        retval = dup_func(val, user_data);
    } else {
        retval = val;
    }

    x_datalist_unlock(datalist);
    return retval;
}

xboolean x_datalist_id_replace_data(XData **datalist, XQuark key_id, xpointer oldval, xpointer newval, XDestroyNotify destroy, XDestroyNotify *old_destroy)
{
    XData *d;
    xpointer val = NULL;
    XDataElt *data, *data_end;

    x_return_val_if_fail(datalist != NULL, FALSE);
    x_return_val_if_fail(key_id != 0, FALSE);

    if (old_destroy) {
        *old_destroy = NULL;
    }

    x_datalist_lock(datalist);

    d = X_DATALIST_GET_POINTER(datalist);
    if (d) {
        data = d->data;
        data_end = data + d->len - 1;

        while (data <= data_end) {
            if (data->key == key_id) {
                val = data->data;
                if (val == oldval) {
                    if (old_destroy) {
                        *old_destroy = data->destroy;
                    }

                    if (newval != NULL) {
                        data->data = newval;
                        data->destroy = destroy;
                    } else {
                        if (data != data_end) {
                            *data = *data_end;
                        }

                        d->len--;
                        if (d->len == 0) {
                            X_DATALIST_SET_POINTER(datalist, NULL);
                            x_free(d);
                        }
                    }
                }

                break;
            }

            data++;
        }
    }

    if (val == NULL && oldval == NULL && newval != NULL) {
        XData *old_d;

        old_d = d;
        if (d == NULL) {
            d = (XData *)x_malloc(sizeof(XData));
            d->len = 0;
            d->alloc = 1;
        } else if (d->len == d->alloc) {
            d->alloc = d->alloc * 2;
            d = (XData *)x_realloc(d, sizeof(XData) + (d->alloc - 1) * sizeof(XDataElt));
        }

        if (old_d != d) {
            X_DATALIST_SET_POINTER(datalist, d);
        }

        d->data[d->len].key = key_id;
        d->data[d->len].data = newval;
        d->data[d->len].destroy = destroy;
        d->len++;
    }

    x_datalist_unlock(datalist);

    return val == oldval;
}

xpointer x_datalist_get_data(XData **datalist, const xchar *key)
{
    XData *d;
    xpointer res = NULL;
    XDataElt *data, *data_end;

    x_return_val_if_fail(datalist != NULL, NULL);

    x_datalist_lock(datalist);

    d = X_DATALIST_GET_POINTER (datalist);
    if (d) {
        data = d->data;
        data_end = data + d->len;
        while (data < data_end) {
            if (x_strcmp0(x_quark_to_string(data->key), key) == 0) {
                res = data->data;
                break;
            }

            data++;
        }
    }

    x_datalist_unlock(datalist);

    return res;
}

void x_dataset_foreach(xconstpointer dataset_location, XDataForeachFunc func, xpointer user_data)
{
    XDataset *dataset;

    x_return_if_fail(dataset_location != NULL);
    x_return_if_fail(func != NULL);

    X_LOCK(x_dataset_global);
    if (x_dataset_location_ht) {
        dataset = x_dataset_lookup(dataset_location);
        X_UNLOCK(x_dataset_global);
        if (dataset) {
            x_datalist_foreach(&dataset->datalist, func, user_data);
        }
    } else {
        X_UNLOCK(x_dataset_global);
    }
}

void x_datalist_foreach(XData **datalist, XDataForeachFunc func, xpointer user_data)
{
    XData *d;
    XQuark *keys;
    xuint i, j, len;

    x_return_if_fail(datalist != NULL);
    x_return_if_fail(func != NULL);

    d = X_DATALIST_GET_POINTER(datalist);
    if (d == NULL) {
        return;
    }

    len = d->len;
    keys = x_new(XQuark, len);
    for (i = 0; i < len; i++) {
        keys[i] = d->data[i].key;
    }

    for (i = 0; i < len; i++) {
        d = X_DATALIST_GET_POINTER(datalist);
        if (d == NULL) {
            break;
        }

        for (j = 0; j < d->len; j++) {
            if (d->data[j].key == keys[i]) {
                func(d->data[i].key, d->data[i].data, user_data);
                break;
            }
        }
    }

    x_free(keys);
}

void x_datalist_init(XData **datalist)
{
    x_return_if_fail(datalist != NULL);
    x_atomic_pointer_set(datalist, NULL);
}

void x_datalist_set_flags(XData **datalist, xuint flags)
{
    x_return_if_fail(datalist != NULL);
    x_return_if_fail((flags & ~X_DATALIST_FLAGS_MASK) == 0);

    x_atomic_pointer_or(datalist, (xsize)flags);
}

void x_datalist_unset_flags(XData **datalist, xuint flags)
{
    x_return_if_fail(datalist != NULL);
    x_return_if_fail((flags & ~X_DATALIST_FLAGS_MASK) == 0);

    x_atomic_pointer_and (datalist, ~(xsize)flags);
}

xuint x_datalist_get_flags (XData **datalist)
{
    x_return_val_if_fail(datalist != NULL, 0);
    return X_DATALIST_GET_FLAGS(datalist);
}

static void x_data_initialize(void)
{
    x_return_if_fail(x_dataset_location_ht == NULL);

    x_dataset_location_ht = x_hash_table_new(x_direct_hash, NULL);
    x_dataset_cached = NULL;
}
