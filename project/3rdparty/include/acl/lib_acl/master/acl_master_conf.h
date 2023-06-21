#ifndef ACL_LIBACL_MASTER_ACL_MASTER_CONF_H
#define ACL_LIBACL_MASTER_ACL_MASTER_CONF_H

#include "../stdlib/acl_define.h"
#include "acl_master_type.h"

#ifndef ACL_CLIENT_ONLY

#ifdef __cplusplus
extern "C" {
#endif

ACL_API void acl_app_conf_load(const char *pathname);
ACL_API void acl_app_conf_unload(void);
ACL_API void acl_get_app_conf_int_table(ACL_CONFIG_INT_TABLE *table);
ACL_API void acl_get_app_conf_int64_table(ACL_CONFIG_INT64_TABLE *table);
ACL_API void acl_get_app_conf_str_table(ACL_CONFIG_STR_TABLE *table);
ACL_API void acl_get_app_conf_bool_table(ACL_CONFIG_BOOL_TABLE *table);
ACL_API void acl_free_app_conf_str_table(ACL_CONFIG_STR_TABLE *table);

#ifdef __cplusplus
}
#endif

#endif

#endif
