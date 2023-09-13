#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xparam.h>
#include <xlib/xobj/xparamspecs.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

#define PARAM_FLOATING_FLAG                     0x2
#define X_PARAM_USER_MASK                       (~0U << X_PARAM_USER_SHIFT)
#define PSPEC_APPLIES_TO_VALUE(pspec, value)    (X_TYPE_CHECK_VALUE_TYPE((value), X_PARAM_SPEC_VALUE_TYPE(pspec)))

static void x_param_spec_class_base_init(XParamSpecClass *classt);
static void x_param_spec_class_base_finalize(XParamSpecClass *classt);
static void x_param_spec_class_init(XParamSpecClass *classt, xpointer class_data);
static void x_param_spec_init(XParamSpec *pspec, XParamSpecClass *classt);
static void x_param_spec_finalize(XParamSpec *pspec);
static void value_param_init(XValue *value);
static void value_param_free_value(XValue *value);
static void value_param_copy_value(const XValue *src_value, XValue *dest_value);
static void value_param_transform_value(const XValue *src_value, XValue *dest_value);
static xpointer value_param_peek_pointer(const XValue *value);
static xchar *value_param_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);
static xchar *value_param_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);

typedef struct {
    XValue default_value;
    XQuark name_quark;
} XParamSpecPrivate;

static xint x_param_private_offset;

static inline XParamSpecPrivate *x_param_spec_get_private(XParamSpec *pspec)
{
    return &X_STRUCT_MEMBER(XParamSpecPrivate, pspec, x_param_private_offset);
}

void _x_param_type_init(void)
{
    static const XTypeFundamentalInfo finfo = {
        (XTypeFundamentalFlags)(X_TYPE_FLAG_CLASSED | X_TYPE_FLAG_INSTANTIATABLE | X_TYPE_FLAG_DERIVABLE | X_TYPE_FLAG_DEEP_DERIVABLE),
    };

    static const XTypeValueTable param_value_table = {
        value_param_init,
        value_param_free_value,
        value_param_copy_value,
        value_param_peek_pointer,
        "p",
        value_param_collect_value,
        "p",
        value_param_lcopy_value,
    };

    const XTypeInfo param_spec_info = {
        sizeof(XParamSpecClass),

        (XBaseInitFunc)x_param_spec_class_base_init,
        (XBaseFinalizeFunc)x_param_spec_class_base_finalize,
        (XClassInitFunc)x_param_spec_class_init,
        (XClassFinalizeFunc) NULL,
        NULL,

        sizeof(XParamSpec),
        0,
        (XInstanceInitFunc)x_param_spec_init,

        &param_value_table,
    };
    XType type;

    type = x_type_register_fundamental(X_TYPE_PARAM, x_intern_static_string("XParam"), &param_spec_info, &finfo, X_TYPE_FLAG_ABSTRACT);
    x_assert(type == X_TYPE_PARAM);
    x_param_private_offset = x_type_add_instance_private(type, sizeof(XParamSpecPrivate));
    x_value_register_transform_func(X_TYPE_PARAM, X_TYPE_PARAM, value_param_transform_value);
}

static void x_param_spec_class_base_init(XParamSpecClass *classt)
{

}

static void x_param_spec_class_base_finalize(XParamSpecClass *classt)
{

}

static void x_param_spec_class_init(XParamSpecClass *classt, xpointer class_data)
{
    classt->value_type = X_TYPE_NONE;
    classt->finalize = x_param_spec_finalize;
    classt->value_set_default = NULL;
    classt->value_validate = NULL;
    classt->values_cmp = NULL;

    x_type_class_adjust_private_offset(classt, &x_param_private_offset);
}

static void x_param_spec_init(XParamSpec *pspec, XParamSpecClass *classt)
{
    pspec->name = NULL;
    pspec->_nick = NULL;
    pspec->_blurb = NULL;
    pspec->flags = (XParamFlags)0;
    pspec->value_type = classt->value_type;
    pspec->owner_type = 0;
    pspec->qdata = NULL;
    x_datalist_set_flags(&pspec->qdata, PARAM_FLOATING_FLAG);
    pspec->ref_count = 1;
    pspec->param_id = 0;
}

