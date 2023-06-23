#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <string.h>
#include <ctype.h>

/* Utility library. */

#include "acl/lib_acl/stdlib/acl_vstring.h"

/* Global library. */

#include "quote_821_local.hpp"

/* Application-specific. */

#define YES	1
#define	NO	0

/* is_821_dot_string - is this local-part an rfc 821 dot-string? */

static int is_821_dot_string(const char *local_part, const char *end, int flags)
{
    const char *cp;
    int     ch;

    /*
    * Detect any deviations from the definition of dot-string. We could use
    * lookup tables to speed up some of the work, but hey, how large can a
    * local-part be anyway?
    */
    if (local_part == end || local_part[0] == 0 || local_part[0] == '.')
        return (NO);
    for (cp = local_part; cp < end && (ch = *(const unsigned char *) cp) != 0; cp++) {
        if (ch == '.' && cp[1] == '.')
            return (NO);
        if (ch > 127 && !(flags & QUOTE_FLAG_8BITCLEAN))
            return (NO);
        if (ch == ' ')
            return (NO);
        if (ACL_ISCNTRL(ch))
            return (NO);
        if (ch == '<' || ch == '>'
            || ch == '(' || ch == ')'
            || ch == '[' || ch == ']'
            || ch == '\\' || ch == ','
            || ch == ';' || ch == ':'
            || (ch == '@' && !(flags & QUOTE_FLAG_EXPOSE_AT)) || ch == '"')
        {
            return (NO);
        }
    }
    if (cp[-1] == '.')
        return (NO);
    return (YES);
}

/* make_821_quoted_string - make quoted-string from local-part */

static ACL_VSTRING *make_821_quoted_string(ACL_VSTRING *dst, const char *local_part,
        const char *end, int flags)
{
    const char *cp;
    int     ch;

    /*
    * Put quotes around the result, and prepend a backslash to characters
    * that need quoting when they occur in a quoted-string.
    */
    ACL_VSTRING_ADDCH(dst, '"');
    for (cp = local_part; cp < end && (ch = *(const unsigned char *) cp) != 0; cp++) {
        if ((ch > 127 && !(flags & QUOTE_FLAG_8BITCLEAN))
            || ch == '\r' || ch == '\n' || ch == '"' || ch == '\\')
        {
            ACL_VSTRING_ADDCH(dst, '\\');
        }
        ACL_VSTRING_ADDCH(dst, ch);
    }
    ACL_VSTRING_ADDCH(dst, '"');
    ACL_VSTRING_TERMINATE(dst);
    return (dst);
}

/* quote_821_local_flags - quote local part of address according to rfc 821 */

ACL_VSTRING *quote_821_local_flags(ACL_VSTRING *dst, const char *addr, int flags)
{
    const char   *at;

    /*
    * According to RFC 821, a local-part is a dot-string or a quoted-string.
    * We first see if the local-part is a dot-string. If it is not, we turn
    * it into a quoted-string. Anything else would be too painful.
    */
    if ((at = strrchr(addr, '@')) == 0)		/* just in case */
        at = addr + strlen(addr);		/* should not happen */
    if ((flags & QUOTE_FLAG_APPEND) == 0)
        ACL_VSTRING_RESET(dst);
    if (is_821_dot_string(addr, at, flags)) {
        return (acl_vstring_strcat(dst, addr));
    } else {
        make_821_quoted_string(dst, addr, at, flags & QUOTE_FLAG_8BITCLEAN);
        return (acl_vstring_strcat(dst, at));
    }
}

#ifdef TEST

/*
* Test program for local-part quoting as per rfc 821
*/
#include <stdlib.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include "quote_821_local.h"

int     main(void)
{
    VSTRING *src = vstring_alloc(100);
    VSTRING *dst = vstring_alloc(100);

    while (vstring_fgets_nonl(src, VSTREAM_IN)) {
        vstream_fprintf(VSTREAM_OUT, "%s\n",
            vstring_str(quote_821_local(dst, vstring_str(src))));
        vstream_fflush(VSTREAM_OUT);
    }
    exit(0);
}

#endif
#endif // !defined(ACL_MIME_DISABLE)
