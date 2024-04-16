#include <string.h>

#include <xlib/xlib/config.h>

#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xobj/xparamspecs.h>
#include <xlib/xobj/xvaluearray.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

#define X_FLOAT_EPSILON                     (1e-30)
#define X_DOUBLE_EPSILON                    (1e-90)

static void param_char_init(XParamSpec *pspec)
{
    XParamSpecChar *cspec = X_PARAM_SPEC_CHAR(pspec);

    cspec->minimum = 0x7f;
    cspec->maximum = 0x80;
    cspec->default_value = 0;
}

static void param_char_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_int = X_PARAM_SPEC_CHAR(pspec)->default_value;
}

static xboolean param_char_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecChar *cspec = X_PARAM_SPEC_CHAR(pspec);
    xint oval = value->data[0].v_int;

    return cspec->minimum <= oval && oval <= cspec->maximum;
}

static xboolean param_char_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecChar *cspec = X_PARAM_SPEC_CHAR(pspec);
    xint oval = value->data[0].v_int;

    value->data[0].v_int = CLAMP(value->data[0].v_int, cspec->minimum, cspec->maximum);
    return value->data[0].v_int != oval;
}

static void param_uchar_init(XParamSpec *pspec)
{
    XParamSpecUChar *uspec = X_PARAM_SPEC_UCHAR(pspec);

    uspec->minimum = 0;
    uspec->maximum = 0xff;
    uspec->default_value = 0;
}

static void param_uchar_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_uint = X_PARAM_SPEC_UCHAR(pspec)->default_value;
}

static xboolean param_uchar_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecUChar *uspec = X_PARAM_SPEC_UCHAR(pspec);
    xuint oval = value->data[0].v_uint;

    return uspec->minimum <= oval && oval <= uspec->maximum;
}

static xboolean param_uchar_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecUChar *uspec = X_PARAM_SPEC_UCHAR(pspec);
    xuint oval = value->data[0].v_uint;

    value->data[0].v_uint = CLAMP(value->data[0].v_uint, uspec->minimum, uspec->maximum);
    return value->data[0].v_uint != oval;
}

static void param_boolean_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_int = X_PARAM_SPEC_BOOLEAN(pspec)->default_value;
}

static xboolean param_boolean_is_valid(XParamSpec *pspec, const XValue *value)
{
    int oval = value->data[0].v_int;
    return oval == FALSE || oval == TRUE;
}

static xboolean param_boolean_validate(XParamSpec *pspec, XValue *value)
{
    xint oval = value->data[0].v_int;

    value->data[0].v_int = value->data[0].v_int != FALSE;
    return value->data[0].v_int != oval;
}

static void param_int_init(XParamSpec *pspec)
{
    XParamSpecInt *ispec = X_PARAM_SPEC_INT(pspec);

    ispec->minimum = 0x7fffffff;
    ispec->maximum = 0x80000000;
    ispec->default_value = 0;
}

static void param_int_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_int = X_PARAM_SPEC_INT(pspec)->default_value;
}

static xboolean param_int_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecInt *ispec = X_PARAM_SPEC_INT(pspec);
    int oval = value->data[0].v_int;

    return ispec->minimum <= oval && oval <= ispec->maximum;
}

static xboolean param_int_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecInt *ispec = X_PARAM_SPEC_INT(pspec);
    xint oval = value->data[0].v_int;

    value->data[0].v_int = CLAMP(value->data[0].v_int, ispec->minimum, ispec->maximum);
    return value->data[0].v_int != oval;
}

static xint param_int_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_int < value2->data[0].v_int) {
        return -1;
    } else {
        return value1->data[0].v_int > value2->data[0].v_int;
    }
}

static void param_uint_init(XParamSpec *pspec)
{
    XParamSpecUInt *uspec = X_PARAM_SPEC_UINT(pspec);

    uspec->minimum = 0;
    uspec->maximum = 0xffffffff;
    uspec->default_value = 0;
}

static void param_uint_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_uint = X_PARAM_SPEC_UINT(pspec)->default_value;
}

static xboolean param_uint_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecUInt *uspec = X_PARAM_SPEC_UINT(pspec);
    xuint oval = value->data[0].v_uint;

    return uspec->minimum <= oval && oval <= uspec->maximum;
}

static xboolean param_uint_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecUInt *uspec = X_PARAM_SPEC_UINT(pspec);
    xuint oval = value->data[0].v_uint;

    value->data[0].v_uint = CLAMP(value->data[0].v_uint, uspec->minimum, uspec->maximum);
    return value->data[0].v_uint != oval;
}

static xint param_uint_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_uint < value2->data[0].v_uint) {
        return -1;
    } else {
        return value1->data[0].v_uint > value2->data[0].v_uint;
    }
}

static void param_long_init(XParamSpec *pspec)
{
    XParamSpecLong *lspec = X_PARAM_SPEC_LONG(pspec);

#if SIZEOF_LONG == 4
    lspec->minimum = 0x7fffffff;
    lspec->maximum = 0x80000000;
#else
    lspec->minimum = 0x7fffffffffffffff;
    lspec->maximum = 0x8000000000000000;
#endif
    lspec->default_value = 0;
}

static void param_long_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_long = X_PARAM_SPEC_LONG(pspec)->default_value;
}

static xboolean param_long_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecLong *lspec = X_PARAM_SPEC_LONG(pspec);
    xlong oval = value->data[0].v_long;

    return lspec->minimum <= oval && oval <= lspec->maximum;
}

static xboolean param_long_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecLong *lspec = X_PARAM_SPEC_LONG(pspec);
    xlong oval = value->data[0].v_long;

    value->data[0].v_long = CLAMP(value->data[0].v_long, lspec->minimum, lspec->maximum);
    return value->data[0].v_long != oval;
}

static xint param_long_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_long < value2->data[0].v_long) {
        return -1;
    } else {
        return value1->data[0].v_long > value2->data[0].v_long;
    }
}

static void param_ulong_init(XParamSpec *pspec)
{
    XParamSpecULong *uspec = X_PARAM_SPEC_ULONG(pspec);

    uspec->minimum = 0;
#if SIZEOF_LONG == 4
    uspec->maximum = 0xffffffff;
#else
    uspec->maximum = 0xffffffffffffffff;
#endif
    uspec->default_value = 0;
}

static void param_ulong_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_ulong = X_PARAM_SPEC_ULONG(pspec)->default_value;
}

static xboolean param_ulong_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecULong *uspec = X_PARAM_SPEC_ULONG(pspec);
    xulong oval = value->data[0].v_ulong;

    return uspec->minimum <= oval && oval <= uspec->maximum;
}

