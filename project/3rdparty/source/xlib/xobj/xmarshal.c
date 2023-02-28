#include <xlib/xlib/config.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xboxed.h>
#include <xlib/xobj/xobject.h>
#include <xlib/xobj/xvaluetypes.h>

#define x_marshal_value_peek_boolean(v)             (v)->data[0].v_int
#define x_marshal_value_peek_char(v)                (v)->data[0].v_int
#define x_marshal_value_peek_uchar(v)               (v)->data[0].v_uint
#define x_marshal_value_peek_int(v)                 (v)->data[0].v_int
#define x_marshal_value_peek_uint(v)                (v)->data[0].v_uint
#define x_marshal_value_peek_long(v)                (v)->data[0].v_long
#define x_marshal_value_peek_ulong(v)               (v)->data[0].v_ulong
#define x_marshal_value_peek_int64(v)               (v)->data[0].v_int64
#define x_marshal_value_peek_uint64(v)              (v)->data[0].v_uint64
#define x_marshal_value_peek_enum(v)                (v)->data[0].v_long
#define x_marshal_value_peek_flags(v)               (v)->data[0].v_ulong
#define x_marshal_value_peek_float(v)               (v)->data[0].v_float
#define x_marshal_value_peek_double(v)              (v)->data[0].v_double
#define x_marshal_value_peek_string(v)              (v)->data[0].v_pointer
#define x_marshal_value_peek_param(v)               (v)->data[0].v_pointer
#define x_marshal_value_peek_boxed(v)               (v)->data[0].v_pointer
#define x_marshal_value_peek_pointer(v)             (v)->data[0].v_pointer
#define x_marshal_value_peek_object(v)              (v)->data[0].v_pointer
#define x_marshal_value_peek_variant(v)             (v)->data[0].v_pointer

void x_cclosure_marshal_VOID__VOID(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__VOID)(xpointer data1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__VOID callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 1);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__VOID)(marshal_data ? marshal_data : cc->callback);
    callback(data1, data2);
}

void x_cclosure_marshal_VOID__VOIDv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__VOID)(xpointer instance, xpointer data);

    xpointer data1, data2;
    XMarshalFunc_VOID__VOID callback;
    XCClosure *cc = (XCClosure *)closure;

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__VOID)(marshal_data ? marshal_data : cc->callback);
    callback(data1, data2);
}

void x_cclosure_marshal_VOID__BOOLEAN(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__BOOLEAN)(xpointer data1, xboolean arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__BOOLEAN callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__BOOLEAN)(marshal_data ? marshal_data : cc->callback);
    callback (data1, x_marshal_value_peek_boolean(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__BOOLEANv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__BOOLEAN)(xpointer instance, xboolean arg_0, xpointer data);

    xboolean arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__BOOLEAN callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xboolean)va_arg(args_copy, xboolean);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__BOOLEAN)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__CHAR(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__CHAR)(xpointer data1, xchar arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__CHAR callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__CHAR)(marshal_data ? marshal_data : cc->callback);
    callback (data1, x_marshal_value_peek_char(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__CHARv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__CHAR)(xpointer instance, xchar arg_0, xpointer data);

    xchar arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__CHAR callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xchar)va_arg(args_copy, xint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__CHAR)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__UCHAR(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__UCHAR)(xpointer data1, xuchar arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__UCHAR callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__UCHAR)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_uchar(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__UCHARv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__UCHAR)(xpointer instance, xuchar arg_0, xpointer data);

    xuchar arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__UCHAR callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xuchar) va_arg(args_copy, xuint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__UCHAR)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__INT(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__INT)(xpointer data1, xint arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__INT callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__INT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_int (param_values + 1), data2);
}

