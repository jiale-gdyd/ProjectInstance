#ifndef ACL_LIB_ACL_DB_MYSQL_ACL_DBMYSQL_H
#define ACL_LIB_ACL_DB_MYSQL_ACL_DBMYSQL_H

#include "acl/lib_acl/stdlib/acl_define.h"
#include "acl/lib_acl/db/acl_dbpool.h"

#ifndef ACL_CLIENT_ONLY

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAS_MYSQL

ACL_SQL_RES *acl_dbmysql_select(ACL_DB_HANDLE *handle, const char *sql, int *error);
void acl_dbmysql_free_result(ACL_SQL_RES *res);
int acl_dbmysql_results(ACL_DB_HANDLE *handle, const char *sql, int  *error,
    int (*walk_fn)(const void** my_row, void *arg), void *arg);
int acl_dbmysql_result(ACL_DB_HANDLE *handle, const char *sql, int  *error,
    int (*callback)(const void** my_row, void *arg), void *arg);
int acl_dbmysql_update(ACL_DB_HANDLE *handle, const char *sql, int  *error);

#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
