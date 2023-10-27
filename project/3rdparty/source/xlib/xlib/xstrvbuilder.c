#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xarray.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xstrvbuilder.h>

struct _XStrvBuilder {
    XPtrArray array;
};

XStrvBuilder *x_strv_builder_new(void)
{
    return (XStrvBuilder *)x_ptr_array_new_with_free_func(x_free);
}

void x_strv_builder_unref(XStrvBuilder *builder)
{
    x_ptr_array_unref(&builder->array);
}

XStrvBuilder *x_strv_builder_ref(XStrvBuilder *builder)
{
    return (XStrvBuilder *)x_ptr_array_ref(&builder->array);
}

void x_strv_builder_add(XStrvBuilder *builder, const char *value)
{
    x_ptr_array_add(&builder->array, x_strdup(value));
}

void x_strv_builder_addv(XStrvBuilder *builder, const char **value)
{
    xsize i = 0;

    x_return_if_fail(builder != NULL);
    x_return_if_fail(value != NULL);

    for (i = 0; value[i] != NULL; i++) {
        x_strv_builder_add(builder, value[i]);
    }
}

void x_strv_builder_add_many(XStrvBuilder *builder, ...)
{
    va_list var_args;
    const xchar *str;

    x_return_if_fail(builder != NULL);

    va_start(var_args, builder);
    while ((str = va_arg(var_args, xchar *)) != NULL) {
        x_strv_builder_add(builder, str);
    }
    va_end(var_args);
}

void x_strv_builder_take(XStrvBuilder *builder, char *value)
{
    x_ptr_array_add(&builder->array, value);
}

XStrv x_strv_builder_end(XStrvBuilder *builder)
{
    x_ptr_array_add(&builder->array, NULL);
    return (XStrv)x_ptr_array_steal(&builder->array, NULL);
}