static void x_param_spec_finalize(XParamSpec *pspec)
{
    XParamSpecPrivate *priv = x_param_spec_get_private(pspec);

    if (priv->default_value.x_type) {
        x_value_reset(&priv->default_value);
    }

    x_datalist_clear(&pspec->qdata);

    if (!(pspec->flags & X_PARAM_STATIC_NICK)) {
        x_free(pspec->_nick);
    }

    if (!(pspec->flags & X_PARAM_STATIC_BLURB)) {
        x_free(pspec->_blurb);
    }

    x_type_free_instance((XTypeInstance *)pspec);
}

XParamSpec *x_param_spec_ref(XParamSpec *pspec)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);
    x_atomic_int_inc((int *)&pspec->ref_count);

    return pspec;
}

void x_param_spec_unref(XParamSpec *pspec)
{
    xboolean is_zero;

    x_return_if_fail(X_IS_PARAM_SPEC(pspec));

    is_zero = x_atomic_int_dec_and_test((int *)&pspec->ref_count);
    if (X_UNLIKELY(is_zero)) {
        X_PARAM_SPEC_GET_CLASS(pspec)->finalize (pspec);
    }
}

void x_param_spec_sink(XParamSpec *pspec)
{
    xsize oldvalue;
    x_return_if_fail(X_IS_PARAM_SPEC(pspec));

    oldvalue = x_atomic_pointer_and(&pspec->qdata, ~(xsize)PARAM_FLOATING_FLAG);
    if (oldvalue & PARAM_FLOATING_FLAG) {
        x_param_spec_unref(pspec);
    }
}

XParamSpec *x_param_spec_ref_sink(XParamSpec *pspec)
{
    xsize oldvalue;
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);

    oldvalue = x_atomic_pointer_and(&pspec->qdata, ~(xsize)PARAM_FLOATING_FLAG);
    if (!(oldvalue & PARAM_FLOATING_FLAG)) {
        x_param_spec_ref(pspec);
    }

    return pspec;
}

const xchar *x_param_spec_get_name(XParamSpec *pspec)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);
    return pspec->name;
}

const xchar *x_param_spec_get_nick(XParamSpec *pspec)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);

    if (pspec->_nick) {
        return pspec->_nick;
    } else {
        XParamSpec *redirect_target;

        redirect_target = x_param_spec_get_redirect_target(pspec);
        if (redirect_target && redirect_target->_nick) {
            return redirect_target->_nick;
        }
    }

    return pspec->name;
}

const xchar *x_param_spec_get_blurb(XParamSpec *pspec)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);

    if (pspec->_blurb) {
        return pspec->_blurb;
    } else {
        XParamSpec *redirect_target;

        redirect_target = x_param_spec_get_redirect_target(pspec);
        if (redirect_target && redirect_target->_blurb) {
            return redirect_target->_blurb;
        }
    }

    return NULL;
}

static void canonicalize_key(xchar *key)
{
    xchar *p;

    for (p = key; *p != 0; p++) {
        xchar c = *p;

        if (c == '_') {
            *p = '-';
        }
    }
}

static xboolean is_canonical(const xchar *key)
{
    return (strchr(key, '_') == NULL);
}