static xboolean param_ulong_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecULong *uspec = X_PARAM_SPEC_ULONG(pspec);
    xulong oval = value->data[0].v_ulong;

    value->data[0].v_ulong = CLAMP(value->data[0].v_ulong, uspec->minimum, uspec->maximum);
    return value->data[0].v_ulong != oval;
}

static xint param_ulong_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_ulong < value2->data[0].v_ulong) {
        return -1;
    } else {
        return value1->data[0].v_ulong > value2->data[0].v_ulong;
    }
}

static void param_int64_init(XParamSpec *pspec)
{
    XParamSpecInt64 *lspec = X_PARAM_SPEC_INT64(pspec);

    lspec->minimum = X_MININT64;
    lspec->maximum = X_MAXINT64;
    lspec->default_value = 0;
}

static void param_int64_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_int64 = X_PARAM_SPEC_INT64(pspec)->default_value;
}

static xboolean param_int64_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecInt64 *lspec = X_PARAM_SPEC_INT64(pspec);
    xint64 oval = value->data[0].v_int64;

    return lspec->minimum <= oval && oval <= lspec->maximum;
}

static xboolean param_int64_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecInt64 *lspec = X_PARAM_SPEC_INT64(pspec);
    xint64 oval = value->data[0].v_int64;

    value->data[0].v_int64 = CLAMP(value->data[0].v_int64, lspec->minimum, lspec->maximum);
    return value->data[0].v_int64 != oval;
}

static xint param_int64_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_int64 < value2->data[0].v_int64) {
        return -1;
    } else {
        return value1->data[0].v_int64 > value2->data[0].v_int64;
    }
}

static void param_uint64_init(XParamSpec *pspec)
{
    XParamSpecUInt64 *uspec = X_PARAM_SPEC_UINT64(pspec);

    uspec->minimum = 0;
    uspec->maximum = X_MAXUINT64;
    uspec->default_value = 0;
}

static void param_uint64_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_uint64 = X_PARAM_SPEC_UINT64(pspec)->default_value;
}

static xboolean param_uint64_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecUInt64 *uspec = X_PARAM_SPEC_UINT64(pspec);
    xuint64 oval = value->data[0].v_uint64;

    return uspec->minimum <= oval && oval <= uspec->maximum;
}

static xboolean param_uint64_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecUInt64 *uspec = X_PARAM_SPEC_UINT64(pspec);
    xuint64 oval = value->data[0].v_uint64;

    value->data[0].v_uint64 = CLAMP(value->data[0].v_uint64, uspec->minimum, uspec->maximum);
    return value->data[0].v_uint64 != oval;
}

static xint param_uint64_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_uint64 < value2->data[0].v_uint64) {
        return -1;
    } else {
        return value1->data[0].v_uint64 > value2->data[0].v_uint64;
    }
}

static void param_unichar_init(XParamSpec *pspec)
{
    XParamSpecUnichar *uspec = X_PARAM_SPEC_UNICHAR(pspec);
    uspec->default_value = 0;
}

static void param_unichar_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_uint = X_PARAM_SPEC_UNICHAR(pspec)->default_value;
}

static xboolean param_unichar_is_valid(XParamSpec *pspec, const XValue *value)
{
    return x_unichar_validate(value->data[0].v_uint);
}

static xboolean param_unichar_validate(XParamSpec *pspec, XValue *value)
{
    xboolean changed = FALSE;
    xunichar oval = value->data[0].v_uint;

    if (!x_unichar_validate(oval)) {
        value->data[0].v_uint = 0;
        changed = TRUE;
    }

    return changed;
}

static xint param_unichar_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (value1->data[0].v_uint < value2->data[0].v_uint) {
        return -1;
    } else {
        return value1->data[0].v_uint > value2->data[0].v_uint;
    }
}

static void param_enum_init(XParamSpec *pspec)
{
    XParamSpecEnum *espec = X_PARAM_SPEC_ENUM(pspec);

    espec->enum_class = NULL;
    espec->default_value = 0;
}

static void param_enum_finalize(XParamSpec *pspec)
{
    XParamSpecEnum *espec = X_PARAM_SPEC_ENUM(pspec);
    XParamSpecClass *parent_class = (XParamSpecClass *)x_type_class_peek(x_type_parent(X_TYPE_PARAM_ENUM));

    if (espec->enum_class) {
        x_type_class_unref(espec->enum_class);
        espec->enum_class = NULL;
    }

    parent_class->finalize(pspec);
}

static void param_enum_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_long = X_PARAM_SPEC_ENUM(pspec)->default_value;
}

static xboolean param_enum_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecEnum *espec = X_PARAM_SPEC_ENUM(pspec);
    xlong oval = value->data[0].v_long;

    return x_enum_get_value(espec->enum_class, oval) != NULL;
}

static xboolean param_enum_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecEnum *espec = X_PARAM_SPEC_ENUM(pspec);
    xlong oval = value->data[0].v_long;

    if (!espec->enum_class || !x_enum_get_value(espec->enum_class, value->data[0].v_long)) {
        value->data[0].v_long = espec->default_value;
    }

    return value->data[0].v_long != oval;
}

static void param_flags_init(XParamSpec *pspec)
{
    XParamSpecFlags *fspec = X_PARAM_SPEC_FLAGS(pspec);

    fspec->flags_class = NULL;
    fspec->default_value = 0;
}

static void param_flags_finalize(XParamSpec *pspec)
{
    XParamSpecFlags *fspec = X_PARAM_SPEC_FLAGS(pspec);
    XParamSpecClass *parent_class = (XParamSpecClass *)x_type_class_peek(x_type_parent(X_TYPE_PARAM_FLAGS));

    if (fspec->flags_class) {
        x_type_class_unref(fspec->flags_class);
        fspec->flags_class = NULL;
    }

    parent_class->finalize (pspec);
}

static void param_flags_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_ulong = X_PARAM_SPEC_FLAGS(pspec)->default_value;
}

static xboolean param_flags_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecFlags *fspec = X_PARAM_SPEC_FLAGS(pspec);
    xulong oval = value->data[0].v_ulong;

    return (oval & ~fspec->flags_class->mask) == 0;
}

static xboolean param_flags_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecFlags *fspec = X_PARAM_SPEC_FLAGS(pspec);
    xulong oval = value->data[0].v_ulong;

    if (fspec->flags_class) {
        value->data[0].v_ulong &= fspec->flags_class->mask;
    } else {
        value->data[0].v_ulong = fspec->default_value;
    }

    return value->data[0].v_ulong != oval;
}

