#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#ifndef ACL_PREPARE_COMPILE
#include "acl/lib_acl_cpp/stdlib/string.hpp"
#endif

#include "acl/lib_acl_cpp/acl_cpp_init.hpp"

namespace acl
{
    void acl_cpp_init(void)
    {
        acl_lib_init();
    }

    const char *acl_cpp_verbose(void)
    {
        static string buf;
        buf.clear();

    #ifdef HAS_MBEDTLS
        buf += "HAS_MBEDTLS";
    # ifdef HAS_MBEDTLS_DLL
        buf += ", HAS_MBEDTLS_DLL";
    # endif
    #endif

    #ifdef HAS_POLARSSL
        buf += ", HAS_POLARSSL";
    # ifdef HAS_POLARSSL_DLL
        buf += ", HAS_POLARSSL_DLL";
    # endif
    #endif

    #ifdef ACL_CLIENT_ONLY
        buf += ", ACL_CLIENT_ONLY";
    #endif

    #ifdef ACL_HOOK_NEW
        buf += ", ACL_HOOK_NEW";
    #endif

        return buf.c_str();
    }
}
