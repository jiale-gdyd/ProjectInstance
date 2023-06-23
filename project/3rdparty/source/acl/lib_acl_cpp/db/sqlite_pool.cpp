#include "acl/lib_acl_cpp/acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl/lib_acl_cpp/connpool/connect_client.hpp"
#include "acl/lib_acl_cpp/db/db_handle.hpp"
#include "acl/lib_acl_cpp/db/db_sqlite.hpp"
#include "acl/lib_acl_cpp/db/sqlite_pool.hpp"
#endif

#if !defined(ACL_DB_DISABLE)

namespace acl
{

sqlite_pool::sqlite_pool(const char* dbfile, size_t dblimit /* = 64 */,
    const char* charset /* = "utf-8" */)
: db_pool(dbfile, dblimit)
{
    acl_assert(dbfile && *dbfile);
    dbfile_ = acl_mystrdup(dbfile);
    if (charset && *charset) {
        charset_ = acl_mystrdup(charset);
    } else {
        charset_ = NULL;
    }
}

sqlite_pool::~sqlite_pool(void)
{
    acl_myfree(dbfile_);
    if (charset_) {
        acl_myfree(charset_);
    }
}

connect_client* sqlite_pool::create_connect(void)
{
    return NEW db_sqlite(dbfile_, charset_);
}

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