xboolean x_param_spec_is_valid_name(const xchar *name)
{
    const xchar *p;

    if ((name[0] < 'A' || name[0] > 'Z') && (name[0] < 'a' || name[0] > 'z')) {
        return FALSE;
    }

    for (p = name; *p != 0; p++) {
        const xchar c = *p;
        if (c != '-' && c != '_' && (c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
            return FALSE;
        }
    }

    return TRUE;
}

xpointer x_param_spec_internal(XType param_type, const xchar *name, const xchar *nick, const xchar *blurb, XParamFlags flags)
{
    XParamSpec *pspec;
    XParamSpecPrivate *priv;

    x_return_val_if_fail(X_TYPE_IS_PARAM(param_type) && param_type != X_TYPE_PARAM, NULL);
    x_return_val_if_fail(name != NULL, NULL);
    x_return_val_if_fail(x_param_spec_is_valid_name(name), NULL);
    x_return_val_if_fail(!(flags & X_PARAM_STATIC_NAME) || is_canonical(name), NULL);
    
    pspec = (XParamSpec *)x_type_create_instance(param_type);

    if (flags & X_PARAM_STATIC_NAME) {
        pspec->name = (xchar *)x_intern_static_string(name);
        if (!is_canonical(pspec->name)) {
            x_warning("X_PARAM_STATIC_NAME used with non-canonical pspec name: %s", pspec->name);
        }
    } else {
        if (is_canonical(name)) {
            pspec->name = (xchar *)x_intern_string(name);
        } else {
            xchar *tmp = x_strdup(name);
            canonicalize_key(tmp);
            pspec->name = (xchar *)x_intern_string(tmp);
            x_free(tmp);
        }
    }

    priv = x_param_spec_get_private(pspec);
    priv->name_quark = x_quark_from_string(pspec->name);

    if (flags & X_PARAM_STATIC_NICK) {
        pspec->_nick = (xchar *)nick;
    } else {
        pspec->_nick = x_strdup(nick);
    }

    if (flags & X_PARAM_STATIC_BLURB) {
        pspec->_blurb = (xchar *)blurb;
    } else {
        pspec->_blurb = x_strdup(blurb);
    }

    pspec->flags = (XParamFlags)((flags & X_PARAM_USER_MASK) | (flags & X_PARAM_MASK));
    return pspec;
}

xpointer x_param_spec_get_qdata(XParamSpec *pspec, XQuark quark)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);
    return quark ? x_datalist_id_get_data(&pspec->qdata, quark) : NULL;
}

void x_param_spec_set_qdata(XParamSpec *pspec, XQuark quark, xpointer data)
{
    x_return_if_fail(X_IS_PARAM_SPEC(pspec));
    x_return_if_fail(quark > 0);

    x_datalist_id_set_data(&pspec->qdata, quark, data);
}

void x_param_spec_set_qdata_full(XParamSpec *pspec, XQuark quark, xpointer data, XDestroyNotify destroy)
{
    x_return_if_fail(X_IS_PARAM_SPEC(pspec));
    x_return_if_fail(quark > 0);

    x_datalist_id_set_data_full(&pspec->qdata, quark, data, data ? destroy : (XDestroyNotify) NULL);
}

xpointer x_param_spec_steal_qdata(XParamSpec *pspec, XQuark quark)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), NULL);
    x_return_val_if_fail(quark > 0, NULL);

    return x_datalist_id_remove_no_notify(&pspec->qdata, quark);
}

XParamSpec *x_param_spec_get_redirect_target(XParamSpec *pspec)
{
    XTypeInstance *inst = (XTypeInstance *)pspec;

    if (inst && inst->x_class && inst->x_class->x_type == X_TYPE_PARAM_OVERRIDE) {
        return ((XParamSpecOverride *)pspec)->overridden;
    } else {
        return NULL;
    }
}

void x_param_value_set_default(XParamSpec *pspec, XValue *value)
{
    x_return_if_fail(X_IS_PARAM_SPEC(pspec));

    if (X_VALUE_TYPE(value) == X_TYPE_INVALID) {
        x_value_init(value, X_PARAM_SPEC_VALUE_TYPE(pspec));
    } else {
        x_return_if_fail(X_IS_VALUE(value));
        x_return_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, value));
        x_value_reset(value);
    }

    X_PARAM_SPEC_GET_CLASS(pspec)->value_set_default (pspec, value);
}

xboolean x_param_value_defaults(XParamSpec *pspec, const XValue *value)
{
    xboolean defaults;
    XValue dflt_value = X_VALUE_INIT;

    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), FALSE);
    x_return_val_if_fail(X_IS_VALUE(value), FALSE);
    x_return_val_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, value), FALSE);

    x_value_init(&dflt_value, X_PARAM_SPEC_VALUE_TYPE(pspec));
    X_PARAM_SPEC_GET_CLASS(pspec)->value_set_default(pspec, &dflt_value);
    defaults = X_PARAM_SPEC_GET_CLASS(pspec)->values_cmp(pspec, value, &dflt_value) == 0;
    x_value_unset(&dflt_value);

    return defaults;
}

