#ifndef __X_PARAM_H__
#define __X_PARAM_H__

#include "xvalue.h"

X_BEGIN_DECLS

#define X_TYPE_IS_PARAM(type)                   (X_TYPE_FUNDAMENTAL(type) == X_TYPE_PARAM)
#define X_PARAM_SPEC(pspec)                     (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM, XParamSpec))

#if XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_42
#define X_IS_PARAM_SPEC(pspec)                  (X_TYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE((pspec), X_TYPE_PARAM))
#else
#define X_IS_PARAM_SPEC(pspec)                  (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM))
#endif

#define X_PARAM_SPEC_CLASS(pclass)              (X_TYPE_CHECK_CLASS_CAST((pclass), X_TYPE_PARAM, XParamSpecClass))
#define X_IS_PARAM_SPEC_CLASS(pclass)           (X_TYPE_CHECK_CLASS_TYPE((pclass), X_TYPE_PARAM))
#define X_PARAM_SPEC_GET_CLASS(pspec)           (X_TYPE_INSTANCE_GET_CLASS((pspec), X_TYPE_PARAM, XParamSpecClass))

#define X_PARAM_SPEC_TYPE(pspec)                (X_TYPE_FROM_INSTANCE(pspec))
#define X_PARAM_SPEC_TYPE_NAME(pspec)           (x_type_name(X_PARAM_SPEC_TYPE(pspec)))

#define X_PARAM_SPEC_VALUE_TYPE(pspec)          (X_PARAM_SPEC(pspec)->value_type)
#define X_VALUE_HOLDS_PARAM(value)              (X_TYPE_CHECK_VALUE_TYPE((value), X_TYPE_PARAM))

typedef enum {
    X_PARAM_READABLE        = 1 << 0,
    X_PARAM_WRITABLE        = 1 << 1,
    X_PARAM_READWRITE       = (X_PARAM_READABLE | X_PARAM_WRITABLE),
    X_PARAM_CONSTRUCT       = 1 << 2,
    X_PARAM_CONSTRUCT_ONLY  = 1 << 3,
    X_PARAM_LAX_VALIDATION  = 1 << 4,
    X_PARAM_STATIC_NAME     = 1 << 5,
    X_PARAM_PRIVATE XLIB_DEPRECATED_ENUMERATOR_IN_2_26 = X_PARAM_STATIC_NAME,
    X_PARAM_STATIC_NICK     = 1 << 6,
    X_PARAM_STATIC_BLURB    = 1 << 7,
    X_PARAM_EXPLICIT_NOTIFY = 1 << 30,
    X_PARAM_DEPRECATED      = (xint)(1u << 31)
} XParamFlags;

#define X_PARAM_STATIC_STRINGS                  (X_PARAM_STATIC_NAME | X_PARAM_STATIC_NICK | X_PARAM_STATIC_BLURB)
#define X_PARAM_MASK                            (0x000000ff)
#define X_PARAM_USER_SHIFT                      (8)

typedef struct _XParamSpec XParamSpec;
typedef struct _XParamSpecPool XParamSpecPool;
typedef struct _XParamSpecClass XParamSpecClass;
typedef struct _XParameter XParameter XLIB_DEPRECATED_TYPE_IN_2_54;

struct _XParamSpec {
    XTypeInstance x_type_instance;

    const xchar   *name;
    XParamFlags   flags;
    XType         value_type;
    XType         owner_type;

    xchar         *_nick;
    xchar         *_blurb;
    XData         *qdata;
    xuint         ref_count;
    xuint         param_id;
};

struct _XParamSpecClass {
    XTypeClass x_type_class;
    XType      value_type;

    void (*finalize)(XParamSpec *pspec);
    void (*value_set_default)(XParamSpec *pspec, XValue *value);
    xboolean (*value_validate)(XParamSpec *pspec, XValue *value);
    xint (*values_cmp)(XParamSpec *pspec, const XValue *value1, const XValue *value2);
    xboolean (*value_is_valid)(XParamSpec *pspec, const XValue *value);

    xpointer   dummy[3];
};