void x_cclosure_marshal_VOID__INTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__INT)(xpointer instance, xint arg_0, xpointer data);

    xint arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__INT callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xint)va_arg(args_copy, xint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__INT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__UINT(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__UINT)(xpointer data1, xuint arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__UINT callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__UINT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_uint(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__UINTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__UINT)(xpointer instance, xuint arg_0, xpointer data);

    xuint arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__UINT callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xuint)va_arg(args_copy, xuint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__UINT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__LONG(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__LONG)(xpointer data1, xlong arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__LONG callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
     } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__LONG)(marshal_data ? marshal_data : cc->callback);
    callback (data1, x_marshal_value_peek_long(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__LONGv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__LONG)(xpointer instance, xlong arg_0, xpointer data);

    xlong arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__LONG callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xlong)va_arg(args_copy, xlong);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__LONG)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__ULONG(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__ULONG)(xpointer data1, xulong arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__ULONG callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__ULONG)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_ulong(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__ULONGv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__ULONG)(xpointer instance, xulong arg_0, xpointer data);

    xulong arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__ULONG callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xulong)va_arg(args_copy, xulong);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__ULONG)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__ENUM(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__ENUM)(xpointer data1, xint arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__ENUM callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__ENUM)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_enum(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__ENUMv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__ENUM)(xpointer instance, xint arg_0, xpointer data);

    xint arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__ENUM callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xint)va_arg(args_copy, xint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__ENUM)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__FLAGS(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__FLAGS)(xpointer data1, xuint arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__FLAGS callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__FLAGS)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_flags(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__FLAGSv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__FLAGS)(xpointer instance, xuint arg_0, xpointer data);

    xuint arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__FLAGS callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xuint)va_arg(args_copy, xuint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__FLAGS)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__FLOAT(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__FLOAT)(xpointer data1, xfloat arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__FLOAT callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__FLOAT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_float(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__FLOATv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__FLOAT)(xpointer instance, xfloat arg_0, xpointer data);

    xfloat arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__FLOAT callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xfloat)va_arg(args_copy, xdouble);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__FLOAT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__DOUBLE(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__DOUBLE)(xpointer data1, xdouble arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__DOUBLE callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__DOUBLE)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_double(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__DOUBLEv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__DOUBLE)(xpointer instance, xdouble arg_0, xpointer data);

    xdouble arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__DOUBLE callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xdouble)va_arg(args_copy, xdouble);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__DOUBLE)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__STRING(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__STRING)(xpointer data1, xpointer arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__STRING callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__STRING)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_string(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__STRINGv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__STRING)(xpointer instance, xpointer arg_0, xpointer data);

    xpointer arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__STRING callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        arg0 = x_strdup((const xchar *)arg0);
    }
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__STRING)(marshal_data ? marshal_data : cc->callback);
    callback (data1, arg0, data2);

    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        x_free(arg0);
    }
}

void x_cclosure_marshal_VOID__PARAM(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__PARAM)(xpointer data1, xpointer arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__PARAM callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__PARAM)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_param(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__PARAMv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__PARAM)(xpointer instance, xpointer arg_0, xpointer data);

    xpointer arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__PARAM callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xpointer) va_arg(args_copy, xpointer);
    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        arg0 = x_param_spec_ref((XParamSpec *)arg0);
    }
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__PARAM)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);

    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        x_param_spec_unref((XParamSpec *)arg0);
    }
}

void x_cclosure_marshal_VOID__BOXED(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__BOXED)(xpointer data1, xpointer arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__BOXED callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__BOXED)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_boxed(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__BOXEDv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__BOXED)(xpointer instance, xpointer arg_0, xpointer data);

    xpointer arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__BOXED callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        arg0 = x_boxed_copy(param_types[0] & ~X_SIGNAL_TYPE_STATIC_SCOPE, arg0);
    }
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__BOXED)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);

    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        x_boxed_free(param_types[0] & ~X_SIGNAL_TYPE_STATIC_SCOPE, arg0);
    }
}

void x_cclosure_marshal_VOID__POINTER(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__POINTER)(xpointer data1, xpointer arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__POINTER callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__POINTER)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_pointer(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__POINTERv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__POINTER)(xpointer instance, xpointer arg_0, xpointer data);

    xpointer arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__POINTER callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__POINTER)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);
}

