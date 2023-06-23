#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

/* Utility library. */

/* Global library. */

#include "tok822.hpp"

/* tok822_find_type - find specific token type, forward search */

TOK822 *tok822_find_type(TOK822 *head, int op)
{
    TOK822 *tp;

    for (tp = head; tp != 0 && tp->type != op; tp = tp->next)
        /* void */ ;
    return (tp);
}

/* tok822_rfind_type - find specific token type, backward search */

TOK822 *tok822_rfind_type(TOK822 *tail, int op)
{
    TOK822 *tp;

    for (tp = tail; tp != 0 && tp->type != op; tp = tp->prev)
        /* void */ ;
    return (tp);
}

#endif // !defined(ACL_MIME_DISABLE)
