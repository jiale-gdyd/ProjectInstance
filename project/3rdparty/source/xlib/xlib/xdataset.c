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

#define X_DATALIST_CLEAN_POINTER(ptr)           ((XData *)((xpointer)(((xuintptr)(ptr)) & ~((xuintptr)X_DATALIST_FLAGS_MASK_INTERNAL))))
#define X_DATALIST_GET_POINTER(datalist)        X_DATALIST_CLEAN_POINTER(x_atomic_pointer_get(datalist))

#define X_DATALIST_SET_POINTER(datalist, pointer)                                                                   \
    X_STMT_START {                                                                                                  \
        xpointer _oldv = x_atomic_pointer_get(datalist);                                                            \
        xpointer _newv;                                                                                             \
        do {                                                                                                        \
            _oldv = x_atomic_pointer_get(datalist);                                                                 \
            _newv = (xpointer)(((xuintptr)_oldv & ((xuintptr)X_DATALIST_FLAGS_MASK_INTERNAL)) | (xuintptr)pointer); \
        } while (!x_atomic_pointer_compare_and_exchange_full((void **)datalist, _oldv, _newv, &_oldv));             \
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
static void x_dataset_destroy_internal(XDataset *dataset);
static inline XDataset *x_dataset_lookup(xconstpointer dataset_location);
static inline xpointer x_data_set_internal(XData **datalist, XQuark key_id, xpointer data, XDestroyNotify destroy_func, XDataset *dataset);

X_LOCK_DEFINE_STATIC(x_dataset_global);

static XDataset *x_dataset_cached = NULL;
static XHashTable *x_dataset_location_ht = NULL;

X_ALWAYS_INLINE static inline XData *x_datalist_lock_and_get(XData **datalist)
{
    xuintptr ptr;

    x_pointer_bit_lock_and_get((void **)datalist, DATALIST_LOCK_BIT, &ptr);
    return X_DATALIST_CLEAN_POINTER(ptr);
}

static void x_datalist_unlock(XData **datalist)
{
    x_pointer_bit_unlock((void **)datalist, DATALIST_LOCK_BIT);
}

static void x_datalist_unlock_and_set(XData **datalist, xpointer ptr)
{
    x_pointer_bit_unlock_and_set((void **)datalist, DATALIST_LOCK_BIT, ptr, X_DATALIST_FLAGS_MASK_INTERNAL);
}

static xboolean datalist_append(XData **data, XQuark key_id, xpointer new_data, XDestroyNotify destroy_func)
{
    XData *d;
    xboolean reallocated;

    d = *data;
    if (!d) {
        d = x_malloc(sizeof(XData));
        d->len = 0;
        d->alloc = 1;
        *data = d;
        reallocated = TRUE;
    } else if (d->len == d->alloc) {
        d->alloc = d->alloc * 2u;
        d = x_realloc(d, X_STRUCT_OFFSET(XData, data) + d->alloc * sizeof(XDataElt));
        *data = d;
        reallocated = TRUE;
    } else {
        reallocated = FALSE;
    }

    d->data[d->len] = (XDataElt) {
        .key = key_id,
        .data = new_data,
        .destroy = destroy_func,
    };
    d->len++;

    return reallocated;
}

static XDataElt *datalist_find(XData *data, XQuark key_id, xuint32 *out_idx)
{
    xuint32 i;

    if (data) {
        for (i = 0; i < data->len; i++) {
            XDataElt *data_elt = &data->data[i];
            if (data_elt->key == key_id) {
                if (out_idx) {
                    *out_idx = i;
                }

                return data_elt;
            }
        }
    }

    if (out_idx) {
        *out_idx = X_MAXUINT32;
    }

    return NULL;
}

void x_datalist_clear(XData **datalist)
{
    xuint i;
    XData *data;

    x_return_if_fail(datalist != NULL);

    data = x_datalist_lock_and_get(datalist);
    x_datalist_unlock_and_set(datalist, NULL);

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
        xuint i;
        XData *data;

        data = X_DATALIST_GET_POINTER(&dataset->datalist);
        if (!data) {
            if (dataset == x_dataset_cached) {
                x_dataset_cached = NULL;
            }

            x_hash_table_remove(x_dataset_location_ht, dataset_location);
            x_slice_free(XDataset, dataset);
            break;
        }

        X_DATALIST_SET_POINTER(&dataset->datalist, NULL);

        X_UNLOCK(x_dataset_global);
        for (i = 0; i < data->len; i++) {
            if (data->data[i].data && data->data[i].destroy) {
                data->data[i].destroy (data->data[i].data);
            }
        }
        x_free(data);
        X_LOCK(x_dataset_global);

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
    XData *d;
    xuint32 idx;
    XData *new_d = NULL;
    XDataElt old, *data;

    d = x_datalist_lock_and_get(datalist);
    data = datalist_find(d, key_id, &idx);

    if (new_data == NULL) {
        if (data) {
            old = *data;
            if (idx != (d->len - 1u)) {
                *data = d->data[d->len - 1u];
            }
            d->len--;

            if (d->len == 0) {
                x_datalist_unlock_and_set(datalist, NULL);
                x_free (d);

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

                old.destroy(old.data);
                if (dataset) {
                    X_LOCK(x_dataset_global);
                }
                old.data = NULL;
            }

            return old.data;
        }
    } else {
        if (data) {
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

        if (datalist_append(&d, key_id, new_data, new_destroy_func)) {
            new_d = d;
        }
    }

    if (new_d) {
        x_datalist_unlock_and_set(datalist, new_d);
    } else {
        x_datalist_unlock(datalist);
    }

    return NULL;
}

static inline void x_data_remove_internal(XData **datalist, XQuark *keys, xsize n_keys)
{
    XData *d;
    XDataElt *old;
    XDataElt *data;
    xsize found_keys;
    XDataElt *data_end;
    xboolean free_d = FALSE;
    XDataElt *old_to_free = NULL;

    d = x_datalist_lock_and_get(datalist);
    if (!d) {
        x_datalist_unlock(datalist);
        return;
    }

    if (n_keys <= (400u / sizeof(XDataElt))) {
        old = x_newa0(XDataElt, n_keys);
    } else {
        old_to_free = x_new0(XDataElt, n_keys);
        old = old_to_free;
    }

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
                free_d = TRUE;
                break;
            }
        } else {
            data++;
        }
    }

    if (free_d) {
        x_datalist_unlock_and_set(datalist, NULL);
        x_free(d);
    } else {
        x_datalist_unlock(datalist);
    }

    if (found_keys > 0) {
        for (xsize i = 0; i < n_keys; i++) {
            if (old[i].destroy) {
                old[i].destroy(old[i].data);
            }
        }
    }

    if (X_UNLIKELY(old_to_free)) {
        x_free(old_to_free);
    }
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
    XDataElt *data;
    xpointer val = NULL;
    xpointer retval = NULL;

    d = x_datalist_lock_and_get(datalist);
    data = datalist_find(d, key_id, NULL);
    if (data) {
        val = data->data;
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
    xuint32 idx;
    XDataElt *data;
    xpointer val = NULL;
    XData *new_d = NULL;
    xboolean free_d = FALSE;
    xboolean set_new_d = FALSE;

    x_return_val_if_fail(datalist != NULL, FALSE);
    x_return_val_if_fail(key_id != 0, FALSE);

    if (old_destroy) {
        *old_destroy = NULL;
    }

    d = x_datalist_lock_and_get(datalist);
    data = datalist_find(d, key_id, &idx);
    if (data) {
        val = data->data;
        if (val == oldval) {
            if (old_destroy) {
                *old_destroy = data->destroy;
            }

            if (newval != NULL) {
                data->data = newval;
                data->destroy = destroy;
            } else {
                if (idx != (d->len - 1u)) {
                    *data = d->data[d->len - 1u];
                }
                d->len--;

                if (d->len == 0) {
                    set_new_d = TRUE;
                    free_d = TRUE;
                }
            }
        }
    }

    if ((val == NULL) && (oldval == NULL) && (newval != NULL)) {
        if (datalist_append(&d, key_id, newval, destroy)) {
            new_d = d;
            set_new_d = TRUE;
        }
    }

    if (set_new_d) {
        x_datalist_unlock_and_set(datalist, new_d);
    } else {
        x_datalist_unlock(datalist);
    }

    if (free_d) {
        x_free(d);
    }

    return val == oldval;
}

xpointer x_datalist_get_data(XData **datalist, const xchar *key)
{
    XData *d;
    xpointer res = NULL;
    XDataElt *data, *data_end;

    x_return_val_if_fail(datalist != NULL, NULL);

    d = x_datalist_lock_and_get(datalist);
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