static void param_float_init(XParamSpec *pspec)
{
    XParamSpecFloat *fspec = X_PARAM_SPEC_FLOAT(pspec);

    fspec->minimum = -X_MAXFLOAT;
    fspec->maximum = X_MAXFLOAT;
    fspec->default_value = 0;
    fspec->epsilon = X_FLOAT_EPSILON;
}

static void param_float_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_float = X_PARAM_SPEC_FLOAT(pspec)->default_value;
}

static xboolean param_float_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecFloat *fspec = X_PARAM_SPEC_FLOAT(pspec);
    xfloat oval = value->data[0].v_float;

    return fspec->minimum <= oval && oval <= fspec->maximum;
}

static xboolean param_float_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecFloat *fspec = X_PARAM_SPEC_FLOAT(pspec);
    xfloat oval = value->data[0].v_float;

    value->data[0].v_float = CLAMP(value->data[0].v_float, fspec->minimum, fspec->maximum);
    return value->data[0].v_float != oval;
}

static xint param_float_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    xfloat epsilon = X_PARAM_SPEC_FLOAT(pspec)->epsilon;

    if (value1->data[0].v_float < value2->data[0].v_float) {
        return -(value2->data[0].v_float - value1->data[0].v_float > epsilon);
    } else {
        return value1->data[0].v_float - value2->data[0].v_float > epsilon;
    }
}

static void param_double_init(XParamSpec *pspec)
{
    XParamSpecDouble *dspec = X_PARAM_SPEC_DOUBLE(pspec);

    dspec->minimum = -X_MAXDOUBLE;
    dspec->maximum = X_MAXDOUBLE;
    dspec->default_value = 0;
    dspec->epsilon = X_DOUBLE_EPSILON;
}

static void param_double_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_double = X_PARAM_SPEC_DOUBLE(pspec)->default_value;
}

static xboolean param_double_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecDouble *dspec = X_PARAM_SPEC_DOUBLE(pspec);
    xfloat oval = value->data[0].v_double;

    return dspec->minimum <= oval && oval <= dspec->maximum;
}

static xboolean param_double_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecDouble *dspec = X_PARAM_SPEC_DOUBLE(pspec);
    xdouble oval = value->data[0].v_double;

    value->data[0].v_double = CLAMP(value->data[0].v_double, dspec->minimum, dspec->maximum);
    return value->data[0].v_double != oval;
}

static xint param_double_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    xdouble epsilon = X_PARAM_SPEC_DOUBLE(pspec)->epsilon;

    if (value1->data[0].v_double < value2->data[0].v_double) {
        return - (value2->data[0].v_double - value1->data[0].v_double > epsilon);
    } else {
        return value1->data[0].v_double - value2->data[0].v_double > epsilon;
    }
}

static void param_string_init(XParamSpec *pspec)
{
    XParamSpecString *sspec = X_PARAM_SPEC_STRING(pspec);

    sspec->default_value = NULL;
    sspec->cset_first = NULL;
    sspec->cset_nth = NULL;
    sspec->substitutor = '_';
    sspec->null_fold_if_empty = FALSE;
    sspec->ensure_non_null = FALSE;
}

static void param_string_finalize(XParamSpec *pspec)
{
    XParamSpecString *sspec = X_PARAM_SPEC_STRING(pspec);
    XParamSpecClass *parent_class = (XParamSpecClass *)x_type_class_peek(x_type_parent(X_TYPE_PARAM_STRING));

    x_free(sspec->default_value);
    x_free(sspec->cset_first);
    x_free(sspec->cset_nth);
    sspec->default_value = NULL;
    sspec->cset_first = NULL;
    sspec->cset_nth = NULL;

    parent_class->finalize(pspec);
}

static void param_string_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_pointer = x_strdup(X_PARAM_SPEC_STRING(pspec)->default_value);
}

static xboolean param_string_validate(XParamSpec *pspec, XValue *value)
{
    xuint changed = 0;
    XParamSpecString *sspec = X_PARAM_SPEC_STRING(pspec);
    xchar *string = (xchar *)value->data[0].v_pointer;

    if (string && string[0]){
        xchar *s;

        if (sspec->cset_first && !strchr(sspec->cset_first, string[0])) {
            if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) {
                value->data[0].v_pointer = x_strdup(string);
                string = (xchar *)value->data[0].v_pointer;
                value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
            }

            string[0] = sspec->substitutor;
            changed++;
        }

        if (sspec->cset_nth) {
            for (s = string + 1; *s; s++) {
                if (!strchr(sspec->cset_nth, *s)) {
                    if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) {
                        value->data[0].v_pointer = x_strdup(string);
                        s = (xchar *)value->data[0].v_pointer + (s - string);
                        string = (xchar *)value->data[0].v_pointer;
                        value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
                    }

                    *s = sspec->substitutor;
                    changed++;
                }
            }
        }
    }

    if (sspec->null_fold_if_empty && string && string[0] == 0) {
        if (!(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)) {
            x_free(value->data[0].v_pointer);
        } else {
            value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
        }

        value->data[0].v_pointer = NULL;
        changed++;
        string = (xchar *)value->data[0].v_pointer;
    }

    if (sspec->ensure_non_null && !string) {
        value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
        value->data[0].v_pointer = x_strdup("");
        changed++;
        string = (xchar *)value->data[0].v_pointer;
    }

    return changed;
}

static xboolean param_string_is_valid(XParamSpec *pspec, const XValue *value)
{
    xboolean ret = TRUE;
    XParamSpecString *sspec = X_PARAM_SPEC_STRING(pspec);

    if (sspec->cset_first != NULL || sspec->cset_nth != NULL || sspec->ensure_non_null || sspec->null_fold_if_empty) {
        XValue tmp_value = X_VALUE_INIT;

        x_value_init(&tmp_value, X_VALUE_TYPE(value));
        x_value_copy(value, &tmp_value);

        ret = !param_string_validate(pspec, &tmp_value);
        x_value_unset(&tmp_value);
    }

    return ret;
}

static xint param_string_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    if (!value1->data[0].v_pointer) {
        return value2->data[0].v_pointer != NULL ? -1 : 0;
    } else if (!value2->data[0].v_pointer) {
        return value1->data[0].v_pointer != NULL;
    } else {
        return strcmp((const char *)value1->data[0].v_pointer, (const char *)value2->data[0].v_pointer);
    }
}

static void param_param_init(XParamSpec *pspec)
{

}

static void param_param_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static xboolean param_param_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpec *param = (XParamSpec *)value->data[0].v_pointer;
    if (param == NULL) {
        return FALSE;
    }

    return x_value_type_compatible(X_PARAM_SPEC_TYPE(param), X_PARAM_SPEC_VALUE_TYPE(pspec));
}

