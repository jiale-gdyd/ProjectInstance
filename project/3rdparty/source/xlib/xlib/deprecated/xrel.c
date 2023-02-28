#include <stdarg.h>
#include <string.h>

#include <xlib/xlib/config.h>

#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/deprecated/xrel.h>

typedef struct _XRealTuples XRealTuples;

struct _XRelation {
    xint       fields;
    xint       current_field;
    XHashTable *all_tuples;
    XHashTable **hashed_tuple_tables;
    xint       count;
};

struct _XRealTuples {
    xint     len;
    xint     width;
    xpointer *data;
};

static xboolean tuple_equal_2(xconstpointer v_a, xconstpointer v_b)
{
    xpointer *a = (xpointer *)v_a;
    xpointer *b = (xpointer *)v_b;

    return a[0] == b[0] && a[1] == b[1];
}

static xuint tuple_hash_2(xconstpointer v_a)
{
#if XLIB_SIZEOF_VOID_P > XLIB_SIZEOF_LONG
    xuint *a = (xuint *)v_a;
    return (a[0] ^ a[1] ^ a[2] ^ a[3]);
#else
    xpointer *a = (xpointer *)v_a;
    return (xulong)a[0] ^ (xulong)a[1];
#endif
}

static XHashFunc tuple_hash(xint fields)
{
    switch (fields) {
        case 2:
            return tuple_hash_2;

        default:
            x_error("no tuple hash for %d", fields);
    }

    return NULL;
}

static XEqualFunc tuple_equal(xint fields)
{
    switch (fields) {
        case 2:
            return tuple_equal_2;

        default:
            x_error("no tuple equal for %d", fields);
    }

    return NULL;
}

XRelation *x_relation_new(xint fields)
{
    XRelation *rel = x_new0(XRelation, 1);

    rel->fields = fields;
    rel->all_tuples = x_hash_table_new(tuple_hash(fields), tuple_equal(fields));
    rel->hashed_tuple_tables = x_new0(XHashTable *, fields);

    return rel;
}

static void relation_delete_value_tuple(xpointer tuple_key, xpointer tuple_value, xpointer user_data)
{
    xpointer *tuple = (xpointer *)tuple_value;
    XRelation *relation = (XRelation *)user_data;

    x_slice_free1(relation->fields * sizeof(xpointer), tuple);
}

static void x_relation_free_array(xpointer key, xpointer value, xpointer user_data)
{
    x_hash_table_destroy((XHashTable *)value);
}

void x_relation_destroy(XRelation *relation)
{
    xint i;

    if (relation) {
        for (i = 0; i < relation->fields; i += 1) {
            if (relation->hashed_tuple_tables[i]) {
                x_hash_table_foreach(relation->hashed_tuple_tables[i], x_relation_free_array, NULL);
                x_hash_table_destroy(relation->hashed_tuple_tables[i]);
            }
        }

        x_hash_table_foreach(relation->all_tuples, relation_delete_value_tuple, relation);
        x_hash_table_destroy(relation->all_tuples);

        x_free(relation->hashed_tuple_tables);
        x_free(relation);
    }
}

void x_relation_index(XRelation *relation, xint field, XHashFunc hash_func, XEqualFunc key_equal_func)
{
    x_return_if_fail(relation != NULL);
    x_return_if_fail(relation->count == 0 && relation->hashed_tuple_tables[field] == NULL);

    relation->hashed_tuple_tables[field] = x_hash_table_new(hash_func, key_equal_func);
}

void x_relation_insert(XRelation *relation, ...)
{
    xint i;
    va_list args;
    xpointer *tuple = (xpointer *)x_slice_alloc(relation->fields * sizeof(xpointer));

    va_start(args, relation);
    for (i = 0; i < relation->fields; i += 1) {
        tuple[i] = va_arg(args, xpointer);
    }
    va_end (args);

    x_hash_table_insert(relation->all_tuples, tuple, tuple);

    relation->count += 1;
    for (i = 0; i < relation->fields; i += 1) {
        xpointer key;
        XHashTable *table;
        XHashTable *per_key_table;

        table = relation->hashed_tuple_tables[i];
        if (table == NULL) {
            continue;
        }

        key = tuple[i];
        per_key_table = (XHashTable *)x_hash_table_lookup(table, key);

        if (per_key_table == NULL) {
            per_key_table = x_hash_table_new(tuple_hash(relation->fields), tuple_equal(relation->fields));
            x_hash_table_insert(table, key, per_key_table);
        }

        x_hash_table_insert(per_key_table, tuple, tuple);
    }
}

static void x_relation_delete_tuple(xpointer tuple_key, xpointer tuple_value, xpointer user_data)
{
    xint j;
    xpointer *tuple = (xpointer *)tuple_value;
    XRelation *relation = (XRelation *)user_data;

    x_assert(tuple_key == tuple_value);

    for (j = 0; j < relation->fields; j += 1) {
        xpointer one_key;
        XHashTable *per_key_table;
        XHashTable *one_table = relation->hashed_tuple_tables[j];

        if (one_table == NULL) {
            continue;
        }

        if (j == relation->current_field) {
            continue;
        }

        one_key = tuple[j];
        per_key_table = (XHashTable *)x_hash_table_lookup(one_table, one_key);

        x_hash_table_remove(per_key_table, tuple);
    }

    if (x_hash_table_remove(relation->all_tuples, tuple)) {
        x_slice_free1(relation->fields * sizeof(xpointer), tuple);
    }

    relation->count -= 1;
}