xboolean x_param_value_validate(XParamSpec *pspec, XValue *value)
{
    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), FALSE);
    x_return_val_if_fail(X_IS_VALUE(value), FALSE);
    x_return_val_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, value), FALSE);

    if (X_PARAM_SPEC_GET_CLASS(pspec)->value_validate) {
        XValue oval = *value;

        if (X_PARAM_SPEC_GET_CLASS(pspec)->value_validate (pspec, value) || memcmp(&oval.data, &value->data, sizeof(oval.data))) {
            return TRUE;
        }
    }

    return FALSE;
}

xboolean x_param_value_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecClass *classt;

    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), TRUE);
    x_return_val_if_fail(X_IS_VALUE(value), TRUE);
    x_return_val_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, value), TRUE);

    classt = X_PARAM_SPEC_GET_CLASS(pspec);

    if (classt->value_is_valid) {
        return classt->value_is_valid(pspec, value);
    } else if (classt->value_validate) {
        xboolean changed;
        XValue val = X_VALUE_INIT;

        x_value_init(&val, X_VALUE_TYPE(value));
        x_value_copy(value, &val);

        changed = classt->value_validate(pspec, &val);
        x_value_unset(&val);

        return !changed;
    }

    return TRUE;
}

xboolean x_param_value_convert (XParamSpec *pspec, const XValue *src_value, XValue *dest_value, xboolean strict_validation)
{
    XValue tmp_value = X_VALUE_INIT;

    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), FALSE);
    x_return_val_if_fail(X_IS_VALUE(src_value), FALSE);
    x_return_val_if_fail(X_IS_VALUE(dest_value), FALSE);
    x_return_val_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, dest_value), FALSE);

    x_value_init(&tmp_value, X_VALUE_TYPE(dest_value));
    if (x_value_transform(src_value, &tmp_value) && (!x_param_value_validate(pspec, &tmp_value) || !strict_validation)) {
        x_value_unset(dest_value);
        memcpy(dest_value, &tmp_value, sizeof(tmp_value));

        return TRUE;
    } else {
        x_value_unset(&tmp_value);
        return FALSE;
    }
}

xint x_param_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    xint cmp;

    x_return_val_if_fail(X_IS_PARAM_SPEC(pspec), 0);
    x_return_val_if_fail(X_IS_VALUE(value1), 0);
    x_return_val_if_fail(X_IS_VALUE(value2), 0);
    x_return_val_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, value1), 0);
    x_return_val_if_fail(PSPEC_APPLIES_TO_VALUE(pspec, value2), 0);

    cmp = X_PARAM_SPEC_GET_CLASS(pspec)->values_cmp(pspec, value1, value2);
    return CLAMP(cmp, -1, 1);
}

static void value_param_init(XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static void value_param_free_value(XValue *value)
{
    if (value->data[0].v_pointer) {
        x_param_spec_unref((XParamSpec *)value->data[0].v_pointer);
    }
}

static void value_param_copy_value(const XValue *src_value, XValue *dest_value)
{
    if (src_value->data[0].v_pointer) {
        dest_value->data[0].v_pointer = x_param_spec_ref((XParamSpec *)src_value->data[0].v_pointer);
    } else {
        dest_value->data[0].v_pointer = NULL;
    }
}

static void value_param_transform_value(const XValue *src_value, XValue *dest_value)
{
    if (src_value->data[0].v_pointer && x_type_is_a(X_PARAM_SPEC_TYPE(dest_value->data[0].v_pointer), X_VALUE_TYPE(dest_value))) {
        dest_value->data[0].v_pointer = x_param_spec_ref((XParamSpec *)src_value->data[0].v_pointer);
    } else {
        dest_value->data[0].v_pointer = NULL;
    }
}

static xpointer value_param_peek_pointer(const XValue *value)
{
    return value->data[0].v_pointer;
}

static xchar *value_param_collect_value(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    if (collect_values[0].v_pointer) {
        XParamSpec *param = (XParamSpec *)collect_values[0].v_pointer;

        if (param->x_type_instance.x_class == NULL) {
            return x_strconcat("invalid unclassed param spec pointer for value type '", X_VALUE_TYPE_NAME(value), "'", NULL);
        } else if (!x_value_type_compatible(X_PARAM_SPEC_TYPE(param), X_VALUE_TYPE(value))) {
            return x_strconcat("invalid param spec type '", X_PARAM_SPEC_TYPE_NAME(param), "' for value type '", X_VALUE_TYPE_NAME(value), "'", NULL);
        }

        value->data[0].v_pointer = x_param_spec_ref(param);
    } else {
        value->data[0].v_pointer = NULL;
    }

    return NULL;
}

static xchar *value_param_lcopy_value(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags)
{
    XParamSpec **param_p = (XParamSpec **)collect_values[0].v_pointer;

    x_return_val_if_fail(param_p != NULL, x_strdup_printf("value location for '%s' passed as NULL", X_VALUE_TYPE_NAME(value)));

    if (!value->data[0].v_pointer) {
        *param_p = NULL;
    } else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        *param_p = (XParamSpec *)value->data[0].v_pointer;
    } else {
        *param_p = x_param_spec_ref((XParamSpec *)value->data[0].v_pointer);
    }

    return NULL;
}

