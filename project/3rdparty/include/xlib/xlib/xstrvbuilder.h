#ifndef __X_STRVBUILDER_H__
#define __X_STRVBUILDER_H__

#include "xstrfuncs.h"
#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XStrvBuilder XStrvBuilder;

XLIB_AVAILABLE_IN_2_68
XStrvBuilder *x_strv_builder_new(void);

XLIB_AVAILABLE_IN_2_68
void x_strv_builder_unref(XStrvBuilder *builder);

XLIB_AVAILABLE_IN_2_68
XStrvBuilder *x_strv_builder_ref(XStrvBuilder *builder);

XLIB_AVAILABLE_IN_2_68
void x_strv_builder_add(XStrvBuilder *builder, const char *value);

XLIB_AVAILABLE_IN_2_70
void x_strv_builder_addv(XStrvBuilder *builder, const char **value);

XLIB_AVAILABLE_IN_2_70
void x_strv_builder_add_many(XStrvBuilder *builder, ...) X_GNUC_NULL_TERMINATED;

XLIB_AVAILABLE_IN_2_68
XStrv x_strv_builder_end(XStrvBuilder *builder);

X_END_DECLS

#endif