static xboolean param_param_validate(XParamSpec *pspec, XValue *value)
{
    xuint changed = 0;
    XParamSpec *param = (XParamSpec *)value->data[0].v_pointer;

    if (param && !x_value_type_compatible(X_PARAM_SPEC_TYPE(param), X_PARAM_SPEC_VALUE_TYPE(pspec))) {
        x_param_spec_unref(param);
        value->data[0].v_pointer = NULL;
        changed++;
    }

    return changed;
}

static void param_boxed_init(XParamSpec *pspec)
{

}

static void param_boxed_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static xint param_boxed_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    xuint8 *p1 = (xuint8 *)value1->data[0].v_pointer;
    xuint8 *p2 = (xuint8 *)value2->data[0].v_pointer;

    return p1 < p2 ? -1 : p1 > p2;
}

static void param_pointer_init(XParamSpec *pspec)
{

}

static void param_pointer_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static xint param_pointer_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    xuint8 *p1 = (xuint8 *)value1->data[0].v_pointer;
    xuint8 *p2 = (xuint8 *)value2->data[0].v_pointer;

    return p1 < p2 ? -1 : p1 > p2;
}

static void param_value_array_init(XParamSpec *pspec)
{
    XParamSpecValueArray *aspec = X_PARAM_SPEC_VALUE_ARRAY(pspec);

    aspec->element_spec = NULL;
    aspec->fixed_n_elements = 0;
}

static inline xuint value_array_ensure_size(XValueArray *value_array, xuint fixed_n_elements)
{
    xuint changed = 0;

    if (fixed_n_elements) {
        while (value_array->n_values < fixed_n_elements) {
            x_value_array_append(value_array, NULL);
            changed++;
        }

        while (value_array->n_values > fixed_n_elements) {
            x_value_array_remove(value_array, value_array->n_values - 1);
            changed++;
        }
    }

    return changed;
}

static void param_value_array_finalize(XParamSpec *pspec)
{
    XParamSpecValueArray *aspec = X_PARAM_SPEC_VALUE_ARRAY(pspec);
    XParamSpecClass *parent_class = (XParamSpecClass *)x_type_class_peek(x_type_parent(X_TYPE_PARAM_VALUE_ARRAY));

    if (aspec->element_spec) {
        x_param_spec_unref(aspec->element_spec);
        aspec->element_spec = NULL;
    }

    parent_class->finalize (pspec);
}

static void param_value_array_set_default(XParamSpec *pspec, XValue *value)
{
    XParamSpecValueArray *aspec = X_PARAM_SPEC_VALUE_ARRAY(pspec);

    if (!value->data[0].v_pointer && aspec->fixed_n_elements) {
        value->data[0].v_pointer = x_value_array_new(aspec->fixed_n_elements);
    }

    if (value->data[0].v_pointer) {
        value_array_ensure_size((XValueArray *)value->data[0].v_pointer, aspec->fixed_n_elements);
    }
}

static xboolean param_value_array_validate(XParamSpec *pspec, XValue *value)
{
    xuint changed = 0;
    XParamSpecValueArray *aspec = X_PARAM_SPEC_VALUE_ARRAY(pspec);
    XValueArray *value_array = (XValueArray *)value->data[0].v_pointer;

    if (!value->data[0].v_pointer && aspec->fixed_n_elements) {
        value_array = value->data[0].v_pointer = x_value_array_new(aspec->fixed_n_elements);
    }

    if (value->data[0].v_pointer) {
        changed += value_array_ensure_size(value_array, aspec->fixed_n_elements);
        if (aspec->element_spec) {
            xuint i;
            XParamSpec *element_spec = aspec->element_spec;

            for (i = 0; i < value_array->n_values; i++) {
                XValue *element = value_array->values + i;

                if (!x_value_type_compatible(X_VALUE_TYPE(element), X_PARAM_SPEC_VALUE_TYPE(element_spec))) {
                    if (X_VALUE_TYPE(element) != 0) {
                        x_value_unset(element);
                    }

                    x_value_init(element, X_PARAM_SPEC_VALUE_TYPE(element_spec));
                    x_param_value_set_default(element_spec, element);
                    changed++;
                } else {
                    changed += x_param_value_validate(element_spec, element);
                }
            }
        }
    }

    return changed;
}

static xint param_value_array_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    XParamSpecValueArray *aspec = X_PARAM_SPEC_VALUE_ARRAY(pspec);
    XValueArray *value_array1 = (XValueArray *)value1->data[0].v_pointer;
    XValueArray *value_array2 = (XValueArray *)value2->data[0].v_pointer;

    if (!value_array1 || !value_array2) {
        return value_array2 ? -1 : value_array1 != value_array2;
    }

    if (value_array1->n_values != value_array2->n_values) {
        return value_array1->n_values < value_array2->n_values ? -1 : 1;
    } else if (!aspec->element_spec) {
        return value_array1->n_values < value_array2->n_values ? -1 : value_array1->n_values > value_array2->n_values;
    } else {
        xuint i;

        for (i = 0; i < value_array1->n_values; i++) {
            xint cmp;
            XValue *element1 = value_array1->values + i;
            XValue *element2 = value_array2->values + i;

            if (X_VALUE_TYPE(element1) != X_VALUE_TYPE(element2)) {
                return X_VALUE_TYPE(element1) < X_VALUE_TYPE(element2) ? -1 : 1;
            }

            cmp = x_param_values_cmp(aspec->element_spec, element1, element2);
            if (cmp) {
                return cmp;
            }
        }

        return 0;
    }
}

static void param_object_init(XParamSpec *pspec)
{

}

static void param_object_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_pointer = NULL;
}

static xboolean param_object_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecObject *ospec = X_PARAM_SPEC_OBJECT(pspec);
    XObject *object = (XObject *)value->data[0].v_pointer;

    return object && x_value_type_compatible(X_OBJECT_TYPE(object), X_PARAM_SPEC_VALUE_TYPE(ospec));
}

static xboolean param_object_validate(XParamSpec *pspec, XValue *value)
{
    xuint changed = 0;
    XParamSpecObject *ospec = X_PARAM_SPEC_OBJECT(pspec);
    XObject *object = (XObject *)value->data[0].v_pointer;

    if (object && !x_value_type_compatible(X_OBJECT_TYPE(object), X_PARAM_SPEC_VALUE_TYPE(ospec))) {
        x_object_unref(object);
        value->data[0].v_pointer = NULL;
        changed++;
    }

    return changed;
}

static xint param_object_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    xuint8 *p1 = (xuint8 *)value1->data[0].v_pointer;
    xuint8 *p2 = (xuint8 *)value2->data[0].v_pointer;

    return p1 < p2 ? -1 : p1 > p2;
}