struct _XParamSpecPool {
    XMutex     mutex;
    xboolean   type_prefixing;
    XHashTable *hash_table;
};

static xuint param_spec_pool_hash(xconstpointer key_spec)
{
    const xchar *p;
    const XParamSpec *key = (const XParamSpec *)key_spec;
    xuint h = (xuint)key->owner_type;

    for (p = key->name; *p; p++) {
        h = (h << 5) - h + *p;
    }

    return h;
}

static xboolean param_spec_pool_equals(xconstpointer key_spec_1, xconstpointer key_spec_2)
{
    const XParamSpec *key1 = (const XParamSpec *)key_spec_1;
    const XParamSpec *key2 = (const XParamSpec *)key_spec_2;

    return (key1->owner_type == key2->owner_type && (key1->name == key2->name || strcmp(key1->name, key2->name) == 0));
}

XParamSpecPool *x_param_spec_pool_new(xboolean type_prefixing)
{
    static XMutex init_mutex;
    XParamSpecPool *pool = x_new(XParamSpecPool, 1);

    memcpy(&pool->mutex, &init_mutex, sizeof(init_mutex));
    pool->type_prefixing = type_prefixing != FALSE;
    pool->hash_table = x_hash_table_new(param_spec_pool_hash, param_spec_pool_equals);

    return pool;
}

void x_param_spec_pool_insert(XParamSpecPool *pool, XParamSpec *pspec, XType owner_type)
{
    const xchar *p;

    if (pool && pspec && owner_type > 0 && pspec->owner_type == 0) {
        for (p = pspec->name; *p; p++) {
            if (!strchr(X_CSET_A_2_Z X_CSET_a_2_z X_CSET_DIGITS "-_", *p)) {
                x_critical(X_STRLOC ": pspec name \"%s\" contains invalid characters", pspec->name);
                return;
            }
        }

        x_mutex_lock(&pool->mutex);
        pspec->owner_type = owner_type;
        x_param_spec_ref(pspec);
        x_hash_table_add(pool->hash_table, pspec);
        x_mutex_unlock(&pool->mutex);
    } else {
        x_return_if_fail(pool != NULL);
        x_return_if_fail(pspec);
        x_return_if_fail(owner_type > 0);
        x_return_if_fail(pspec->owner_type == 0);
    }
}

void x_param_spec_pool_remove(XParamSpecPool *pool, XParamSpec *pspec)
{
    if (pool && pspec) {
        x_mutex_lock(&pool->mutex);
        if (x_hash_table_remove(pool->hash_table, pspec)) {
            x_param_spec_unref(pspec);
        } else {
            x_critical(X_STRLOC ": attempt to remove unknown pspec '%s' from pool", pspec->name);
        }
        x_mutex_unlock(&pool->mutex);
    } else {
        x_return_if_fail(pool != NULL);
        x_return_if_fail(pspec);
    }
}

