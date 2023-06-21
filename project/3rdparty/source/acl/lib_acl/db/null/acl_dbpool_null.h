#ifndef ACL_LIB_ACL_DB_NULL_ACL_DBPOOL_NULL_H
#define ACL_LIB_ACL_DB_NULL_ACL_DBPOOL_NULL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl/lib_acl/stdlib/acl_define.h"
#include "acl/lib_acl/db/acl_dbpool.h"

/* in acl_dbpool_null.c */
/**
 * 创建一个 null 类型的数据库连接池
 * @param db_info 记录着有关连接数据所需要的信息
 * @return DB_POOL * 返回一个能用的连接池句柄
 */
extern ACL_DB_POOL *acl_dbpool_null_create(const ACL_DB_INFO *db_info);

#ifdef __cplusplus
}
#endif

#endif