static void param_override_init(XParamSpec *pspec)
{

}

static void param_override_finalize(XParamSpec *pspec)
{
    XParamSpecOverride *ospec = X_PARAM_SPEC_OVERRIDE(pspec);
    XParamSpecClass *parent_class = (XParamSpecClass *)x_type_class_peek(x_type_parent(X_TYPE_PARAM_OVERRIDE));

    if (ospec->overridden) {
        x_param_spec_unref(ospec->overridden);
        ospec->overridden = NULL;
    }

    parent_class->finalize (pspec);
}

static void param_override_set_default(XParamSpec *pspec, XValue *value)
{
    XParamSpecOverride *ospec = X_PARAM_SPEC_OVERRIDE(pspec);
    x_param_value_set_default(ospec->overridden, value);
}

static xboolean param_override_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecOverride *ospec = X_PARAM_SPEC_OVERRIDE(pspec);
    return x_param_value_is_valid(ospec->overridden, value);
}

static xboolean param_override_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecOverride *ospec = X_PARAM_SPEC_OVERRIDE(pspec);
    return x_param_value_validate(ospec->overridden, value);
}

static xint param_override_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    XParamSpecOverride *ospec = X_PARAM_SPEC_OVERRIDE(pspec);
    return x_param_values_cmp(ospec->overridden, value1, value2);
}

static void param_gtype_init(XParamSpec *pspec)
{

}

static void param_gtype_set_default(XParamSpec *pspec, XValue *value)
{
    XParamSpecGType *tspec = X_PARAM_SPEC_GTYPE(pspec);
    value->data[0].v_pointer = XTYPE_TO_POINTER(tspec->is_a_type);
}

static xboolean param_gtype_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecGType *tspec = X_PARAM_SPEC_GTYPE(pspec);
    XType gtype = XPOINTER_TO_TYPE(value->data[0].v_pointer);

    return tspec->is_a_type == X_TYPE_NONE || x_type_is_a(gtype, tspec->is_a_type);
}

static xboolean param_gtype_validate(XParamSpec *pspec, XValue *value)
{
    xuint changed = 0;
    XParamSpecGType *tspec = X_PARAM_SPEC_GTYPE(pspec);
    XType gtype = XPOINTER_TO_TYPE(value->data[0].v_pointer);

    if (tspec->is_a_type != X_TYPE_NONE && !x_type_is_a(gtype, tspec->is_a_type)) {
        value->data[0].v_pointer = XTYPE_TO_POINTER(tspec->is_a_type);
        changed++;
    }

    return changed;
}

static xint param_gtype_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    XType p1 = XPOINTER_TO_TYPE(value1->data[0].v_pointer);
    XType p2 = XPOINTER_TO_TYPE(value2->data[0].v_pointer);

    return p1 < p2 ? -1 : p1 > p2;
}

static void param_variant_init(XParamSpec *pspec)
{
    XParamSpecVariant *vspec = (XParamSpecVariant *)X_PARAM_SPEC_VARIANT(pspec);

    vspec->type = NULL;
    vspec->default_value = NULL;
}

static void param_variant_finalize(XParamSpec *pspec)
{
    XParamSpecVariant *vspec = X_PARAM_SPEC_VARIANT(pspec);
    XParamSpecClass *parent_class = (XParamSpecClass *)x_type_class_peek(x_type_parent(X_TYPE_PARAM_VARIANT));

    if (vspec->default_value) {
        x_variant_unref(vspec->default_value);
    }
    x_variant_type_free(vspec->type);

    parent_class->finalize(pspec);
}

static void param_variant_set_default(XParamSpec *pspec, XValue *value)
{
    value->data[0].v_pointer = X_PARAM_SPEC_VARIANT(pspec)->default_value;
    value->data[1].v_uint |= X_VALUE_NOCOPY_CONTENTS;
}

static xboolean param_variant_is_valid(XParamSpec *pspec, const XValue *value)
{
    XParamSpecVariant *vspec = X_PARAM_SPEC_VARIANT(pspec);
    XVariant *variant = (XVariant *)value->data[0].v_pointer;

    if (variant == NULL) {
        return vspec->default_value == NULL;
    } else {
        return x_variant_is_of_type(variant, vspec->type);
    }
}

static xboolean param_variant_validate(XParamSpec *pspec, XValue *value)
{
    XParamSpecVariant *vspec = X_PARAM_SPEC_VARIANT(pspec);
    XVariant *variant = (XVariant *)value->data[0].v_pointer;

    if ((variant == NULL && vspec->default_value != NULL) || (variant != NULL && !x_variant_is_of_type(variant, vspec->type))) {
        x_param_value_set_default(pspec, value);
        return TRUE;
    }

    return FALSE;
}

static xboolean variant_is_incomparable(XVariant *v)
{
    XVariantClass v_class = x_variant_classify(v);

    return (v_class == X_VARIANT_CLASS_HANDLE ||
            v_class == X_VARIANT_CLASS_VARIANT ||
            v_class ==  X_VARIANT_CLASS_MAYBE||
            v_class == X_VARIANT_CLASS_ARRAY ||
            v_class ==  X_VARIANT_CLASS_TUPLE ||
            v_class == X_VARIANT_CLASS_DICT_ENTRY);
}

static xint param_variant_values_cmp(XParamSpec *pspec, const XValue *value1, const XValue *value2)
{
    XVariant *v1 = (XVariant *)value1->data[0].v_pointer;
    XVariant *v2 = (XVariant *)value2->data[0].v_pointer;

    if (v1 == NULL && v2 == NULL) {
        return 0;
    } else if (v1 == NULL && v2 != NULL) {
        return -1;
    } else if (v1 != NULL && v2 == NULL) {
        return 1;
    }

    if (!x_variant_type_equal(x_variant_get_type(v1), x_variant_get_type(v2)) || variant_is_incomparable(v1) || variant_is_incomparable(v2)) {
        return x_variant_equal(v1, v2) ? 0 : (v1 < v2 ? -1 : 1);
    }

    return x_variant_compare(v1, v2);
}

#define set_is_valid_vfunc(type, func)                                          \
    {                                                                           \
        XParamSpecClass *classt = (XParamSpecClass *)x_type_class_ref(type);    \
        classt->value_is_valid = func;                                          \
        x_type_class_unref(classt);                                             \
    }

XType *x_param_spec_types = NULL;

