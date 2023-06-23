#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <string.h>
#include <ctype.h>

/* Utility library. */

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_vstring.h"

/* Global library. */

#include "lex_822.hpp"
#include "header_token.hpp"

/* Application-specific. */

/*
* Silly little macros.
*/
#define STR(x)	acl_vstring_str(x)
#define LEN(x)	ACL_VSTRING_LEN(x)
#define CU_CHAR_PTR(x)	((const unsigned char *) (x))

/* header_token - parse out the next item in a message header */

ssize_t header_token(HEADER_TOKEN *token, ssize_t token_len,
    ACL_VSTRING *token_buffer, const char **ptr,
    const char *user_specials, int user_terminator)
{
    ssize_t comment_level;
    const unsigned char *cp;
    ssize_t len;
    int     ch;
    ssize_t tok_count;
    ssize_t n;

    /*
    * Initialize.
    */
    ACL_VSTRING_RESET(token_buffer);
    cp = CU_CHAR_PTR(*ptr);
    tok_count = 0;
    if (user_specials == 0)
        user_specials = LEX_822_SPECIALS;

    /*
    * Main parsing loop.
    * 
    * XXX What was the reason to continue parsing when user_terminator is
    * specified? Perhaps this was needed at some intermediate stage of
    * development?
    */
    while ((ch = *cp) != 0 && (user_terminator != 0 || tok_count < token_len)) {
        cp++;

        /*
        * Skip RFC 822 linear white space.
        */
        if (IS_SPACE_TAB_CR_LF(ch))
            continue;

        /*
        * Terminator.
        */
        if (ch == user_terminator)
            break;

        /*
        * Skip RFC 822 comment.
        */
        if (ch == '(') {
            comment_level = 1;
            while ((ch = *cp) != 0) {
                cp++;
                if (ch == '(') {  /* comments can nest! */
                    comment_level++;
                } else if (ch == ')') {
                    if (--comment_level == 0)
                        break;
                } else if (ch == '\\') {
                    if ((ch = *cp) == 0)
                        break;
                    cp++;
                }
            }
            continue;
        }

        /*
        * Copy quoted text according to RFC 822.
        */
        if (ch == '"') {
            if (tok_count < token_len) {
                token[tok_count].u.offset = (ssize_t) LEN(token_buffer);
                token[tok_count].type = HEADER_TOK_QSTRING;
            }
            while ((ch = *cp) != 0) {
                cp++;
                if (ch == '"')
                    break;
                if (ch == '\n') {		/* unfold */
                    if (tok_count < token_len) {
                        len = (ssize_t) LEN(token_buffer);
                        while (len > 0 && IS_SPACE_TAB_CR_LF(STR(token_buffer)[len - 1]))
                            len--;
                        if (len < (ssize_t) LEN(token_buffer))
                            acl_vstring_truncate(token_buffer, len);
                    }
                    continue;
                }
                if (ch == '\\') {
                    if (tok_count < token_len)
                        ACL_VSTRING_ADDCH(token_buffer, ch);

                    if (*cp == 0)
                        break;
                    ch = *cp;
                    cp++;
                }
                if (tok_count < token_len)
                    ACL_VSTRING_ADDCH(token_buffer, ch);
            }
            if (tok_count < token_len) {
                ACL_VSTRING_ADDCH(token_buffer, 0);
                tok_count++;
            }
            continue;
        }

        /*
        * Control, or special.
        */
        if (strchr(user_specials, ch) || ACL_ISCNTRL(ch)) {
            if (tok_count < token_len) {
                token[tok_count].u.offset = (ssize_t) LEN(token_buffer);
                token[tok_count].type = ch;
                ACL_VSTRING_ADDCH(token_buffer, ch);
                ACL_VSTRING_ADDCH(token_buffer, 0);
                tok_count++;
            }
            continue;
        }

        /*
        * Token.
        */
        else {
            if (tok_count < token_len) {
                token[tok_count].u.offset = (ssize_t) LEN(token_buffer);
                token[tok_count].type = HEADER_TOK_TOKEN;
                ACL_VSTRING_ADDCH(token_buffer, ch);
            }
            while ((ch = *cp) != 0 && !IS_SPACE_TAB_CR_LF(ch)
                && !ACL_ISCNTRL(ch) && !strchr(user_specials, ch)) {
                    cp++;
                    if (tok_count < token_len)
                        ACL_VSTRING_ADDCH(token_buffer, ch);
                }
                if (tok_count < token_len) {
                    ACL_VSTRING_ADDCH(token_buffer, 0);
                    tok_count++;
                }
                continue;
        }
    }

    /*
    * Ignore a zero-length item after the last terminator.
    */
    if (tok_count == 0 && ch == 0)
        return (-1);

    /*
    * Finalize. Fill in the string pointer array, now that the token buffer
    * is no longer dynamically reallocated as it grows.
    */
    *ptr = (const char *) cp;
    for (n = 0; n < tok_count; n++)
        token[n].u.value = STR(token_buffer) + token[n].u.offset;

    //if (acl_msg_verbose)
    //	acl_msg_info("header_token: %s %s %s",
    //		tok_count > 0 ? token[0].u.value : "",
    //		tok_count > 1 ? token[1].u.value : "",
    //		tok_count > 2 ? token[2].u.value : "");

    return (tok_count);
}

#endif // !defined(ACL_MIME_DISABLE)
