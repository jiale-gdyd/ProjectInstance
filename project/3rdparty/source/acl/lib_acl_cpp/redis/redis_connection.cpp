#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#ifndef ACL_PREPARE_COMPILE
#include "acl/lib_acl_cpp/stdlib/string.hpp"
#include "acl/lib_acl_cpp/stdlib/snprintf.hpp"
#include "acl/lib_acl_cpp/redis/redis_client.hpp"
#include "acl/lib_acl_cpp/redis/redis_connection.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_connection::redis_connection()
{
}

redis_connection::redis_connection(redis_client* conn)
: redis_command(conn)
{
}

redis_connection::redis_connection(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_connection::redis_connection(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_connection::redis_connection(redis_client_pipeline* pipeline)
: redis_command(pipeline)
{
}

redis_connection::~redis_connection()
{
}

bool redis_connection::auth(const char* passwd)
{
    const char* argv[2];
    size_t lens[2];

    argv[0] = "AUTH";
    lens[0] = sizeof("AUTH") - 1;

    argv[1] = passwd;
    lens[1] = strlen(passwd);

    build_request(2, argv, lens);
    return check_status();
}

bool redis_connection::select(int dbnum)
{
    const char* argv[2];
    size_t lens[2];
    
    argv[0] = "SELECT";
    lens[0] = strlen(argv[0]);

    char* buf = (char*) dbuf_->dbuf_alloc(21);
    safe_snprintf(buf, 21, "%d", dbnum);
    argv[1] = buf;
    lens[1] = strlen(argv[1]);

    build_request(2, argv, lens);
    return check_status();
}

bool redis_connection::ping()
{
    const char* argv[1];
    size_t lens[1];
    
    argv[0] = "PING";
    lens[0] = strlen(argv[0]);

    build_request(1, argv, lens);
    return check_status("PONG");
}

bool redis_connection::echo(const char* s)
{
    const char* argv[2];
    size_t lens[2];
    
    argv[0] = "ECHO";
    lens[0] = strlen(argv[0]);

    argv[1] = s;
    lens[1] = strlen(argv[1]);

    string buf;
    build_request(2, argv, lens);
    return get_string(buf) >= 0 ? true : false;
}

bool redis_connection::quit()
{
    const char* argv[1];
    size_t lens[1];

    argv[0] = "QUIT";
    lens[0] = strlen(argv[0]);

    build_request(1, argv, lens);
    return check_status();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
