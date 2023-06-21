#ifndef ACL_LIBACL_UNIT_TEST_ACL_TEST_VAR_H
#define ACL_LIBACL_UNIT_TEST_ACL_TEST_VAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_array.h"

extern int var_aut_log_level;
extern int var_aut_verbose;

extern ACL_ARRAY *var_aut_line_array;
extern int var_aut_valid_line_idx;

#ifdef __cplusplus
}
#endif

#endif
