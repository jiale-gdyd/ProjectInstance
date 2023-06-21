#ifndef ACL_LIB_ACL_DB_MEMDB_MDB_PRIVATE_H
#define ACL_LIB_ACL_DB_MEMDB_MDB_PRIVATE_H

#include "struct.h"

ACL_MDT *acl_mdt_hash_create(void);
ACL_MDT *acl_mdt_binhash_create(void);
ACL_MDT *acl_mdt_avl_create(void);

#endif
