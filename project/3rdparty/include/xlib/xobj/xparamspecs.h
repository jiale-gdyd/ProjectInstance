#ifndef __X_PARAMSPECS_H__
#define __X_PARAMSPECS_H__

#include "xvalue.h"
#include "xenums.h"
#include "xboxed.h"
#include "xobject.h"

X_BEGIN_DECLS

#define X_TYPE_PARAM_CHAR                       (x_param_spec_types[0])
#define X_IS_PARAM_SPEC_CHAR(pspec)             (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_CHAR))
#define X_PARAM_SPEC_CHAR(pspec)                (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_CHAR, XParamSpecChar))
#define X_TYPE_PARAM_UCHAR                      (x_param_spec_types[1])
#define X_IS_PARAM_SPEC_UCHAR(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_UCHAR))
#define X_PARAM_SPEC_UCHAR(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_UCHAR, XParamSpecUChar))
#define X_TYPE_PARAM_BOOLEAN                    (x_param_spec_types[2])
#define X_IS_PARAM_SPEC_BOOLEAN(pspec)          (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_BOOLEAN))
#define X_PARAM_SPEC_BOOLEAN(pspec)             (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_BOOLEAN, XParamSpecBoolean))
#define X_TYPE_PARAM_INT                        (x_param_spec_types[3])
#define X_IS_PARAM_SPEC_INT(pspec)              (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_INT))
#define X_PARAM_SPEC_INT(pspec)                 (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_INT, XParamSpecInt))
#define X_TYPE_PARAM_UINT                       (x_param_spec_types[4])
#define X_IS_PARAM_SPEC_UINT(pspec)             (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_UINT))
#define X_PARAM_SPEC_UINT(pspec)                (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_UINT, XParamSpecUInt))
#define X_TYPE_PARAM_LONG                       (x_param_spec_types[5])
#define X_IS_PARAM_SPEC_LONG(pspec)             (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_LONG))
#define X_PARAM_SPEC_LONG(pspec)                (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_LONG, XParamSpecLong))
#define X_TYPE_PARAM_ULONG                      (x_param_spec_types[6])
#define X_IS_PARAM_SPEC_ULONG(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_ULONG))
#define X_PARAM_SPEC_ULONG(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_ULONG, XParamSpecULong))
#define X_TYPE_PARAM_INT64                      (x_param_spec_types[7])
#define X_IS_PARAM_SPEC_INT64(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_INT64))
#define X_PARAM_SPEC_INT64(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_INT64, XParamSpecInt64))
#define X_TYPE_PARAM_UINT64                     (x_param_spec_types[8])
#define X_IS_PARAM_SPEC_UINT64(pspec)           (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_UINT64))
#define X_PARAM_SPEC_UINT64(pspec)              (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_UINT64, XParamSpecUInt64))
#define X_TYPE_PARAM_UNICHAR                    (x_param_spec_types[9])
#define X_PARAM_SPEC_UNICHAR(pspec)             (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_UNICHAR, XParamSpecUnichar))
#define X_IS_PARAM_SPEC_UNICHAR(pspec)          (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_UNICHAR))
#define X_TYPE_PARAM_ENUM                       (x_param_spec_types[10])
#define X_IS_PARAM_SPEC_ENUM(pspec)             (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_ENUM))
#define X_PARAM_SPEC_ENUM(pspec)                (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_ENUM, XParamSpecEnum))
#define X_TYPE_PARAM_FLAGS                      (x_param_spec_types[11])
#define X_IS_PARAM_SPEC_FLAGS(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_FLAGS))
#define X_PARAM_SPEC_FLAGS(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_FLAGS, XParamSpecFlags))