void _x_param_spec_types_init(void)
{
    XType type, *spec_types;
    XType *spec_types_bound;
    const xuint n_types = 23;

    x_param_spec_types = x_new0(XType, n_types);
    spec_types = x_param_spec_types;
    spec_types_bound = x_param_spec_types + n_types;

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecChar),
            16,
            param_char_init,
            X_TYPE_CHAR,
            NULL,
            param_char_set_default,
            param_char_validate,
            param_int_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamChar"), &pspec_info);
        set_is_valid_vfunc(type, param_char_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_CHAR);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecUChar),
            16,
            param_uchar_init,
            X_TYPE_UCHAR,
            NULL,
            param_uchar_set_default,
            param_uchar_validate,
            param_uint_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamUChar"), &pspec_info);
        set_is_valid_vfunc(type, param_uchar_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_UCHAR);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecBoolean),
            16,
            NULL,
            X_TYPE_BOOLEAN,
            NULL,
            param_boolean_set_default,
            param_boolean_validate,
            param_int_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamBoolean"), &pspec_info);
        set_is_valid_vfunc(type, param_boolean_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_BOOLEAN);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecInt),
            16,
            param_int_init,
            X_TYPE_INT,
            NULL,
            param_int_set_default,
            param_int_validate,
            param_int_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamInt"), &pspec_info);
        set_is_valid_vfunc(type, param_int_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_INT);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecUInt),
            16,
            param_uint_init,
            X_TYPE_UINT,
            NULL,
            param_uint_set_default,
            param_uint_validate,
            param_uint_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamUInt"), &pspec_info);
        set_is_valid_vfunc(type, param_uint_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_UINT);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecLong),
            16,
            param_long_init,
            X_TYPE_LONG,
            NULL,
            param_long_set_default,
            param_long_validate,
            param_long_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamLong"), &pspec_info);
        set_is_valid_vfunc(type, param_long_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_LONG);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecULong),
            16,
            param_ulong_init,
            X_TYPE_ULONG,
            NULL,
            param_ulong_set_default,
            param_ulong_validate,
            param_ulong_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamULong"), &pspec_info);
        set_is_valid_vfunc(type, param_ulong_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_ULONG);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecInt64),
            16,
            param_int64_init,
            X_TYPE_INT64,
            NULL,
            param_int64_set_default,
            param_int64_validate,
            param_int64_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamInt64"), &pspec_info);
        set_is_valid_vfunc(type, param_int64_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_INT64);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecUInt64),
            16,
            param_uint64_init,
            X_TYPE_UINT64,
            NULL,
            param_uint64_set_default,
            param_uint64_validate,
            param_uint64_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamUInt64"), &pspec_info);
        set_is_valid_vfunc(type, param_uint64_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_UINT64);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecUnichar),
            16,
            param_unichar_init,
            X_TYPE_UINT,
            NULL,
            param_unichar_set_default,
            param_unichar_validate,
            param_unichar_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamUnichar"), &pspec_info);
        set_is_valid_vfunc(type, param_unichar_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_UNICHAR);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecEnum),
            16,
            param_enum_init,
            X_TYPE_ENUM,
            param_enum_finalize,
            param_enum_set_default,
            param_enum_validate,
            param_long_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamEnum"), &pspec_info);
        set_is_valid_vfunc(type, param_enum_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_ENUM);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecFlags),
            16,
            param_flags_init,
            X_TYPE_FLAGS,
            param_flags_finalize,
            param_flags_set_default,
            param_flags_validate,
            param_ulong_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamFlags"), &pspec_info);
        set_is_valid_vfunc(type, param_flags_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_FLAGS);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecFloat),
            16,
            param_float_init,
            X_TYPE_FLOAT,
            NULL,
            param_float_set_default,
            param_float_validate,
            param_float_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamFloat"), &pspec_info);
        set_is_valid_vfunc(type, param_float_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_FLOAT);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecDouble),
            16,
            param_double_init,
            X_TYPE_DOUBLE,
            NULL,
            param_double_set_default,
            param_double_validate,
            param_double_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamDouble"), &pspec_info);
        set_is_valid_vfunc(type, param_double_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_DOUBLE);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecString),
            16,
            param_string_init,
            X_TYPE_STRING,
            param_string_finalize,
            param_string_set_default,
            param_string_validate,
            param_string_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamString"), &pspec_info);
        set_is_valid_vfunc(type, param_string_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_STRING);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecParam),
            16,
            param_param_init,
            X_TYPE_PARAM,
            NULL,
            param_param_set_default,
            param_param_validate,
            param_pointer_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamParam"), &pspec_info);
        set_is_valid_vfunc(type, param_param_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_PARAM);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecBoxed),
            4,
            param_boxed_init,
            X_TYPE_BOXED,
            NULL,
            param_boxed_set_default,
            NULL,
            param_boxed_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamBoxed"), &pspec_info);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_BOXED);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecPointer),
            0,
            param_pointer_init,
            X_TYPE_POINTER,
            NULL,
            param_pointer_set_default,
            NULL,
            param_pointer_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamPointer"), &pspec_info);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_POINTER);
    }

    {
        XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecValueArray),
            0,
            param_value_array_init,
            0xdeadbeef,
            param_value_array_finalize,
            param_value_array_set_default,
            param_value_array_validate,
            param_value_array_values_cmp,
        };

        pspec_info.value_type = X_TYPE_VALUE_ARRAY;
        type = x_param_type_register_static(x_intern_static_string("XParamValueArray"), &pspec_info);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_VALUE_ARRAY);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecObject),
            16,
            param_object_init,
            X_TYPE_OBJECT,
            NULL,
            param_object_set_default,
            param_object_validate,
            param_object_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamObject"), &pspec_info);
        set_is_valid_vfunc(type, param_object_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_OBJECT);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecOverride),
            16,
            param_override_init,
            X_TYPE_NONE,
            param_override_finalize,
            param_override_set_default,
            param_override_validate,
            param_override_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamOverride"), &pspec_info);
        set_is_valid_vfunc(type, param_override_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_OVERRIDE);
    }

    {
        XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecGType),
            0,
            param_gtype_init,
            0xdeadbeef,
            NULL,
            param_gtype_set_default,
            param_gtype_validate,
            param_gtype_values_cmp,
        };

        pspec_info.value_type = X_TYPE_GTYPE;
        type = x_param_type_register_static(x_intern_static_string("XParamGType"), &pspec_info);
        set_is_valid_vfunc(type, param_gtype_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_GTYPE);
    }

    {
        const XParamSpecTypeInfo pspec_info = {
            sizeof(XParamSpecVariant),
            0,
            param_variant_init,
            X_TYPE_VARIANT,
            param_variant_finalize, 
            param_variant_set_default,
            param_variant_validate,
            param_variant_values_cmp,
        };

        type = x_param_type_register_static(x_intern_static_string("XParamVariant"), &pspec_info);
        set_is_valid_vfunc(type, param_variant_is_valid);
        *spec_types++ = type;
        x_assert(type == X_TYPE_PARAM_VARIANT);
    }

    x_assert(spec_types == spec_types_bound);
}

