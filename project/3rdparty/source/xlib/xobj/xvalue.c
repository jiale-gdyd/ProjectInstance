#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xvalue.h>
#include <xlib/xlib/xbsearcharray.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

typedef struct {
    XType           src_type;
    XType           dest_type;
    XValueTransform func;
} TransformEntry;

static xint transform_entries_cmp(xconstpointer bsearch_node1, xconstpointer bsearch_node2);

static XBSearchArray *transform_array = NULL;
static XBSearchConfig transform_bconfig = {
    sizeof(TransformEntry),
    transform_entries_cmp,
    X_BSEARCH_ARRAY_ALIGN_POWER2,
};

void _x_value_c_init(void)
{
    transform_array = x_bsearch_array_create(&transform_bconfig);
}

static inline void value_meminit(XValue *value, XType value_type)
{
    value->x_type = value_type;
    memset(value->data, 0, sizeof(value->data));
}

XValue *x_value_init(XValue *value, XType x_type)
{
    XTypeValueTable *value_table;

    x_return_val_if_fail(value != NULL, NULL);

    value_table = x_type_value_table_peek(x_type);

    if (value_table && X_VALUE_TYPE(value) == 0) {
        value_meminit(value, x_type);
        value_table->value_init (value);
    } else if (X_VALUE_TYPE(value)) {
        x_critical("%s: cannot initialize XValue with type '%s', the value has already been initialized as '%s'", X_STRLOC, x_type_name(x_type), x_type_name(X_VALUE_TYPE(value)));
    } else {
        x_critical("%s: cannot initialize XValue with type '%s', %s", X_STRLOC, x_type_name(x_type),
            value_table ? "this type is abstract with regards to XValue use, use a more specific (derived) type" : "this type has no XTypeValueTable implementation");
    }

    return value;
}

void x_value_copy(const XValue *src_value, XValue *dest_value)
{
    x_return_if_fail(src_value);
    x_return_if_fail(dest_value);
    x_return_if_fail(x_value_type_compatible(X_VALUE_TYPE(src_value), X_VALUE_TYPE(dest_value)));

    if (src_value != dest_value) {
        XType dest_type = X_VALUE_TYPE(dest_value);
        XTypeValueTable *value_table = x_type_value_table_peek(dest_type);

        x_return_if_fail(value_table);

        if (value_table->value_free) {
            value_table->value_free (dest_value);
        }

        value_meminit(dest_value, dest_type);
        value_table->value_copy(src_value, dest_value);
    }
}

XValue *x_value_reset(XValue *value)
{
    XType x_type;
    XTypeValueTable *value_table;

    x_return_val_if_fail(value, NULL);
    x_type = X_VALUE_TYPE(value);

    value_table = x_type_value_table_peek(x_type);
    x_return_val_if_fail(value_table, NULL);

    if (value_table->value_free) {
        value_table->value_free(value);
    }

    value_meminit(value, x_type);
    value_table->value_init(value);

    return value;
}

void x_value_unset(XValue *value)
{
    XTypeValueTable *value_table;

    if (value->x_type == 0) {
        return;
    }

    x_return_if_fail(value);
    value_table = x_type_value_table_peek(X_VALUE_TYPE(value));
    x_return_if_fail(value_table);

    if (value_table->value_free) {
        value_table->value_free(value);
    }

    memset(value, 0, sizeof(*value));
}

xboolean x_value_fits_pointer(const XValue *value)
{
    XTypeValueTable *value_table;

    x_return_val_if_fail(value, FALSE);
    value_table = x_type_value_table_peek(X_VALUE_TYPE(value));
    x_return_val_if_fail(value_table, FALSE);

    return value_table->value_peek_pointer != NULL;
}

xpointer x_value_peek_pointer(const XValue *value)
{
    XTypeValueTable *value_table;

    x_return_val_if_fail(value, NULL);
    value_table = x_type_value_table_peek(X_VALUE_TYPE(value));
    x_return_val_if_fail(value_table, NULL);

    if (!value_table->value_peek_pointer) {
        x_return_val_if_fail(x_value_fits_pointer(value) == TRUE, NULL);
        return NULL;
    }

    return value_table->value_peek_pointer(value);
}

void x_value_set_instance(XValue *value, xpointer instance)
{
    XType x_type;
    xchar *error_msg;
    XTypeCValue cvalue;
    XTypeValueTable *value_table;

    x_return_if_fail(value);
    x_type = X_VALUE_TYPE(value);
    value_table = x_type_value_table_peek(x_type);
    x_return_if_fail(value_table);

    if (instance) {
        x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));
        x_return_if_fail(x_value_type_compatible(X_TYPE_FROM_INSTANCE(instance), X_VALUE_TYPE(value)));
    }

    x_return_if_fail(strcmp(value_table->collect_format, "p") == 0);
    
    memset(&cvalue, 0, sizeof(cvalue));
    cvalue.v_pointer = instance;

    if (value_table->value_free) {
        value_table->value_free(value);
    }

    value_meminit(value, x_type);
    error_msg = value_table->collect_value(value, 1, &cvalue, 0);
    if (error_msg) {
        x_critical("%s: %s", X_STRLOC, error_msg);
        x_free(error_msg);

        value_meminit(value, x_type);
        value_table->value_init (value);
    }
}