#define X_TYPE_PARAM_FLOAT                      (x_param_spec_types[12])
#define X_IS_PARAM_SPEC_FLOAT(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_FLOAT))
#define X_PARAM_SPEC_FLOAT(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_FLOAT, XParamSpecFloat))
#define X_TYPE_PARAM_DOUBLE                     (x_param_spec_types[13])
#define X_IS_PARAM_SPEC_DOUBLE(pspec)           (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_DOUBLE))
#define X_PARAM_SPEC_DOUBLE(pspec)              (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_DOUBLE, XParamSpecDouble))
#define X_TYPE_PARAM_STRING                     (x_param_spec_types[14])
#define X_IS_PARAM_SPEC_STRING(pspec)           (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_STRING))
#define X_PARAM_SPEC_STRING(pspec)              (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_STRING, XParamSpecString))
#define X_TYPE_PARAM_PARAM                      (x_param_spec_types[15])
#define X_IS_PARAM_SPEC_PARAM(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_PARAM))
#define X_PARAM_SPEC_PARAM(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_PARAM, XParamSpecParam))
#define X_TYPE_PARAM_BOXED                      (x_param_spec_types[16])
#define X_IS_PARAM_SPEC_BOXED(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_BOXED))
#define X_PARAM_SPEC_BOXED(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_BOXED, XParamSpecBoxed))
#define X_TYPE_PARAM_POINTER                    (x_param_spec_types[17])
#define X_IS_PARAM_SPEC_POINTER(pspec)          (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_POINTER))
#define X_PARAM_SPEC_POINTER(pspec)             (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_POINTER, XParamSpecPointer))
#define X_TYPE_PARAM_VALUE_ARRAY                (x_param_spec_types[18]) XLIB_DEPRECATED_MACRO_IN_2_32
#define X_IS_PARAM_SPEC_VALUE_ARRAY(pspec)      (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_VALUE_ARRAY)) XLIB_DEPRECATED_MACRO_IN_2_32
#define X_PARAM_SPEC_VALUE_ARRAY(pspec)         (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_VALUE_ARRAY, XParamSpecValueArray)) XLIB_DEPRECATED_MACRO_IN_2_32
#define X_TYPE_PARAM_OBJECT                     (x_param_spec_types[19])
#define X_IS_PARAM_SPEC_OBJECT(pspec)           (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_OBJECT))
#define X_PARAM_SPEC_OBJECT(pspec)              (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_OBJECT, XParamSpecObject))
#define X_TYPE_PARAM_OVERRIDE                   (x_param_spec_types[20])
#define X_IS_PARAM_SPEC_OVERRIDE(pspec)         (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_OVERRIDE))
#define X_PARAM_SPEC_OVERRIDE(pspec)            (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_OVERRIDE, XParamSpecOverride))
#define X_TYPE_PARAM_GTYPE                      (x_param_spec_types[21])
#define X_IS_PARAM_SPEC_GTYPE(pspec)            (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_GTYPE))
#define X_PARAM_SPEC_GTYPE(pspec)               (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_GTYPE, XParamSpecGType))
#define X_TYPE_PARAM_VARIANT                    (x_param_spec_types[22])
#define X_IS_PARAM_SPEC_VARIANT(pspec)          (X_TYPE_CHECK_INSTANCE_TYPE((pspec), X_TYPE_PARAM_VARIANT))
#define X_PARAM_SPEC_VARIANT(pspec)             (X_TYPE_CHECK_INSTANCE_CAST((pspec), X_TYPE_PARAM_VARIANT, XParamSpecVariant))

typedef struct _XParamSpecChar       XParamSpecChar;
typedef struct _XParamSpecUChar      XParamSpecUChar;
typedef struct _XParamSpecBoolean    XParamSpecBoolean;
typedef struct _XParamSpecInt        XParamSpecInt;
typedef struct _XParamSpecUInt       XParamSpecUInt;
typedef struct _XParamSpecLong       XParamSpecLong;
typedef struct _XParamSpecULong      XParamSpecULong;
typedef struct _XParamSpecInt64      XParamSpecInt64;
typedef struct _XParamSpecUInt64     XParamSpecUInt64;
typedef struct _XParamSpecUnichar    XParamSpecUnichar;
typedef struct _XParamSpecEnum       XParamSpecEnum;
typedef struct _XParamSpecFlags      XParamSpecFlags;
typedef struct _XParamSpecFloat      XParamSpecFloat;
typedef struct _XParamSpecDouble     XParamSpecDouble;
typedef struct _XParamSpecString     XParamSpecString;
typedef struct _XParamSpecParam      XParamSpecParam;
typedef struct _XParamSpecBoxed      XParamSpecBoxed;
typedef struct _XParamSpecPointer    XParamSpecPointer;
typedef struct _XParamSpecValueArray XParamSpecValueArray;
typedef struct _XParamSpecObject     XParamSpecObject;
typedef struct _XParamSpecOverride   XParamSpecOverride;
typedef struct _XParamSpecGType      XParamSpecGType;
typedef struct _XParamSpecVariant    XParamSpecVariant;

struct _XParamSpecChar {
    XParamSpec parent_instance;
    xint8      minimum;
    xint8      maximum;
    xint8      default_value;
};

struct _XParamSpecUChar {
    XParamSpec parent_instance;
    xuint8     minimum;
    xuint8     maximum;
    xuint8     default_value;
};

struct _XParamSpecBoolean {
    XParamSpec parent_instance;
    xboolean   default_value;
};

struct _XParamSpecInt {
    XParamSpec parent_instance;
    xint       minimum;
    xint       maximum;
    xint       default_value;
};

struct _XParamSpecUInt {
    XParamSpec parent_instance;
    xuint      minimum;
    xuint      maximum;
    xuint      default_value;
};

struct _XParamSpecLong {
    XParamSpec parent_instance;
    xlong      minimum;
    xlong      maximum;
    xlong      default_value;
};

struct _XParamSpecULong {
    XParamSpec parent_instance;
    xulong     minimum;
    xulong     maximum;
    xulong     default_value;
};