void x_cclosure_marshal_VOID__OBJECT(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__OBJECT)(xpointer data1, xpointer arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__OBJECT callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__OBJECT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_object(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__OBJECTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__OBJECT)(xpointer instance, xpointer arg_0, xpointer data);

    xpointer arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__OBJECT callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    if (arg0 != NULL) {
        arg0 = x_object_ref(arg0);
    }
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__OBJECT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);

    if (arg0 != NULL) {
        x_object_unref(arg0);
    }
}

void x_cclosure_marshal_VOID__VARIANT(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__VARIANT)(xpointer data1, xpointer arg_1, xpointer data2);

    xpointer data1, data2;
    XMarshalFunc_VOID__VARIANT callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__VARIANT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_variant(param_values + 1), data2);
}

void x_cclosure_marshal_VOID__VARIANTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__VARIANT)(xpointer instance, xpointer arg_0, xpointer data);

    xpointer arg0;
    va_list args_copy;
    xpointer data1, data2;
    XMarshalFunc_VOID__VARIANT callback;
    XCClosure *cc = (XCClosure *)closure;

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        arg0 = x_variant_ref_sink((XVariant *)arg0);
    }
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__VARIANT)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, data2);

    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        x_variant_unref((XVariant *)arg0);
    }
}

void x_cclosure_marshal_VOID__UINT_POINTER(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef void (*XMarshalFunc_VOID__UINT_POINTER)(xpointer data1, xuint arg_1, xpointer arg_2, xpointer data2);

    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_VOID__UINT_POINTER callback;

    x_return_if_fail(n_param_values == 3);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__UINT_POINTER)(marshal_data ? marshal_data : cc->callback);
    callback(data1, x_marshal_value_peek_uint(param_values + 1), x_marshal_value_peek_pointer(param_values + 2), data2);
}

void x_cclosure_marshal_VOID__UINT_POINTERv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef void (*XMarshalFunc_VOID__UINT_POINTER)(xpointer instance, xuint arg_0, xpointer arg_1, xpointer data);

    xuint arg0;
    xpointer arg1;
    va_list args_copy;
    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_VOID__UINT_POINTER callback;

    va_copy(args_copy, args);
    arg0 = (xuint)va_arg(args_copy, xuint);
    arg1 = (xpointer)va_arg(args_copy, xpointer);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_VOID__UINT_POINTER)(marshal_data ? marshal_data : cc->callback);
    callback(data1, arg0, arg1, data2);
}

void x_cclosure_marshal_BOOLEAN__FLAGS(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef xboolean (*XMarshalFunc_BOOLEAN__FLAGS)(xpointer data1, xuint arg_1, xpointer data2);

    xboolean v_return;
    xpointer data1, data2;
    XMarshalFunc_BOOLEAN__FLAGS callback;
    XCClosure *cc = (XCClosure *)closure;

    x_return_if_fail(return_value != NULL);
    x_return_if_fail(n_param_values == 2);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_BOOLEAN__FLAGS)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(data1, x_marshal_value_peek_flags(param_values + 1), data2);

    x_value_set_boolean(return_value, v_return);
}

void x_cclosure_marshal_BOOLEAN__FLAGSv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef xboolean (*XMarshalFunc_BOOLEAN__FLAGS)(xpointer instance, xuint arg_0, xpointer data);

    xuint arg0;
    va_list args_copy;
    xboolean v_return;
    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_BOOLEAN__FLAGS callback;

    x_return_if_fail(return_value != NULL);

    va_copy(args_copy, args);
    arg0 = (xuint)va_arg(args_copy, xuint);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_BOOLEAN__FLAGS)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(data1, arg0, data2);

    x_value_set_boolean(return_value, v_return);
}