static inline XParamSpec *param_spec_ht_lookup(XHashTable *hash_table, const xchar *param_name, XType owner_type, xboolean walk_ancestors)
{
    XParamSpec key, *pspec;

    key.owner_type = owner_type;
    key.name = (xchar *)param_name;

    if (walk_ancestors) {
        do {
            pspec = (XParamSpec *)x_hash_table_lookup(hash_table, &key);
            if (pspec) {
                return pspec;
            }

            key.owner_type = x_type_parent(key.owner_type);
        } while (key.owner_type);
    } else {
        pspec = (XParamSpec *)x_hash_table_lookup(hash_table, &key);
    }

    if (!pspec && !is_canonical(param_name)) {
        xchar *canonical;

        canonical = x_strdup(key.name);
        canonicalize_key(canonical);

        key.name = canonical;
        key.owner_type = owner_type;

        if (walk_ancestors) {
            do {
                pspec = (XParamSpec *)x_hash_table_lookup(hash_table, &key);
                if (pspec) {
                    x_free(canonical);
                    return pspec;
                }

                key.owner_type = x_type_parent(key.owner_type);
            } while (key.owner_type);
        } else {
            pspec = (XParamSpec *)x_hash_table_lookup(hash_table, &key);
        }

        x_free(canonical);
    }

    return pspec;
}

XParamSpec *x_param_spec_pool_lookup(XParamSpecPool *pool, const xchar *param_name, XType owner_type, xboolean walk_ancestors)
{
    XParamSpec *pspec;

    x_return_val_if_fail(pool != NULL, NULL);
    x_return_val_if_fail(param_name != NULL, NULL);

    x_mutex_lock(&pool->mutex);

    pspec = param_spec_ht_lookup(pool->hash_table, param_name, owner_type, walk_ancestors);
    if (pspec) {
        x_mutex_unlock(&pool->mutex);
        return pspec;
    }

    if (pool->type_prefixing) {
        char *delim;

        delim = (char *)strchr((const char *)param_name, ':');
        if (delim && delim[1] == ':') {
            XType type;
            xuint l = delim - param_name;
            xchar stack_buffer[32], *buffer = l < 32 ? stack_buffer : x_new(xchar, l + 1);

            strncpy(buffer, param_name, delim - param_name);
            buffer[l] = 0;
            type = x_type_from_name(buffer);
            if (l >= 32) {
                x_free(buffer);
            }

            if (type) {
                if ((!walk_ancestors && type != owner_type) || !x_type_is_a(owner_type, type)) {
                    x_mutex_unlock(&pool->mutex);
                    return NULL;
                }

                owner_type = type;
                param_name += l + 2;
                pspec = param_spec_ht_lookup(pool->hash_table, param_name, owner_type, walk_ancestors);
                x_mutex_unlock(&pool->mutex);

                return pspec;
            }
        }
    }

    x_mutex_unlock(&pool->mutex);

    return NULL;
}

static void pool_list(xpointer key, xpointer value, xpointer user_data)
{
    XParamSpec *pspec = (XParamSpec *)value;
    xpointer *data = (xpointer *)user_data;
    XType owner_type = (XType)data[1];

    if (owner_type == pspec->owner_type) {
        data[0] = x_list_prepend((XList *)data[0], pspec);
    }
}

XList *x_param_spec_pool_list_owned(XParamSpecPool *pool, XType owner_type)
{
    xpointer data[2];

    x_return_val_if_fail(pool != NULL, NULL);
    x_return_val_if_fail(owner_type > 0, NULL);

    x_mutex_lock(&pool->mutex);
    data[0] = NULL;
    data[1] = (xpointer)owner_type;
    x_hash_table_foreach(pool->hash_table, pool_list, &data);
    x_mutex_unlock(&pool->mutex);

    return (XList *)data[0];
}

static xint pspec_compare_id(xconstpointer a, xconstpointer b)
{
    const XParamSpec *pspec1 = (const XParamSpec *)a, *pspec2 = (const XParamSpec *)b;

    if (pspec1->param_id < pspec2->param_id) {
        return -1;
    }

    if (pspec1->param_id > pspec2->param_id) {
        return 1;
    }

    return strcmp(pspec1->name, pspec2->name);
}

