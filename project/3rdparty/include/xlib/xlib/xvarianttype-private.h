#ifndef __X_VARIANTTYPE_PRIVATE_H__
#define __X_VARIANTTYPE_PRIVATE_H__

#include "xvarianttype.h"

X_BEGIN_DECLS

static inline xboolean _x_variant_type_equal(const XVariantType *type1, const XVariantType *type2)
{
    xsize index = 0;
    int brackets = 0;
    const char *str1 = (const char *)type1;
    const char *str2 = (const char *)type2;

    if (str1 == str2) {
        return TRUE;
    }

    do {
        if (str1[index] != str2[index]) {
            return FALSE;
        }

        while (str1[index] == 'a' || str1[index] == 'm') {
            index++;
            if (str1[index] != str2[index]) {
                return FALSE;
            }
        }

        if (str1[index] == '(' || str1[index] == '{') {
            brackets++;
        } else if (str1[index] == ')' || str1[index] == '}') {
            brackets--;
        }

        index++;
    } while (brackets);

    return TRUE;
}

static inline xuint _x_variant_type_hash(xconstpointer type)
{
    xuint value = 0;
    xsize index = 0;
    int brackets = 0;
    const xchar *type_string = type;

    do {
        value = (value << 5) - value + type_string[index];
        while (type_string[index] == 'a' || type_string[index] == 'm') {
            index++;
            value = (value << 5) - value + type_string[index];
        }

        if (type_string[index] == '(' || type_string[index] == '{') {
            brackets++;
        } else if (type_string[index] == ')' || type_string[index] == '}') {
            brackets--;
        }

        index++;
    } while (brackets);

    return value;
}

X_END_DECLS

#endif
