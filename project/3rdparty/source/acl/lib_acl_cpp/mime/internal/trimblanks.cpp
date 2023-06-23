#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <ctype.h>

/* Utility library. */

#include "trimblanks.hpp"

char   *trimblanks(char *string, int len)
{
    char   *curr;

    if (len) {
    curr = string + len;
    } else {
    for (curr = string; *curr != 0; curr++)
        /* void */ ;
    }
    while (curr > string && ACL_ISSPACE(curr[-1]))
    curr -= 1;
    return (curr);
}

#endif // !defined(ACL_MIME_DISABLE)
