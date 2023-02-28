#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xvalue.h>
#include <xlib/xobj/xenums.h>
#include <xlib/xobj/xtype-private.h>

static void value_transform_memcpy_data0(const XValue *src_value, XValue *dest_value)
{
    memcpy(&dest_value->data[0], &src_value->data[0], sizeof (src_value->data[0]));
}

#define value_transform_int_int         value_transform_memcpy_data0
#define value_transform_uint_uint       value_transform_memcpy_data0
#define value_transform_long_long       value_transform_memcpy_data0
#define value_transform_ulong_ulong     value_transform_memcpy_data0
#define value_transform_int64_int64     value_transform_memcpy_data0
#define value_transform_uint64_uint64   value_transform_memcpy_data0
#define value_transform_float_float     value_transform_memcpy_data0
#define value_transform_double_double   value_transform_memcpy_data0

#define DEFINE_CAST(func_name, from_member, ctype, to_member)                               \
    static void value_transform_##func_name(const XValue *src_value, XValue *dest_value)    \
    {                                                                                       \
        ctype c_value = src_value->data[0].from_member;                                     \
        dest_value->data[0].to_member = c_value;                                            \
    } extern void xlib_dummy_decl(void)

DEFINE_CAST(int_s8,            v_int,    xint8,   v_int);
DEFINE_CAST(int_u8,            v_int,    xuint8,  v_uint);
DEFINE_CAST(int_uint,          v_int,    xuint,   v_uint);
DEFINE_CAST(int_long,          v_int,    xlong,   v_long);
DEFINE_CAST(int_ulong,         v_int,    xulong,  v_ulong);
DEFINE_CAST(int_int64,         v_int,    xint64,  v_int64);
DEFINE_CAST(int_uint64,        v_int,    xuint64, v_uint64);
DEFINE_CAST(int_float,         v_int,    xfloat,  v_float);
DEFINE_CAST(int_double,        v_int,    xdouble, v_double);
DEFINE_CAST(uint_s8,           v_uint,   xint8,   v_int);
DEFINE_CAST(uint_u8,           v_uint,   xuint8,  v_uint);
DEFINE_CAST(uint_int,          v_uint,   xint,    v_int);
DEFINE_CAST(uint_long,         v_uint,   xlong,   v_long);
DEFINE_CAST(uint_ulong,        v_uint,   xulong,  v_ulong);
DEFINE_CAST(uint_int64,        v_uint,   xint64,  v_int64);
DEFINE_CAST(uint_uint64,       v_uint,   xuint64, v_uint64);
DEFINE_CAST(uint_float,        v_uint,   xfloat,  v_float);
DEFINE_CAST(uint_double,       v_uint,   xdouble, v_double);
DEFINE_CAST(long_s8,           v_long,   xint8,   v_int);
DEFINE_CAST(long_u8,           v_long,   xuint8,  v_uint);
DEFINE_CAST(long_int,          v_long,   xint,    v_int);
DEFINE_CAST(long_uint,         v_long,   xuint,   v_uint);
DEFINE_CAST(long_ulong,        v_long,   xulong,  v_ulong);
DEFINE_CAST(long_int64,        v_long,   xint64,  v_int64);
DEFINE_CAST(long_uint64,       v_long,   xuint64, v_uint64);
DEFINE_CAST(long_float,        v_long,   xfloat,  v_float);
DEFINE_CAST(long_double,       v_long,   xdouble, v_double);
DEFINE_CAST(ulong_s8,          v_ulong,  xint8,   v_int);
DEFINE_CAST(ulong_u8,          v_ulong,  xuint8,  v_uint);
DEFINE_CAST(ulong_int,         v_ulong,  xint,    v_int);
DEFINE_CAST(ulong_uint,        v_ulong,  xuint,   v_uint);
DEFINE_CAST(ulong_int64,       v_ulong,  xint64,  v_int64);
DEFINE_CAST(ulong_uint64,      v_ulong,  xuint64, v_uint64);
DEFINE_CAST(ulong_long,        v_ulong,  xlong,   v_long);
DEFINE_CAST(ulong_float,       v_ulong,  xfloat,  v_float);
DEFINE_CAST(ulong_double,      v_ulong,  xdouble, v_double);
DEFINE_CAST(int64_s8,          v_int64,  xint8,   v_int);
DEFINE_CAST(int64_u8,          v_int64,  xuint8,  v_uint);
DEFINE_CAST(int64_int,         v_int64,  xint,    v_int);
DEFINE_CAST(int64_uint,        v_int64,  xuint,   v_uint);
DEFINE_CAST(int64_long,        v_int64,  xlong,   v_long);
DEFINE_CAST(int64_uint64,      v_int64,  xuint64, v_uint64);
DEFINE_CAST(int64_ulong,       v_int64,  xulong,  v_ulong);
DEFINE_CAST(int64_float,       v_int64,  xfloat,  v_float);
DEFINE_CAST(int64_double,      v_int64,  xdouble, v_double);
DEFINE_CAST(uint64_s8,         v_uint64, xint8,   v_int);
DEFINE_CAST(uint64_u8,         v_uint64, xuint8,  v_uint);
DEFINE_CAST(uint64_int,        v_uint64, xint,    v_int);
DEFINE_CAST(uint64_uint,       v_uint64, xuint,   v_uint);
DEFINE_CAST(uint64_long,       v_uint64, xlong,   v_long);
DEFINE_CAST(uint64_ulong,      v_uint64, xulong,  v_ulong);
DEFINE_CAST(uint64_int64,      v_uint64, xint64,  v_int64);
DEFINE_CAST(uint64_float,      v_uint64, xfloat,  v_float);
DEFINE_CAST(uint64_double,     v_uint64, xdouble, v_double);
DEFINE_CAST(float_s8,          v_float,  xint8,   v_int);
DEFINE_CAST(float_u8,          v_float,  xuint8,  v_uint);
DEFINE_CAST(float_int,         v_float,  xint,    v_int);
DEFINE_CAST(float_uint,        v_float,  xuint,   v_uint);
DEFINE_CAST(float_long,        v_float,  xlong,   v_long);
DEFINE_CAST(float_ulong,       v_float,  xulong,  v_ulong);
DEFINE_CAST(float_int64,       v_float,  xint64,  v_int64);
DEFINE_CAST(float_uint64,      v_float,  xuint64, v_uint64);
DEFINE_CAST(float_double,      v_float,  xdouble, v_double);
DEFINE_CAST(double_s8,         v_double, xint8,   v_int);
DEFINE_CAST(double_u8,         v_double, xuint8,  v_uint);
DEFINE_CAST(double_int,        v_double, xint,    v_int);
DEFINE_CAST(double_uint,       v_double, xuint,   v_uint);
DEFINE_CAST(double_long,       v_double, xlong,   v_long);
DEFINE_CAST(double_ulong,      v_double, xulong,  v_ulong);
DEFINE_CAST(double_int64,      v_double, xint64,  v_int64);
DEFINE_CAST(double_uint64,     v_double, xuint64, v_uint64);
DEFINE_CAST(double_float,      v_double, xfloat,  v_float);

