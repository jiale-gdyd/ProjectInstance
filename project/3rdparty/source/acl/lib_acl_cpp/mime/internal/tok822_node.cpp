#include "acl/lib_acl_cpp/acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <string.h>

/* Utility library. */

#include "acl/lib_acl/stdlib/acl_mymalloc.h"
#include "acl/lib_acl/stdlib/acl_vstring.h"

/* Global library. */

#include "tok822.hpp"

/* tok822_alloc - allocate and initialize token */

TOK822 *tok822_alloc(int type, const char *strval)
{
    TOK822 *tp;

#define CONTAINER_TOKEN(x) \
    ((x) == TOK822_ADDR || (x) == TOK822_STARTGRP)

    tp = (TOK822 *) acl_mymalloc(sizeof(*tp));
    tp->type = type;
    tp->next = tp->prev = tp->head = tp->tail = tp->owner = 0;
    tp->vstr = (type < TOK822_MINTOK || CONTAINER_TOKEN(type) ? 0 :
        strval == 0 ? acl_vstring_alloc(10) :
        acl_vstring_strcpy(acl_vstring_alloc(strlen(strval) + 1), strval));
    return (tp);
}

/* tok822_free - destroy token */

TOK822 *tok822_free(TOK822 *tp)
{
    if (tp->vstr)
        acl_vstring_free(tp->vstr);
    acl_myfree(tp);
    return (0);
}

#endif // !defined(ACL_MIME_DISABLE)