xint x_relation_delete(XRelation *relation, xconstpointer key, xint field)
{
    xint count;
    XHashTable *table;
    XHashTable *key_table;

    x_return_val_if_fail(relation != NULL, 0);

    table = relation->hashed_tuple_tables[field];
    count = relation->count;

    x_return_val_if_fail(table != NULL, 0);

    key_table = (XHashTable *)x_hash_table_lookup(table, key);
    if (!key_table) {
        return 0;
    }

    relation->current_field = field;
    x_hash_table_foreach(key_table, x_relation_delete_tuple, relation);
    x_hash_table_remove(table, key);
    x_hash_table_destroy(key_table);

    return count - relation->count;
}

static void x_relation_select_tuple(xpointer tuple_key, xpointer tuple_value, xpointer user_data)
{
    xpointer *tuple = (xpointer *)tuple_value;
    XRealTuples *tuples = (XRealTuples *)user_data;
    xint stride = sizeof(xpointer) * tuples->width;

    x_assert(tuple_key == tuple_value);

    memcpy(tuples->data + (tuples->len * tuples->width), tuple, stride);
    tuples->len += 1;
}

XTuples *x_relation_select(XRelation *relation, xconstpointer key, xint field)
{
    xint count;
    XHashTable *table;
    XRealTuples *tuples;
    XHashTable *key_table;

    x_return_val_if_fail(relation != NULL, NULL);
    table = relation->hashed_tuple_tables[field];
    x_return_val_if_fail(table != NULL, NULL);

    tuples = x_new0(XRealTuples, 1);
    key_table = (XHashTable *)x_hash_table_lookup(table, key);

    if (!key_table) {
        return (XTuples *)tuples;
    }

    count = x_relation_count(relation, key, field);
    tuples->data = (xpointer *)x_malloc(sizeof(xpointer) * relation->fields * count);
    tuples->width = relation->fields;

    x_hash_table_foreach(key_table, x_relation_select_tuple, tuples);
    x_assert(count == tuples->len);

    return (XTuples *)tuples;
}

xint x_relation_count(XRelation *relation, xconstpointer key, xint field)
{
    XHashTable *table;
    XHashTable *key_table;

    x_return_val_if_fail(relation != NULL, 0);
    table = relation->hashed_tuple_tables[field];
    x_return_val_if_fail(table != NULL, 0);

    key_table = (XHashTable *)x_hash_table_lookup(table, key);
    if (!key_table) {
        return 0;
    }

    return x_hash_table_size(key_table);
}

xboolean x_relation_exists(XRelation *relation, ...)
{
    xint i;
    va_list args;
    xboolean result;
    xpointer *tuple = (xpointer *)x_slice_alloc(relation->fields * sizeof(xpointer));

    va_start(args, relation);
    for (i = 0; i < relation->fields; i += 1) {
        tuple[i] = va_arg(args, xpointer);
    }
    va_end(args);

    result = x_hash_table_lookup(relation->all_tuples, tuple) != NULL;
    x_slice_free1(relation->fields * sizeof(xpointer), tuple);

    return result;
}

void x_tuples_destroy(XTuples *tuples0)
{
    XRealTuples *tuples = (XRealTuples *)tuples0;

    if (tuples) {
        x_free(tuples->data);
        x_free(tuples);
    }
}

xpointer x_tuples_index(XTuples *tuples0, xint index, xint field)
{
    XRealTuples *tuples = (XRealTuples *)tuples0;

    x_return_val_if_fail(tuples0 != NULL, NULL);
    x_return_val_if_fail(field < tuples->width, NULL);

    return tuples->data[index * tuples->width + field];
}

static void x_relation_print_one(xpointer tuple_key, xpointer tuple_value, xpointer user_data)
{
    xint i;
    XString *gstring;
    XRelation *rel = (XRelation *)user_data;
    xpointer *tuples = (xpointer *)tuple_value;

    gstring = x_string_new("[");

    for (i = 0; i < rel->fields; i += 1) {
        x_string_append_printf(gstring, "%p", tuples[i]);
        if (i < (rel->fields - 1)) {
            x_string_append(gstring, ",");
        }
    }

    x_string_append(gstring, "]");
    x_log(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, "%s", gstring->str);
    x_string_free(gstring, TRUE);
}

static void x_relation_print_index(xpointer tuple_key, xpointer tuple_value, xpointer user_data)
{
    XRelation *rel = (XRelation *)user_data;
    XHashTable *table = (XHashTable *)tuple_value;

    x_log(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, "*** key %p", tuple_key);
    x_hash_table_foreach(table, x_relation_print_one, rel);
}

void x_relation_print(XRelation *relation)
{
    xint i;

    x_log(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, "*** all tuples (%d)", relation->count);
    x_hash_table_foreach(relation->all_tuples, x_relation_print_one, relation);

    for (i = 0; i < relation->fields; i += 1) {
        if (relation->hashed_tuple_tables[i] == NULL) {
            continue;
        }

        x_log(X_LOG_DOMAIN, X_LOG_LEVEL_INFO, "*** index %d", i);
        x_hash_table_foreach(relation->hashed_tuple_tables[i], x_relation_print_index, relation);
    }
}
