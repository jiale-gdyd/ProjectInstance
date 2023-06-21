#ifndef ACL_LIBACL_STDLIB_ACL_GETOPT_H
#define ACL_LIBACL_STDLIB_ACL_GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

extern ACL_API int acl_optind;
extern ACL_API char *acl_optarg;

ACL_API void acl_getopt_init(void);
ACL_API int acl_getopt(int argc, char *argv[], const char *opts);

#ifdef __cplusplus
}
#endif

#endif
