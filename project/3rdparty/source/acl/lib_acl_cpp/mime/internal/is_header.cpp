#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <ctype.h>

/* Global library. */

#include "is_header.hpp"

/* is_header_buf - determine if this can be a header line */

ssize_t is_header_buf(const char *str, ssize_t str_len)
{
    const unsigned char *cp;
    int     state;
    int     c;
    ssize_t len;

#define INIT		0
#define IN_CHAR		1
#define IN_CHAR_SPACE	2
#define CU_CHAR_PTR(x)	((const unsigned char *) (x))

    /*
    * XXX RFC 2822 Section 4.5, Obsolete header fields: whitespace may
    * appear between header label and ":" (see: RFC 822, Section 3.4.2.).
    * 
    * XXX Don't run off the end in case some non-standard iscntrl()
    * implementation considers null a non-control character...
    */
    for (len = 0, state = INIT, cp = CU_CHAR_PTR(str); /* see below */; cp++) {
        if (str_len != IS_HEADER_NULL_TERMINATED && str_len-- <= 0)
            return (0);
        switch (c = *cp) {
        default:
            if (c == 0 || !ACL_ISASCII(c) || ACL_ISCNTRL(c))
                return (0);
            if (state == INIT)
                state = IN_CHAR;
            if (state == IN_CHAR) {
                len++;
                continue;
            }
            return (0);
        case ' ':
        case '\t':
            if (state == IN_CHAR)
                state = IN_CHAR_SPACE;
            if (state == IN_CHAR_SPACE)
                continue;
            return (0);
        case ':':
            return ((state == IN_CHAR || state == IN_CHAR_SPACE) ? len : 0);
        }
    }
    /* Redundant return for future proofing. */
    return (0);
}

#endif // !defined(ACL_MIME_DISABLE)