#define DEFINE_BOOL_CHECK(func_name, from_member)                                           \
    static void value_transform_##func_name(const XValue *src_value, XValue *dest_value)    \
    {                                                                                       \
        dest_value->data[0].v_int = src_value->data[0].from_member != 0;                    \
    } extern void xlib_dummy_decl(void)

DEFINE_BOOL_CHECK(int_bool,    v_int);
DEFINE_BOOL_CHECK(uint_bool,   v_uint);
DEFINE_BOOL_CHECK(long_bool,   v_long);
DEFINE_BOOL_CHECK(ulong_bool,  v_ulong);
DEFINE_BOOL_CHECK(int64_bool,  v_int64);
DEFINE_BOOL_CHECK(uint64_bool, v_uint64);

#define DEFINE_SPRINTF(func_name, from_member, format)                                              \
    static void value_transform_##func_name(const XValue *src_value, XValue *dest_value)            \
    {                                                                                               \
        dest_value->data[0].v_pointer = x_strdup_printf((format), src_value->data[0].from_member);  \
    } extern void xlib_dummy_decl(void)

DEFINE_SPRINTF(int_string,     v_int,    "%d");
DEFINE_SPRINTF(uint_string,    v_uint,   "%u");
DEFINE_SPRINTF(long_string,    v_long,   "%ld");
DEFINE_SPRINTF(ulong_string,   v_ulong,  "%lu");
DEFINE_SPRINTF(int64_string,   v_int64,  "%" X_XINT64_FORMAT);
DEFINE_SPRINTF(uint64_string,  v_uint64, "%" X_XUINT64_FORMAT);
DEFINE_SPRINTF(float_string,   v_float,  "%f");
DEFINE_SPRINTF(double_string,  v_double, "%f");

