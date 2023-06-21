#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"
#include <ctype.h>
#include <string.h>
#include <limits.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_mymalloc.h"
#include "acl/lib_acl/stdlib/acl_vstring.h"
#include "acl/lib_acl/code/acl_vstring_base64.h"

#endif

static const unsigned char to_b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char un_b64[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255,
    255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255,
    255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

#define UNSIG_CHAR_PTR(x) ((const unsigned char *)(x))

/* vstring_base64_encode - raw data to encoded */

ACL_VSTRING *acl_vstring_base64_encode(ACL_VSTRING *result,
    const char *in, int len)
{
    const unsigned char *cp;
    int     count, size = len * 4 /3;

    ACL_VSTRING_SPACE(result, size);

    /*
    * Encode 3 -> 4.
    */
    ACL_VSTRING_RESET(result);
    for (cp = UNSIG_CHAR_PTR(in), count = len;
        count > 0; count -= 3, cp += 3) {
        ACL_VSTRING_ADDCH(result, to_b64[cp[0] >> 2]);
        if (count > 1) {
            ACL_VSTRING_ADDCH(result,
                to_b64[(cp[0] & 0x3) << 4 | cp[1] >> 4]);
            if (count > 2) {
                ACL_VSTRING_ADDCH(result,
                    to_b64[(cp[1] & 0xf) << 2 | cp[2] >> 6]);
                ACL_VSTRING_ADDCH(result, to_b64[cp[2] & 0x3f]);
            } else {
                ACL_VSTRING_ADDCH(result, to_b64[(cp[1] & 0xf) << 2]);
                ACL_VSTRING_ADDCH(result, '=');
                break;
            }
        } else {
            ACL_VSTRING_ADDCH(result, to_b64[(cp[0] & 0x3) << 4]);
            ACL_VSTRING_ADDCH(result, '=');
            ACL_VSTRING_ADDCH(result, '=');
            break;
        }
    }
    ACL_VSTRING_TERMINATE(result);
    return (result);
}

/* acl_vstring_base64_decode - encoded data to raw */

ACL_VSTRING *acl_vstring_base64_decode(ACL_VSTRING *result,
    const char *in, int len)
{
    const unsigned char *cp;
    int     count;
    int     ch0;
    int     ch1;
    int     ch2;
    int     ch3;

#define CHARS_PER_BYTE	(UCHAR_MAX + 1)
#define INVALID		0xff

    /*
    * Sanity check.
    */
    if (len % 4)
        return (NULL);

    /*
    * Once: initialize the decoding lookup table on the fly.
    *
    * if (un_b64 == 0) {
    *	un_b64 = (unsigned char *) mymalloc(CHARS_PER_BYTE);
    *	memset(un_b64, INVALID, CHARS_PER_BYTE);
    *	for (cp = to_b64; cp < to_b64 + sizeof(to_b64); cp++)
    *		un_b64[*cp] = cp - to_b64;
    * }
    */

    ACL_VSTRING_SPACE(result, len);
    /*
    * Decode 4 -> 3.
    */
    ACL_VSTRING_RESET(result);

    for (cp = UNSIG_CHAR_PTR(in), count = 0; count < len; count += 4) {
        if ((ch0 = un_b64[*cp++]) == INVALID
            || (ch1 = un_b64[*cp++]) == INVALID)
            return (0);
        ACL_VSTRING_ADDCH(result, ch0 << 2 | ch1 >> 4);
        if ((ch2 = *cp++) == '=')
            break;
        if ((ch2 = un_b64[ch2]) == INVALID)
            return (0);
        ACL_VSTRING_ADDCH(result, ch1 << 4 | ch2 >> 2);
        if ((ch3 = *cp++) == '=')
            break;
        if ((ch3 = un_b64[ch3]) == INVALID)
            return (0);
        ACL_VSTRING_ADDCH(result, ch2 << 6 | ch3);
    }
    ACL_VSTRING_TERMINATE(result);
    return (result);
}
