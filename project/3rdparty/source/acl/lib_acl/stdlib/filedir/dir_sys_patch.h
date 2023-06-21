#ifndef ACL_LIB_ACL_STDLIB_FILEDIR_DIR_SYS_PATCH_H
#define ACL_LIB_ACL_STDLIB_FILEDIR_DIR_SYS_PATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)

#include <direct.h>
#define __S_ISTYPE(mode, mask)  (((mode) & _S_IFMT) == (mask))

#ifndef S_ISDIR
#define S_ISDIR(mode)           __S_ISTYPE((mode), _S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)           __S_ISTYPE((mode), _S_IFREG)
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
