#include <string.h>
#include <stdlib.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xvaluearray.h>

#define GROUP_N_VALUES          (8)

XValue *x_value_array_get_nth(XValueArray *value_array, xuint index)
{
    x_return_val_if_fail(value_array != NULL, NULL);
    x_return_val_if_fail(index < value_array->n_values, NULL);

    return value_array->values + index;
}

static inline void value_array_grow(XValueArray *value_array, xuint n_values, xboolean zero_init)
{
    x_return_if_fail(n_values >= value_array->n_values);

    value_array->n_values = n_values;
    if (value_array->n_values > value_array->n_prealloced) {
        xuint i = value_array->n_prealloced;

        value_array->n_prealloced = (value_array->n_values + GROUP_N_VALUES - 1) & ~(GROUP_N_VALUES - 1);
        value_array->values = x_renew(XValue, value_array->values, value_array->n_prealloced);
        if (!zero_init) {
            i = value_array->n_values;
        }

        memset(value_array->values + i, 0, (value_array->n_prealloced - i) * sizeof (value_array->values[0]));
    }
}

XValueArray *x_value_array_new(xuint n_prealloced)
{
    XValueArray *value_array = x_slice_new(XValueArray);

    value_array->n_values = 0;
    value_array->n_prealloced = 0;
    value_array->values = NULL;
    value_array_grow(value_array, n_prealloced, TRUE);
    value_array->n_values = 0;

    return value_array;
}

void x_value_array_free(XValueArray *value_array)
{
    xuint i;

    x_return_if_fail(value_array != NULL);

    for (i = 0; i < value_array->n_values; i++) {
        XValue *value = value_array->values + i;

        if (X_VALUE_TYPE(value) != 0) {
            x_value_unset(value);
        }
    }

    x_free(value_array->values);
    x_slice_free(XValueArray, value_array);
}

XValueArray *x_value_array_copy(const XValueArray *value_array)
{
    xuint i;
    XValueArray *new_array;

    x_return_val_if_fail(value_array != NULL, NULL);

    new_array = x_slice_new(XValueArray);
    new_array->n_values = 0;
    new_array->values = NULL;
    new_array->n_prealloced = 0;
    value_array_grow (new_array, value_array->n_values, TRUE);

    for (i = 0; i < new_array->n_values; i++) {
        if (X_VALUE_TYPE(value_array->values + i) != 0) {
            XValue *value = new_array->values + i;

            x_value_init(value, X_VALUE_TYPE(value_array->values + i));
            x_value_copy(value_array->values + i, value);
        }
    }

    return new_array;
}

XValueArray *x_value_array_prepend(XValueArray *value_array, const XValue *value)
{
    x_return_val_if_fail(value_array != NULL, NULL);

    X_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return x_value_array_insert(value_array, 0, value);
    X_GNUC_END_IGNORE_DEPRECATIONS
}

XValueArray *x_value_array_append(XValueArray *value_array, const XValue *value)
{
    x_return_val_if_fail(value_array != NULL, NULL);

    X_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return x_value_array_insert(value_array, value_array->n_values, value);
    X_GNUC_END_IGNORE_DEPRECATIONS
}

XValueArray *x_value_array_insert(XValueArray *value_array, xuint index, const XValue *value)
{
    xuint i;

    x_return_val_if_fail(value_array != NULL, NULL);
    x_return_val_if_fail(index <= value_array->n_values, value_array);

    i = value_array->n_values;
    value_array_grow(value_array, value_array->n_values + 1, FALSE);
    if (index + 1 < value_array->n_values) {
        memmove(value_array->values + index + 1, value_array->values + index, (i - index) * sizeof (value_array->values[0]));
    }

    memset(value_array->values + index, 0, sizeof (value_array->values[0]));
    if (value) {
        x_value_init(value_array->values + index, X_VALUE_TYPE(value));
        x_value_copy(value, value_array->values + index);
    }

    return value_array;
}

XValueArray *x_value_array_remove(XValueArray *value_array, xuint index)
{
    x_return_val_if_fail(value_array != NULL, NULL);
    x_return_val_if_fail(index < value_array->n_values, value_array);

    if (X_VALUE_TYPE(value_array->values + index) != 0) {
        x_value_unset(value_array->values + index);
    }

    value_array->n_values--;
    if (index < value_array->n_values) {
        memmove(value_array->values + index, value_array->values + index + 1, (value_array->n_values - index) * sizeof (value_array->values[0]));
    }

    if (value_array->n_prealloced > value_array->n_values) {
        memset(value_array->values + value_array->n_values, 0, sizeof (value_array->values[0]));
    }

    return value_array;
}

XValueArray *x_value_array_sort(XValueArray *value_array, XCompareFunc compare_func)
{
    x_return_val_if_fail(compare_func != NULL, NULL);

    if (value_array->n_values) {
        qsort(value_array->values, value_array->n_values, sizeof(value_array->values[0]), compare_func);
    }

    return value_array;
}

XValueArray *x_value_array_sort_with_data(XValueArray *value_array, XCompareDataFunc compare_func, xpointer user_data)
{
    x_return_val_if_fail(value_array != NULL, NULL);
    x_return_val_if_fail(compare_func != NULL, NULL);

    if (value_array->n_values) {
        x_sort_array(value_array->values, value_array->n_values, sizeof(value_array->values[0]), compare_func, user_data);
    }

    return value_array;
}