XParamSpec *x_param_spec_char(const xchar *name, const xchar *nick, const xchar *blurb, xint8 minimum, xint8 maximum, xint8 default_value, XParamFlags flags)
{
    XParamSpecChar *cspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    cspec = (XParamSpecChar *)x_param_spec_internal(X_TYPE_PARAM_CHAR, name, nick, blurb, flags);
    cspec->minimum = minimum;
    cspec->maximum = maximum;
    cspec->default_value = default_value;

    return X_PARAM_SPEC(cspec);
}

XParamSpec *x_param_spec_uchar(const xchar *name, const xchar *nick, const xchar *blurb, xuint8 minimum, xuint8 maximum, xuint8 default_value, XParamFlags flags)
{
    XParamSpecUChar *uspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    uspec = (XParamSpecUChar *)x_param_spec_internal(X_TYPE_PARAM_UCHAR, name, nick, blurb, flags);
    uspec->minimum = minimum;
    uspec->maximum = maximum;
    uspec->default_value = default_value;

    return X_PARAM_SPEC(uspec);
}

XParamSpec *x_param_spec_boolean(const xchar *name, const xchar *nick, const xchar *blurb, xboolean default_value, XParamFlags flags)
{
    XParamSpecBoolean *bspec;

    x_return_val_if_fail(default_value == TRUE || default_value == FALSE, NULL);

    bspec = (XParamSpecBoolean *)x_param_spec_internal(X_TYPE_PARAM_BOOLEAN, name, nick, blurb, flags);
    bspec->default_value = default_value;

    return X_PARAM_SPEC(bspec);
}

XParamSpec *x_param_spec_int(const xchar *name, const xchar *nick, const xchar *blurb, xint minimum, xint maximum, xint default_value, XParamFlags flags)
{
    XParamSpecInt *ispec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    ispec = (XParamSpecInt *)x_param_spec_internal(X_TYPE_PARAM_INT, name, nick, blurb, flags);
    ispec->minimum = minimum;
    ispec->maximum = maximum;
    ispec->default_value = default_value;

    return X_PARAM_SPEC(ispec);
}

XParamSpec *x_param_spec_uint(const xchar *name, const xchar *nick, const xchar *blurb, xuint minimum, xuint maximum, xuint default_value, XParamFlags flags)
{
    XParamSpecUInt *uspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    uspec = (XParamSpecUInt *)x_param_spec_internal(X_TYPE_PARAM_UINT, name, nick, blurb, flags);
    uspec->minimum = minimum;
    uspec->maximum = maximum;
    uspec->default_value = default_value;

    return X_PARAM_SPEC(uspec);
}

XParamSpec *x_param_spec_long(const xchar *name, const xchar *nick, const xchar *blurb, xlong minimum, xlong maximum, xlong default_value, XParamFlags flags)
{
    XParamSpecLong *lspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    lspec = (XParamSpecLong *)x_param_spec_internal(X_TYPE_PARAM_LONG, name, nick, blurb, flags);
    lspec->minimum = minimum;
    lspec->maximum = maximum;
    lspec->default_value = default_value;

    return X_PARAM_SPEC(lspec);
}

XParamSpec *x_param_spec_ulong(const xchar *name, const xchar *nick, const xchar *blurb, xulong minimum, xulong maximum, xulong default_value, XParamFlags flags)
{
    XParamSpecULong *uspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    uspec = (XParamSpecULong *)x_param_spec_internal(X_TYPE_PARAM_ULONG, name, nick, blurb, flags);
    uspec->minimum = minimum;
    uspec->maximum = maximum;
    uspec->default_value = default_value;

    return X_PARAM_SPEC(uspec);
}

XParamSpec *x_param_spec_int64(const xchar *name, const xchar *nick, const xchar *blurb, xint64 minimum, xint64 maximum, xint64 default_value, XParamFlags flags)
{
    XParamSpecInt64 *lspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    lspec = (XParamSpecInt64 *)x_param_spec_internal(X_TYPE_PARAM_INT64, name, nick, blurb, flags);
    lspec->minimum = minimum;
    lspec->maximum = maximum;
    lspec->default_value = default_value;

    return X_PARAM_SPEC(lspec);
}

XParamSpec *x_param_spec_uint64(const xchar *name, const xchar *nick, const xchar *blurb, xuint64 minimum, xuint64 maximum, xuint64 default_value, XParamFlags flags)
{
    XParamSpecUInt64 *uspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    uspec = (XParamSpecUInt64 *)x_param_spec_internal(X_TYPE_PARAM_UINT64, name, nick, blurb, flags);
    uspec->minimum = minimum;
    uspec->maximum = maximum;
    uspec->default_value = default_value;

    return X_PARAM_SPEC(uspec);
}

XParamSpec *x_param_spec_unichar(const xchar *name, const xchar *nick, const xchar *blurb, xunichar default_value, XParamFlags flags)
{
    XParamSpecUnichar *uspec;

    uspec = (XParamSpecUnichar *)x_param_spec_internal(X_TYPE_PARAM_UNICHAR, name, nick, blurb, flags);
    uspec->default_value = default_value;

    return X_PARAM_SPEC(uspec);
}

XParamSpec *x_param_spec_enum(const xchar *name, const xchar *nick, const xchar *blurb, XType enum_type, xint default_value, XParamFlags flags)
{
    XParamSpecEnum *espec;
    XEnumClass *enum_class;

    x_return_val_if_fail(X_TYPE_IS_ENUM(enum_type), NULL);
    enum_class = (XEnumClass *)x_type_class_ref(enum_type);
    x_return_val_if_fail(x_enum_get_value(enum_class, default_value) != NULL, NULL);

    espec = (XParamSpecEnum *)x_param_spec_internal(X_TYPE_PARAM_ENUM, name, nick, blurb, flags);
    espec->enum_class = enum_class;
    espec->default_value = default_value;
    X_PARAM_SPEC(espec)->value_type = enum_type;

    return X_PARAM_SPEC(espec);
}

XParamSpec *x_param_spec_flags(const xchar *name, const xchar *nick, const xchar *blurb, XType flags_type, xuint default_value, XParamFlags flags)
{
    XParamSpecFlags *fspec;
    XFlagsClass *flags_class;

    x_return_val_if_fail(X_TYPE_IS_FLAGS(flags_type), NULL);

    flags_class = (XFlagsClass *)x_type_class_ref(flags_type);
    x_return_val_if_fail((default_value & flags_class->mask) == default_value, NULL);

    fspec = (XParamSpecFlags *)x_param_spec_internal(X_TYPE_PARAM_FLAGS, name, nick, blurb, flags);
    fspec->flags_class = flags_class;
    fspec->default_value = default_value;
    X_PARAM_SPEC(fspec)->value_type = flags_type;

    return X_PARAM_SPEC(fspec);
}