struct _XParameter {
    const xchar *name;
    XValue      value;
} XLIB_DEPRECATED_TYPE_IN_2_54;

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_ref(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
void x_param_spec_unref(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
void x_param_spec_sink(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_ref_sink(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
xpointer x_param_spec_get_qdata(XParamSpec *pspec, XQuark quark);

XLIB_AVAILABLE_IN_ALL
void x_param_spec_set_qdata(XParamSpec *pspec, XQuark quark, xpointer data);

XLIB_AVAILABLE_IN_ALL
void x_param_spec_set_qdata_full(XParamSpec *pspec, XQuark quark, xpointer data, XDestroyNotify destroy);

XLIB_AVAILABLE_IN_ALL
xpointer x_param_spec_steal_qdata(XParamSpec *pspec, XQuark quark);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_get_redirect_target(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
void x_param_value_set_default(XParamSpec *pspec, XValue *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_param_value_defaults(XParamSpec *pspec, const XValue *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_param_value_validate(XParamSpec *pspec, XValue *value);

XLIB_AVAILABLE_IN_2_74
xboolean x_param_value_is_valid(XParamSpec *pspec, const XValue *value);

XLIB_AVAILABLE_IN_ALL
xboolean x_param_value_convert(XParamSpec *pspec, const XValue *src_value, XValue *dest_value, xboolean strict_validation);

XLIB_AVAILABLE_IN_ALL
xint x_param_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2);

XLIB_AVAILABLE_IN_ALL
const xchar *x_param_spec_get_name(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
const xchar *x_param_spec_get_nick(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
const xchar *x_param_spec_get_blurb(XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
void x_value_set_param(XValue *value, XParamSpec *param);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_value_get_param(const XValue *value);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_value_dup_param(const XValue *value);

XLIB_AVAILABLE_IN_ALL
void x_value_take_param(XValue *value, XParamSpec *param);

XLIB_DEPRECATED_FOR(x_value_take_param)
void x_value_set_param_take_ownership(XValue *value, XParamSpec *param);

XLIB_AVAILABLE_IN_2_36
const XValue *x_param_spec_get_default_value(XParamSpec *pspec);

XLIB_AVAILABLE_IN_2_46
XQuark x_param_spec_get_name_quark(XParamSpec *pspec);

typedef struct _XParamSpecTypeInfo XParamSpecTypeInfo;

struct _XParamSpecTypeInfo {
    xuint16 instance_size;
    xuint16 n_preallocs;
    void (*instance_init)(XParamSpec *pspec);

    XType   value_type;
    void (*finalize)(XParamSpec *pspec);
    void (*value_set_default)(XParamSpec *pspec, XValue *value);
    xboolean (*value_validate)(XParamSpec *pspec, XValue *value);
    xint (*values_cmp)(XParamSpec *pspec, const XValue *value1, const XValue *value2);
};

XLIB_AVAILABLE_IN_ALL
XType x_param_type_register_static(const xchar *name, const XParamSpecTypeInfo *pspec_info);

XLIB_AVAILABLE_IN_2_66
xboolean x_param_spec_is_valid_name(const xchar *name);

XType _x_param_type_register_static_constant(const xchar *name, const XParamSpecTypeInfo *pspec_info, XType opt_type);

XLIB_AVAILABLE_IN_ALL
xpointer x_param_spec_internal(XType param_type, const xchar *name, const xchar *nick, const xchar *blurb, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpecPool *x_param_spec_pool_new(xboolean type_prefixing);

XLIB_AVAILABLE_IN_ALL
void x_param_spec_pool_insert(XParamSpecPool *pool, XParamSpec *pspec, XType owner_type);

XLIB_AVAILABLE_IN_ALL
void x_param_spec_pool_remove(XParamSpecPool *pool, XParamSpec *pspec);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_pool_lookup(XParamSpecPool *pool, const xchar *param_name, XType owner_type, xboolean walk_ancestors);

XLIB_AVAILABLE_IN_ALL
XList *x_param_spec_pool_list_owned(XParamSpecPool *pool, XType owner_type);

XLIB_AVAILABLE_IN_ALL
XParamSpec **x_param_spec_pool_list(XParamSpecPool *pool, XType owner_type, xuint *n_pspecs_p);

X_END_DECLS

#endif