void x_value_init_from_instance(XValue *value, xpointer instance)
{
    x_return_if_fail(value != NULL && X_VALUE_TYPE(value) == 0);

    if (X_IS_OBJECT(instance)) {
        value_meminit(value, X_TYPE_FROM_INSTANCE(instance));
        value->data[0].v_pointer = x_object_ref(instance);
    } else { 
        XType x_type;
        xchar *error_msg;
        XTypeCValue cvalue;
        XTypeValueTable *value_table;

        x_return_if_fail(X_TYPE_CHECK_INSTANCE(instance));

        x_type = X_TYPE_FROM_INSTANCE(instance);
        value_table = x_type_value_table_peek(x_type);
        x_return_if_fail(strcmp(value_table->collect_format, "p") == 0);

        memset(&cvalue, 0, sizeof(cvalue));
        cvalue.v_pointer = instance;

        value_meminit(value, x_type);
        value_table->value_init(value);

        error_msg = value_table->collect_value (value, 1, &cvalue, 0);
        if (error_msg) {
            x_critical("%s: %s", X_STRLOC, error_msg);
            x_free(error_msg);

            value_meminit(value, x_type);
            value_table->value_init(value);
        }
    }
}

static XType transform_lookup_get_parent_type(XType type)
{
    if (x_type_fundamental(type) == X_TYPE_INTERFACE) {
        return x_type_interface_instantiatable_prerequisite(type);
    }

    return x_type_parent(type);
}

static XValueTransform transform_func_lookup(XType src_type, XType dest_type)
{
    TransformEntry entry;

    entry.src_type = src_type;
    do {
        entry.dest_type = dest_type;
        do {
            TransformEntry *e;

            e = (TransformEntry *)x_bsearch_array_lookup(transform_array, &transform_bconfig, &entry);
            if (e) {
                if (x_type_value_table_peek(entry.dest_type) == x_type_value_table_peek(dest_type) && x_type_value_table_peek(entry.src_type) == x_type_value_table_peek(src_type)) {
                    return e->func;
                }
            }

            entry.dest_type = transform_lookup_get_parent_type(entry.dest_type);
        } while (entry.dest_type);

        entry.src_type = transform_lookup_get_parent_type(entry.src_type);
    } while (entry.src_type);

    return NULL;
}

static xint transform_entries_cmp(xconstpointer bsearch_node1, xconstpointer bsearch_node2)
{
    const TransformEntry *e1 = (const TransformEntry *)bsearch_node1;
    const TransformEntry *e2 = (const TransformEntry *)bsearch_node2;
    xint cmp = X_BSEARCH_ARRAY_CMP(e1->src_type, e2->src_type);

    if (cmp) {
        return cmp;
    } else {
        return X_BSEARCH_ARRAY_CMP(e1->dest_type, e2->dest_type);
    }
}

void x_value_register_transform_func(XType src_type, XType dest_type, XValueTransform transform_func)
{
    TransformEntry entry;

    x_return_if_fail(transform_func != NULL);

    entry.src_type = src_type;
    entry.dest_type = dest_type;

    entry.func = transform_func;
    transform_array = x_bsearch_array_replace(transform_array, &transform_bconfig, &entry);
}

xboolean x_value_type_transformable(XType src_type, XType dest_type)
{
    x_return_val_if_fail(src_type, FALSE);
    x_return_val_if_fail(dest_type, FALSE);

    return (x_value_type_compatible(src_type, dest_type) || transform_func_lookup(src_type, dest_type) != NULL);
}

xboolean x_value_type_compatible(XType src_type, XType dest_type)
{
    x_return_val_if_fail(src_type, FALSE);
    x_return_val_if_fail(dest_type, FALSE);

    if (src_type == dest_type) {
        return TRUE;
    }

    return (x_type_is_a(src_type, dest_type) && x_type_value_table_peek(dest_type) == x_type_value_table_peek(src_type));
}

xboolean x_value_transform(const XValue *src_value, XValue *dest_value)
{
    XType dest_type;

    x_return_val_if_fail(src_value, FALSE);
    x_return_val_if_fail(dest_value, FALSE);

    dest_type = X_VALUE_TYPE(dest_value);
    if (x_value_type_compatible(X_VALUE_TYPE(src_value), dest_type)) {
        x_value_copy(src_value, dest_value);

        return TRUE;
    } else {
        XValueTransform transform = transform_func_lookup(X_VALUE_TYPE(src_value), dest_type);
        if (transform) {
            x_value_unset(dest_value);

            value_meminit(dest_value, dest_type);
            transform (src_value, dest_value);

            return TRUE;
        }
    }

    return FALSE;
}