XParamSpec *x_param_spec_float(const xchar *name, const xchar *nick, const xchar *blurb, xfloat minimum, xfloat maximum, xfloat default_value, XParamFlags flags)
{
    XParamSpecFloat *fspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    fspec = (XParamSpecFloat *)x_param_spec_internal(X_TYPE_PARAM_FLOAT, name, nick, blurb, flags);
    fspec->minimum = minimum;
    fspec->maximum = maximum;
    fspec->default_value = default_value;

    return X_PARAM_SPEC(fspec);
}

XParamSpec *x_param_spec_double(const xchar *name, const xchar *nick, const xchar *blurb, xdouble minimum, xdouble maximum, xdouble default_value, XParamFlags flags)
{
    XParamSpecDouble *dspec;

    x_return_val_if_fail(default_value >= minimum && default_value <= maximum, NULL);

    dspec = (XParamSpecDouble *)x_param_spec_internal(X_TYPE_PARAM_DOUBLE, name, nick, blurb, flags);
    dspec->minimum = minimum;
    dspec->maximum = maximum;
    dspec->default_value = default_value;

    return X_PARAM_SPEC(dspec);
}

XParamSpec *x_param_spec_string(const xchar *name, const xchar *nick, const xchar *blurb, const xchar *default_value, XParamFlags flags)
{
    XParamSpecString *sspec = (XParamSpecString *)x_param_spec_internal(X_TYPE_PARAM_STRING, name, nick, blurb, flags);

    x_free(sspec->default_value);
    sspec->default_value = x_strdup(default_value);

    return X_PARAM_SPEC(sspec);
}

XParamSpec *x_param_spec_param (const xchar *name, const xchar *nick, const xchar *blurb, XType param_type, XParamFlags flags)
{
    XParamSpecParam *pspec;

    x_return_val_if_fail(X_TYPE_IS_PARAM(param_type), NULL);
    pspec = (XParamSpecParam *)x_param_spec_internal(X_TYPE_PARAM_PARAM, name, nick, blurb, flags);
    X_PARAM_SPEC(pspec)->value_type = param_type;

    return X_PARAM_SPEC(pspec);
}

XParamSpec *x_param_spec_boxed(const xchar *name, const xchar *nick, const xchar *blurb, XType boxed_type, XParamFlags flags)
{
    XParamSpecBoxed *bspec;

    x_return_val_if_fail(X_TYPE_IS_BOXED(boxed_type), NULL);
    x_return_val_if_fail(X_TYPE_IS_VALUE_TYPE(boxed_type), NULL);

    bspec = (XParamSpecBoxed *)x_param_spec_internal(X_TYPE_PARAM_BOXED, name, nick, blurb, flags);
    X_PARAM_SPEC(bspec)->value_type = boxed_type;

    return X_PARAM_SPEC(bspec);
}

XParamSpec *x_param_spec_pointer(const xchar *name, const xchar *nick, const xchar *blurb, XParamFlags flags)
{
    XParamSpecPointer *pspec;

    pspec = (XParamSpecPointer *)x_param_spec_internal(X_TYPE_PARAM_POINTER, name, nick, blurb, flags);
    return X_PARAM_SPEC(pspec);
}

XParamSpec *x_param_spec_gtype(const xchar *name, const xchar *nick, const xchar *blurb, XType is_a_type, XParamFlags flags)
{
    XParamSpecGType *tspec;

    tspec = (XParamSpecGType *)x_param_spec_internal(X_TYPE_PARAM_GTYPE, name, nick, blurb, flags);
    tspec->is_a_type = is_a_type;

    return X_PARAM_SPEC(tspec);
}

XParamSpec *x_param_spec_value_array(const xchar *name, const xchar *nick, const xchar *blurb, XParamSpec *element_spec, XParamFlags flags)
{
    XParamSpecValueArray *aspec;

    x_return_val_if_fail(element_spec == NULL || X_IS_PARAM_SPEC(element_spec), NULL);

    aspec = (XParamSpecValueArray *)x_param_spec_internal(X_TYPE_PARAM_VALUE_ARRAY, name, nick, blurb, flags);
    if (element_spec) {
        aspec->element_spec = x_param_spec_ref(element_spec);
        x_param_spec_sink(element_spec);
    }

    return X_PARAM_SPEC(aspec);
}

XParamSpec *x_param_spec_object(const xchar *name, const xchar *nick, const xchar *blurb, XType object_type, XParamFlags flags)
{
    XParamSpecObject *ospec;

    x_return_val_if_fail(x_type_is_a(object_type, X_TYPE_OBJECT), NULL);

    ospec = (XParamSpecObject *)x_param_spec_internal(X_TYPE_PARAM_OBJECT, name, nick, blurb, flags);
    X_PARAM_SPEC(ospec)->value_type = object_type;

    return X_PARAM_SPEC(ospec);
}

XParamSpec *x_param_spec_override(const xchar *name, XParamSpec *overridden)
{
    XParamSpec *pspec;

    x_return_val_if_fail(name != NULL, NULL);
    x_return_val_if_fail(X_IS_PARAM_SPEC(overridden), NULL);

    while (TRUE) {
        XParamSpec *indirect = x_param_spec_get_redirect_target(overridden);
        if (indirect) {
            overridden = indirect;
        } else {
            break;
        }
    }

    pspec = (XParamSpec *)x_param_spec_internal(X_TYPE_PARAM_OVERRIDE, name, NULL, NULL, overridden->flags);
    pspec->value_type = X_PARAM_SPEC_VALUE_TYPE(overridden);
    X_PARAM_SPEC_OVERRIDE(pspec)->overridden = x_param_spec_ref(overridden);

    return pspec;
}

XParamSpec *x_param_spec_variant(const xchar *name, const xchar *nick, const xchar *blurb, const XVariantType *type, XVariant *default_value, XParamFlags flags)
{
    XParamSpecVariant *vspec;

    x_return_val_if_fail(type != NULL, NULL);
    x_return_val_if_fail(default_value == NULL || x_variant_is_of_type(default_value, type), NULL);

    vspec = (XParamSpecVariant *)x_param_spec_internal(X_TYPE_PARAM_VARIANT, name, nick, blurb, flags);
    vspec->type = x_variant_type_copy(type);
    if (default_value) {
        vspec->default_value = x_variant_ref_sink(default_value);
    }

    return X_PARAM_SPEC(vspec);
}