static inline xboolean should_list_pspec(XParamSpec *pspec, XType owner_type, XHashTable *ht)
{
    XParamSpec *found;

    if (x_param_spec_get_redirect_target(pspec)) {
        return FALSE;
    }

    found = param_spec_ht_lookup(ht, pspec->name, owner_type, TRUE);
    if (found != pspec) {
        XParamSpec *redirect = x_param_spec_get_redirect_target(found);
        if (redirect != pspec) {
            return FALSE;
        }
    }

    return TRUE;
}

static void pool_depth_list(xpointer key, xpointer value, xpointer user_data)
{
    XParamSpec *pspec = (XParamSpec *)value;
    xpointer *data = (xpointer *)user_data;
    XSList **slists = (XSList **)data[0];
    XType owner_type = (XType)data[1];
    XHashTable *ht = (XHashTable *)data[2];
    int *count = (int *)data[3];

    if (x_type_is_a(owner_type, pspec->owner_type) && should_list_pspec(pspec, owner_type, ht)) {
        if (X_TYPE_IS_INTERFACE(pspec->owner_type)) {
            slists[0] = x_slist_prepend(slists[0], pspec);
            *count = *count + 1;
        } else {
            xuint d = x_type_depth(pspec->owner_type);
            slists[d - 1] = x_slist_prepend(slists[d - 1], pspec);
            *count = *count + 1;
        }
    }
}

static void pool_depth_list_for_interface(xpointer key, xpointer value, xpointer user_data)
{
    XParamSpec *pspec = (XParamSpec *)value;
    xpointer *data = (xpointer *)user_data;
    XSList **slists = (XSList **)data[0];
    XType owner_type = (XType)data[1];
    XHashTable *ht = (XHashTable *)data[2];
    int *count = (int *)data[3];

    if (pspec->owner_type == owner_type && should_list_pspec(pspec, owner_type, ht)) {
        slists[0] = x_slist_prepend(slists[0], pspec);
        *count = *count + 1;
    }
}

XParamSpec **x_param_spec_pool_list(XParamSpecPool *pool, XType owner_type, xuint *n_pspecs_p)
{
    xuint d, i;
    int n_pspecs = 0;
    xpointer data[4];
    XSList **slists, *node;
    XParamSpec **pspecs, **p;

    x_return_val_if_fail(pool != NULL, NULL);
    x_return_val_if_fail(owner_type > 0, NULL);
    x_return_val_if_fail(n_pspecs_p != NULL, NULL);

    x_mutex_lock(&pool->mutex);
    d = x_type_depth(owner_type);
    slists = x_new0(XSList *, d);
    data[0] = slists;
    data[1] = (xpointer) owner_type;
    data[2] = pool->hash_table;
    data[3] = &n_pspecs;

    x_hash_table_foreach(pool->hash_table, X_TYPE_IS_INTERFACE(owner_type) ? pool_depth_list_for_interface : pool_depth_list, &data);

    pspecs = x_new(XParamSpec *, n_pspecs + 1);
    p = pspecs;

    for (i = 0; i < d; i++) {
        slists[i] = x_slist_sort(slists[i], pspec_compare_id);
        for (node = slists[i]; node; node = node->next) {
            *p++ = (XParamSpec *)node->data;
        }
        x_slist_free(slists[i]);
    }

    *p++ = NULL;
    x_free(slists);
    x_mutex_unlock(&pool->mutex);

    *n_pspecs_p = n_pspecs;

    return pspecs;
}

typedef struct {
    XType value_type;
    void (*finalize)(XParamSpec *pspec);
    void (*value_set_default)(XParamSpec *pspec, XValue *value);
    xboolean (*value_validate)(XParamSpec *pspec, XValue *value);
    xint (*values_cmp)(XParamSpec *pspec, const XValue *value1, const XValue *value2);
} ParamSpecClassInfo;

static void param_spec_generic_class_init(xpointer x_class, xpointer class_data)
{
    XParamSpecClass *classt = (XParamSpecClass *)x_class;
    ParamSpecClassInfo *info = (ParamSpecClassInfo *)class_data;

    classt->value_type = info->value_type;
    if (info->finalize) {
        classt->finalize = info->finalize;
    }

    classt->value_set_default = info->value_set_default;
    if (info->value_validate) {
        classt->value_validate = info->value_validate;
    }

    classt->values_cmp = info->values_cmp;
    x_free(class_data);
}

