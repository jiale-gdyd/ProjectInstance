#ifndef __X_TYPE_PLUGIN_H__
#define __X_TYPE_PLUGIN_H__

#include "xtype.h"

X_BEGIN_DECLS

#define X_TYPE_TYPE_PLUGIN                  (x_type_plugin_get_type())
#define X_TYPE_PLUGIN(inst)                 (X_TYPE_CHECK_INSTANCE_CAST((inst), X_TYPE_TYPE_PLUGIN, XTypePlugin))
#define X_TYPE_PLUGIN_CLASS(vtable)         (X_TYPE_CHECK_CLASS_CAST((vtable), X_TYPE_TYPE_PLUGIN, XTypePluginClass))
#define X_IS_TYPE_PLUGIN(inst)              (X_TYPE_CHECK_INSTANCE_TYPE((inst), X_TYPE_TYPE_PLUGIN))
#define X_IS_TYPE_PLUGIN_CLASS(vtable)      (X_TYPE_CHECK_CLASS_TYPE((vtable), X_TYPE_TYPE_PLUGIN))
#define X_TYPE_PLUGIN_GET_CLASS(inst)       (X_TYPE_INSTANCE_GET_INTERFACE((inst), X_TYPE_TYPE_PLUGIN, XTypePluginClass))

typedef struct _XTypePluginClass XTypePluginClass;

typedef void (*XTypePluginUse)(XTypePlugin *plugin);
typedef void (*XTypePluginUnuse)(XTypePlugin *plugin);

typedef void  (*XTypePluginCompleteTypeInfo)(XTypePlugin *plugin, XType x_type, XTypeInfo *info, XTypeValueTable *value_table);
typedef void  (*XTypePluginCompleteInterfaceInfo)(XTypePlugin *plugin, XType instance_type, XType interface_type, XInterfaceInfo *info);

struct _XTypePluginClass {
    XTypeInterface                   base_iface;

    XTypePluginUse                   use_plugin;
    XTypePluginUnuse                 unuse_plugin;
    XTypePluginCompleteTypeInfo      complete_type_info;
    XTypePluginCompleteInterfaceInfo complete_interface_info;
};

XLIB_AVAILABLE_IN_ALL
XType x_type_plugin_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
void x_type_plugin_use(XTypePlugin *plugin);

XLIB_AVAILABLE_IN_ALL
void x_type_plugin_unuse(XTypePlugin *plugin);

XLIB_AVAILABLE_IN_ALL
void x_type_plugin_complete_type_info(XTypePlugin *plugin, XType x_type, XTypeInfo *info, XTypeValueTable *value_table);

XLIB_AVAILABLE_IN_ALL
void x_type_plugin_complete_interface_info(XTypePlugin *plugin, XType instance_type, XType interface_type, XInterfaceInfo *info);

X_END_DECLS

#endif