struct _XParamSpecInt64 {
    XParamSpec parent_instance;
    xint64     minimum;
    xint64     maximum;
    xint64     default_value;
};

struct _XParamSpecUInt64 {
    XParamSpec parent_instance;
    xuint64    minimum;
    xuint64    maximum;
    xuint64    default_value;
};

struct _XParamSpecUnichar {
    XParamSpec parent_instance;
    xunichar   default_value;
};

struct _XParamSpecEnum {
    XParamSpec parent_instance;
    XEnumClass *enum_class;
    xint       default_value;
};

struct _XParamSpecFlags {
    XParamSpec  parent_instance;
    XFlagsClass *flags_class;
    xuint       default_value;
};

struct _XParamSpecFloat {
    XParamSpec parent_instance;
    xfloat     minimum;
    xfloat     maximum;
    xfloat     default_value;
    xfloat     epsilon;
};

struct _XParamSpecDouble {
    XParamSpec parent_instance;
    xdouble    minimum;
    xdouble    maximum;
    xdouble    default_value;
    xdouble    epsilon;
};

struct _XParamSpecString {
    XParamSpec parent_instance;
    xchar      *default_value;
    xchar      *cset_first;
    xchar      *cset_nth;
    xchar      substitutor;
    xuint      null_fold_if_empty : 1;
    xuint      ensure_non_null : 1;
};

struct _XParamSpecParam {
    XParamSpec parent_instance;
};

struct _XParamSpecBoxed {
    XParamSpec parent_instance;
};

struct _XParamSpecPointer {
    XParamSpec parent_instance;
};

struct _XParamSpecValueArray {
    XParamSpec parent_instance;
    XParamSpec *element_spec;
    xuint      fixed_n_elements;
};

struct _XParamSpecObject {
    XParamSpec parent_instance;
};

struct _XParamSpecOverride {
    XParamSpec parent_instance;
    XParamSpec *overridden;
};

struct _XParamSpecGType {
    XParamSpec parent_instance;
    XType      is_a_type;
};

struct _XParamSpecVariant {
    XParamSpec   parent_instance;
    XVariantType *type;
    XVariant     *default_value;
    xpointer     padding[4];
};

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_char(const xchar *name, const xchar *nick, const xchar *blurb, xint8 minimum, xint8 maximum, xint8 default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_uchar(const xchar *name, const xchar *nick, const xchar *blurb, xuint8 minimum, xuint8 maximum, xuint8 default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_boolean(const xchar *name, const xchar *nick, const xchar *blurb, xboolean default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_int(const xchar *name, const xchar *nick, const xchar *blurb, xint minimum, xint maximum, xint default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_uint(const xchar *name, const xchar *nick, const xchar *blurb, xuint minimum, xuint maximum, xuint default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_long(const xchar *name, const xchar *nick, const xchar *blurb, xlong minimum, xlong maximum, xlong default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_ulong(const xchar *name, const xchar *nick, const xchar *blurb, xulong minimum, xulong maximum, xulong default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_int64(const xchar *name, const xchar *nick, const xchar *blurb, xint64 minimum, xint64 maximum, xint64 default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_uint64(const xchar *name, const xchar *nick, const xchar *blurb, xuint64 minimum, xuint64 maximum, xuint64 default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_unichar(const xchar *name, const xchar *nick, const xchar *blurb, xunichar default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_enum(const xchar *name, const xchar *nick, const xchar *blurb, XType enum_type, xint default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_flags(const xchar *name, const xchar *nick, const xchar *blurb, XType flags_type, xuint default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_float(const xchar *name, const xchar *nick, const xchar *blurb, xfloat minimum, xfloat maximum, xfloat default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_double(const xchar *name, const xchar *nick, const xchar *blurb, xdouble minimum, xdouble maximum, xdouble default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_string(const xchar *name, const xchar *nick, const xchar *blurb, const xchar *default_value, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_param(const xchar *name, const xchar *nick, const xchar *blurb, XType param_type, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_boxed(const xchar *name, const xchar *nick, const xchar *blurb, XType boxed_type, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_pointer(const xchar *name, const xchar *nick, const xchar *blurb, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_value_array(const xchar *name, const xchar *nick, const xchar *blurb, XParamSpec *element_spec, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_object(const xchar *name, const xchar *nick, const xchar *blurb, XType object_type, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_override(const xchar *name, XParamSpec *overridden);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_gtype(const xchar *name, const xchar *nick, const xchar *blurb, XType is_a_type, XParamFlags flags);

XLIB_AVAILABLE_IN_ALL
XParamSpec *x_param_spec_variant(const xchar *name, const xchar *nick, const xchar *blurb, const XVariantType *type, XVariant *default_value, XParamFlags flags);

#ifndef XOBJECT_VAR
#define XOBJECT_VAR     _XLIB_EXTERN
#endif

XOBJECT_VAR XType *x_param_spec_types;

X_END_DECLS

#endif