static void value_transform_bool_string(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_pointer = x_strdup_printf("%s", src_value->data[0].v_int ? "TRUE" : "FALSE");
}

static void value_transform_string_string(const XValue *src_value, XValue *dest_value)
{
    dest_value->data[0].v_pointer = x_strdup((const xchar *)src_value->data[0].v_pointer);
}

static void value_transform_enum_string(const XValue *src_value, XValue *dest_value)
{
    xint v_enum = src_value->data[0].v_long;
    xchar *str = x_enum_to_string(X_VALUE_TYPE(src_value), v_enum);

    dest_value->data[0].v_pointer = str;
}

static void value_transform_flags_string(const XValue *src_value, XValue *dest_value)
{
    XFlagsClass *classt = (XFlagsClass *)x_type_class_ref(X_VALUE_TYPE(src_value));
    XFlagsValue *flags_value = x_flags_get_first_value(classt, src_value->data[0].v_ulong);

    if (flags_value) {
        XString *gstring = x_string_new(NULL);
        xuint v_flags = src_value->data[0].v_ulong;

        do {
            v_flags &= ~flags_value->value;

            if (gstring->str[0]) {
                x_string_append(gstring, " | ");
            }

            x_string_append(gstring, flags_value->value_name);
            flags_value = x_flags_get_first_value(classt, v_flags);
        } while (flags_value && v_flags);

        if (v_flags) {
            dest_value->data[0].v_pointer = x_strdup_printf("%s | %u", gstring->str, v_flags);
        } else {
            dest_value->data[0].v_pointer = x_strdup(gstring->str);
        }

        x_string_free(gstring, TRUE);
    } else {
        dest_value->data[0].v_pointer = x_strdup_printf("%lu", src_value->data[0].v_ulong);
    }

    x_type_class_unref(classt);
}