static void default_value_set_default(XParamSpec *pspec, XValue *value)
{

}

static xint default_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    return memcmp(&value1->data, &value2->data, sizeof(value1->data));
}

XType x_param_type_register_static(const xchar *name, const XParamSpecTypeInfo *pspec_info)
{
    XTypeInfo info = {
        sizeof(XParamSpecClass),
        NULL,
        NULL,
        param_spec_generic_class_init,
        NULL,
        NULL,
        0,
        16,
        NULL,
        NULL,
    };
    ParamSpecClassInfo *cinfo;

    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(pspec_info != NULL, 0);
    x_return_val_if_fail(x_type_from_name(name) == 0, 0);
    x_return_val_if_fail(pspec_info->instance_size >= sizeof(XParamSpec), 0);
    x_return_val_if_fail(x_type_name(pspec_info->value_type) != NULL, 0);

    info.instance_size = pspec_info->instance_size;
    info.n_preallocs = pspec_info->n_preallocs;
    info.instance_init = (XInstanceInitFunc) pspec_info->instance_init;
    cinfo = x_new(ParamSpecClassInfo, 1);
    cinfo->value_type = pspec_info->value_type;
    cinfo->finalize = pspec_info->finalize;
    cinfo->value_set_default = pspec_info->value_set_default ? pspec_info->value_set_default : default_value_set_default;
    cinfo->value_validate = pspec_info->value_validate;
    cinfo->values_cmp = pspec_info->values_cmp ? pspec_info->values_cmp : default_values_cmp;
    info.class_data = cinfo;

    return x_type_register_static(X_TYPE_PARAM, name, &info, (XTypeFlags)0);
}

void x_value_set_param(XValue *value, XParamSpec *param)
{
    x_return_if_fail(X_VALUE_HOLDS_PARAM(value));
    if (param) {
        x_return_if_fail(X_IS_PARAM_SPEC(param));
    }

    if (value->data[0].v_pointer) {
        x_param_spec_unref((XParamSpec *)value->data[0].v_pointer);
    }

    value->data[0].v_pointer = param;
    if (value->data[0].v_pointer) {
        x_param_spec_ref((XParamSpec *)value->data[0].v_pointer);
    }
}

void x_value_set_param_take_ownership(XValue *value, XParamSpec *param)
{
    x_value_take_param(value, param);
}

void x_value_take_param(XValue *value, XParamSpec *param)
{
    x_return_if_fail(X_VALUE_HOLDS_PARAM(value));
    if (param) {
        x_return_if_fail(X_IS_PARAM_SPEC(param));
    }

    if (value->data[0].v_pointer) {
        x_param_spec_unref((XParamSpec *)value->data[0].v_pointer);
    }

    value->data[0].v_pointer = param;
}

XParamSpec *x_value_get_param(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_PARAM(value), NULL);
    return (XParamSpec *)value->data[0].v_pointer;
}

XParamSpec *x_value_dup_param(const XValue *value)
{
    x_return_val_if_fail(X_VALUE_HOLDS_PARAM(value), NULL);
    return value->data[0].v_pointer ? x_param_spec_ref((XParamSpec *)value->data[0].v_pointer) : NULL;
}

const XValue *x_param_spec_get_default_value(XParamSpec *pspec)
{
    XParamSpecPrivate *priv = x_param_spec_get_private(pspec);

    if (x_once_init_enter(&priv->default_value.x_type)) {
        XValue default_value = X_VALUE_INIT;

        x_value_init(&default_value, pspec->value_type);
        x_param_value_set_default(pspec, &default_value);

        memcpy(priv->default_value.data, default_value.data, sizeof(default_value.data));

        x_once_init_leave(&priv->default_value.x_type, pspec->value_type);
    }

    return &priv->default_value;
}

XQuark x_param_spec_get_name_quark(XParamSpec *pspec)
{
    XParamSpecPrivate *priv = x_param_spec_get_private(pspec);
    return priv->name_quark;
}
