#ifndef __X_SIGNAL_GROUP_H__
#define __X_SIGNAL_GROUP_H__

#include "../xlib.h"
#include "xobject.h"
#include "xsignal.h"

X_BEGIN_DECLS

#define X_SIGNAL_GROUP(obj)         (X_TYPE_CHECK_INSTANCE_CAST((obj), X_TYPE_SIGNAL_GROUP, XSignalGroup))
#define X_IS_SIGNAL_GROUP(obj)      (X_TYPE_CHECK_INSTANCE_TYPE((obj), X_TYPE_SIGNAL_GROUP))
#define X_TYPE_SIGNAL_GROUP         (x_signal_group_get_type())

typedef struct _XSignalGroup XSignalGroup;

XLIB_AVAILABLE_IN_2_72
XType x_signal_group_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_72
XSignalGroup *x_signal_group_new(XType target_type);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_set_target(XSignalGroup *self, xpointer target);

XLIB_AVAILABLE_IN_2_72
xpointer x_signal_group_dup_target(XSignalGroup *self);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_block(XSignalGroup *self);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_unblock(XSignalGroup *self);

XLIB_AVAILABLE_IN_2_74
void x_signal_group_connect_closure(XSignalGroup *self, const xchar *detailed_signal, XClosure *closure, xboolean after);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_connect_object(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer object, XConnectFlags flags);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_connect_data(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data, XClosureNotify notify, XConnectFlags flags);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_connect(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_connect_after(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data);

XLIB_AVAILABLE_IN_2_72
void x_signal_group_connect_swapped(XSignalGroup *self, const xchar *detailed_signal, XCallback c_handler, xpointer data);

X_END_DECLS

#endif