void _x_value_transforms_init(void)
{
#define SKIP____register_transform_func(type1, type2, transform_func)       (void)0

    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_CHAR,            value_transform_int_int);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_UCHAR,           value_transform_int_u8);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_BOOLEAN,         value_transform_int_bool);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_INT,             value_transform_int_int);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_UINT,            value_transform_int_uint);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_LONG,            value_transform_int_long);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_ULONG,           value_transform_int_ulong);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_INT64,           value_transform_int_int64);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_UINT64,          value_transform_int_uint64);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_ENUM,            value_transform_int_long);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_FLAGS,           value_transform_int_ulong);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_FLOAT,           value_transform_int_float);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_DOUBLE,          value_transform_int_double);
    x_value_register_transform_func(X_TYPE_CHAR,         X_TYPE_STRING,          value_transform_int_string);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_CHAR,            value_transform_uint_s8);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_UCHAR,           value_transform_uint_uint);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_BOOLEAN,         value_transform_uint_bool);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_INT,             value_transform_uint_int);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_UINT,            value_transform_uint_uint);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_LONG,            value_transform_uint_long);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_ULONG,           value_transform_uint_ulong);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_INT64,           value_transform_uint_int64);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_UINT64,          value_transform_uint_uint64);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_ENUM,            value_transform_uint_long);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_FLAGS,           value_transform_uint_ulong);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_FLOAT,           value_transform_uint_float);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_DOUBLE,          value_transform_uint_double);
    x_value_register_transform_func(X_TYPE_UCHAR,        X_TYPE_STRING,          value_transform_uint_string);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_CHAR,            value_transform_int_s8);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_UCHAR,           value_transform_int_u8);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_BOOLEAN,         value_transform_int_int);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_INT,             value_transform_int_int);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_UINT,            value_transform_int_uint);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_LONG,            value_transform_int_long);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_ULONG,           value_transform_int_ulong);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_INT64,           value_transform_int_int64);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_UINT64,          value_transform_int_uint64);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_ENUM,            value_transform_int_long);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_FLAGS,           value_transform_int_ulong);
    SKIP____register_transform_func (X_TYPE_BOOLEAN,      X_TYPE_FLOAT,           value_transform_int_float);
    SKIP____register_transform_func (X_TYPE_BOOLEAN,      X_TYPE_DOUBLE,          value_transform_int_double);
    x_value_register_transform_func(X_TYPE_BOOLEAN,      X_TYPE_STRING,          value_transform_bool_string);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_CHAR,            value_transform_int_s8);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_UCHAR,           value_transform_int_u8);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_BOOLEAN,         value_transform_int_bool);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_INT,             value_transform_int_int);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_UINT,            value_transform_int_uint);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_LONG,            value_transform_int_long);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_ULONG,           value_transform_int_ulong);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_INT64,           value_transform_int_int64);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_UINT64,          value_transform_int_uint64);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_ENUM,            value_transform_int_long);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_FLAGS,           value_transform_int_ulong);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_FLOAT,           value_transform_int_float);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_DOUBLE,          value_transform_int_double);
    x_value_register_transform_func(X_TYPE_INT,          X_TYPE_STRING,          value_transform_int_string);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_CHAR,            value_transform_uint_s8);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_UCHAR,           value_transform_uint_u8);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_BOOLEAN,         value_transform_uint_bool);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_INT,             value_transform_uint_int);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_UINT,            value_transform_uint_uint);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_LONG,            value_transform_uint_long);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_ULONG,           value_transform_uint_ulong);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_INT64,           value_transform_uint_int64);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_UINT64,          value_transform_uint_uint64);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_ENUM,            value_transform_uint_long);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_FLAGS,           value_transform_uint_ulong);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_FLOAT,           value_transform_uint_float);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_DOUBLE,          value_transform_uint_double);
    x_value_register_transform_func(X_TYPE_UINT,         X_TYPE_STRING,          value_transform_uint_string);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_CHAR,            value_transform_long_s8);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_UCHAR,           value_transform_long_u8);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_BOOLEAN,         value_transform_long_bool);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_INT,             value_transform_long_int);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_UINT,            value_transform_long_uint);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_LONG,            value_transform_long_long);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_ULONG,           value_transform_long_ulong);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_INT64,           value_transform_long_int64);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_UINT64,          value_transform_long_uint64);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_ENUM,            value_transform_long_long);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_FLAGS,           value_transform_long_ulong);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_FLOAT,           value_transform_long_float);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_DOUBLE,          value_transform_long_double);
    x_value_register_transform_func(X_TYPE_LONG,         X_TYPE_STRING,          value_transform_long_string);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_CHAR,            value_transform_ulong_s8);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_UCHAR,           value_transform_ulong_u8);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_BOOLEAN,         value_transform_ulong_bool);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_INT,             value_transform_ulong_int);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_UINT,            value_transform_ulong_uint);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_LONG,            value_transform_ulong_long);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_ULONG,           value_transform_ulong_ulong);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_INT64,           value_transform_ulong_int64);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_UINT64,          value_transform_ulong_uint64);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_ENUM,            value_transform_ulong_long);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_FLAGS,           value_transform_ulong_ulong);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_FLOAT,           value_transform_ulong_float);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_DOUBLE,          value_transform_ulong_double);
    x_value_register_transform_func(X_TYPE_ULONG,        X_TYPE_STRING,          value_transform_ulong_string);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_CHAR,            value_transform_int64_s8);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_UCHAR,           value_transform_int64_u8);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_BOOLEAN,         value_transform_int64_bool);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_INT,             value_transform_int64_int);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_UINT,            value_transform_int64_uint);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_LONG,            value_transform_int64_long);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_ULONG,           value_transform_int64_ulong);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_INT64,           value_transform_int64_int64);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_UINT64,          value_transform_int64_uint64);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_ENUM,            value_transform_int64_long);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_FLAGS,           value_transform_int64_ulong);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_FLOAT,           value_transform_int64_float);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_DOUBLE,          value_transform_int64_double);
    x_value_register_transform_func(X_TYPE_INT64,        X_TYPE_STRING,          value_transform_int64_string);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_CHAR,            value_transform_uint64_s8);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_UCHAR,           value_transform_uint64_u8);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_BOOLEAN,         value_transform_uint64_bool);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_INT,             value_transform_uint64_int);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_UINT,            value_transform_uint64_uint);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_LONG,            value_transform_uint64_long);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_ULONG,           value_transform_uint64_ulong);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_INT64,           value_transform_uint64_int64);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_UINT64,          value_transform_uint64_uint64);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_ENUM,            value_transform_uint64_long);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_FLAGS,           value_transform_uint64_ulong);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_FLOAT,           value_transform_uint64_float);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_DOUBLE,          value_transform_uint64_double);
    x_value_register_transform_func(X_TYPE_UINT64,       X_TYPE_STRING,          value_transform_uint64_string);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_CHAR,            value_transform_long_s8);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_UCHAR,           value_transform_long_u8);
    SKIP____register_transform_func (X_TYPE_ENUM,         X_TYPE_BOOLEAN,         value_transform_long_bool);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_INT,             value_transform_long_int);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_UINT,            value_transform_long_uint);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_LONG,            value_transform_long_long);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_ULONG,           value_transform_long_ulong);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_INT64,           value_transform_long_int64);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_UINT64,          value_transform_long_uint64);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_ENUM,            value_transform_long_long);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_FLAGS,           value_transform_long_ulong);
    SKIP____register_transform_func (X_TYPE_ENUM,         X_TYPE_FLOAT,           value_transform_long_float);
    SKIP____register_transform_func (X_TYPE_ENUM,         X_TYPE_DOUBLE,          value_transform_long_double);
    x_value_register_transform_func(X_TYPE_ENUM,         X_TYPE_STRING,          value_transform_enum_string);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_CHAR,            value_transform_ulong_s8);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_UCHAR,           value_transform_ulong_u8);
    SKIP____register_transform_func (X_TYPE_FLAGS,        X_TYPE_BOOLEAN,         value_transform_ulong_bool);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_INT,             value_transform_ulong_int);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_UINT,            value_transform_ulong_uint);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_LONG,            value_transform_ulong_long);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_ULONG,           value_transform_ulong_ulong);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_INT64,           value_transform_ulong_int64);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_UINT64,          value_transform_ulong_uint64);
    SKIP____register_transform_func (X_TYPE_FLAGS,        X_TYPE_ENUM,            value_transform_ulong_long);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_FLAGS,           value_transform_ulong_ulong);
    SKIP____register_transform_func (X_TYPE_FLAGS,        X_TYPE_FLOAT,           value_transform_ulong_float);
    SKIP____register_transform_func (X_TYPE_FLAGS,        X_TYPE_DOUBLE,          value_transform_ulong_double);
    x_value_register_transform_func(X_TYPE_FLAGS,        X_TYPE_STRING,          value_transform_flags_string);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_CHAR,            value_transform_float_s8);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_UCHAR,           value_transform_float_u8);
    SKIP____register_transform_func (X_TYPE_FLOAT,        X_TYPE_BOOLEAN,         value_transform_float_bool);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_INT,             value_transform_float_int);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_UINT,            value_transform_float_uint);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_LONG,            value_transform_float_long);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_ULONG,           value_transform_float_ulong);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_INT64,           value_transform_float_int64);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_UINT64,          value_transform_float_uint64);
    SKIP____register_transform_func (X_TYPE_FLOAT,        X_TYPE_ENUM,            value_transform_float_long);
    SKIP____register_transform_func (X_TYPE_FLOAT,        X_TYPE_FLAGS,           value_transform_float_ulong);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_FLOAT,           value_transform_float_float);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_DOUBLE,          value_transform_float_double);
    x_value_register_transform_func(X_TYPE_FLOAT,        X_TYPE_STRING,          value_transform_float_string);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_CHAR,            value_transform_double_s8);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_UCHAR,           value_transform_double_u8);
    SKIP____register_transform_func (X_TYPE_DOUBLE,       X_TYPE_BOOLEAN,         value_transform_double_bool);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_INT,             value_transform_double_int);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_UINT,            value_transform_double_uint);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_LONG,            value_transform_double_long);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_ULONG,           value_transform_double_ulong);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_INT64,           value_transform_double_int64);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_UINT64,          value_transform_double_uint64);
    SKIP____register_transform_func (X_TYPE_DOUBLE,       X_TYPE_ENUM,            value_transform_double_long);
    SKIP____register_transform_func (X_TYPE_DOUBLE,       X_TYPE_FLAGS,           value_transform_double_ulong);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_FLOAT,           value_transform_double_float);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_DOUBLE,          value_transform_double_double);
    x_value_register_transform_func(X_TYPE_DOUBLE,       X_TYPE_STRING,          value_transform_double_string);

    x_value_register_transform_func(X_TYPE_STRING,       X_TYPE_STRING,          value_transform_string_string);
}