void x_cclosure_marshal_STRING__OBJECT_POINTER(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef xchar *(*XMarshalFunc_STRING__OBJECT_POINTER)(xpointer data1, xpointer arg_1, xpointer arg_2, xpointer data2);

    xchar *v_return;
    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_STRING__OBJECT_POINTER callback;

    x_return_if_fail(return_value != NULL);
    x_return_if_fail(n_param_values == 3);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_STRING__OBJECT_POINTER)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(data1, x_marshal_value_peek_object(param_values + 1), x_marshal_value_peek_pointer(param_values + 2), data2);

    x_value_take_string(return_value, v_return);
}

void x_cclosure_marshal_STRING__OBJECT_POINTERv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef xchar *(*XMarshalFunc_STRING__OBJECT_POINTER)(xpointer instance, xpointer arg_0, xpointer arg_1, xpointer data);

    xpointer arg0;
    xpointer arg1;
    xchar *v_return;
    va_list args_copy;
    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_STRING__OBJECT_POINTER callback;

    x_return_if_fail(return_value != NULL);

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    if (arg0 != NULL) {
        arg0 = x_object_ref(arg0);
    }

    arg1 = (xpointer)va_arg(args_copy, xpointer);
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_STRING__OBJECT_POINTER)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(data1, arg0, arg1, data2);
    if (arg0 != NULL) {
        x_object_unref(arg0);
    }

    x_value_take_string(return_value, v_return);
}

void x_cclosure_marshal_BOOLEAN__BOXED_BOXED(XClosure *closure, XValue *return_value X_GNUC_UNUSED, xuint n_param_values, const XValue *param_values, xpointer invocation_hint X_GNUC_UNUSED, xpointer marshal_data)
{
    typedef xboolean (*XMarshalFunc_BOOLEAN__BOXED_BOXED)(xpointer data1, xpointer arg_1, xpointer arg_2, xpointer data2);

    xboolean v_return;
    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_BOOLEAN__BOXED_BOXED callback;

    x_return_if_fail(return_value != NULL);
    x_return_if_fail(n_param_values == 3);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = x_value_peek_pointer(param_values + 0);
    } else {
        data1 = x_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (XMarshalFunc_BOOLEAN__BOXED_BOXED)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(data1, x_marshal_value_peek_boxed(param_values + 1), x_marshal_value_peek_boxed(param_values + 2), data2);

    x_value_set_boolean(return_value, v_return);
}

void x_cclosure_marshal_BOOLEAN__BOXED_BOXEDv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types)
{
    typedef xboolean (*XMarshalFunc_BOOLEAN__BOXED_BOXED)(xpointer instance, xpointer arg_0, xpointer arg_1, xpointer data);

    xpointer arg0;
    xpointer arg1;
    va_list args_copy;
    xboolean v_return;
    xpointer data1, data2;
    XCClosure *cc = (XCClosure *)closure;
    XMarshalFunc_BOOLEAN__BOXED_BOXED callback;

    x_return_if_fail(return_value != NULL);

    va_copy(args_copy, args);
    arg0 = (xpointer)va_arg(args_copy, xpointer);
    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        arg0 = x_boxed_copy(param_types[0] & ~X_SIGNAL_TYPE_STATIC_SCOPE, arg0);
    }

    arg1 = (xpointer)va_arg(args_copy, xpointer);
    if ((param_types[1] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL) {
        arg1 = x_boxed_copy(param_types[1] & ~X_SIGNAL_TYPE_STATIC_SCOPE, arg1);
    }
    va_end(args_copy);

    if (X_CCLOSURE_SWAP_DATA(closure)) {
        data1 = closure->data;
        data2 = instance;
    } else {
        data1 = instance;
        data2 = closure->data;
    }

    callback = (XMarshalFunc_BOOLEAN__BOXED_BOXED)(marshal_data ? marshal_data : cc->callback);
    v_return = callback(data1, arg0, arg1, data2);
    if ((param_types[0] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL) {
        x_boxed_free(param_types[0] & ~X_SIGNAL_TYPE_STATIC_SCOPE, arg0);
    }

    if ((param_types[1] & X_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL) {
        x_boxed_free(param_types[1] & ~X_SIGNAL_TYPE_STATIC_SCOPE, arg1);
    }

    x_value_set_boolean(return_value, v_return);
}
