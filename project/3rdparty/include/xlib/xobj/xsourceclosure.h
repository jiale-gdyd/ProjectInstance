#ifndef __X_SOURCECLOSURE_H__
#define __X_SOURCECLOSURE_H__

#include "xclosure.h"
#include "xlib-types.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
void x_source_set_closure(XSource *source, XClosure *closure);

XLIB_AVAILABLE_IN_ALL
void x_source_set_dummy_callback(XSource *source);

X_END_DECLS

#endif
