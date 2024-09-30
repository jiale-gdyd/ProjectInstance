#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xvariant-internal.h>
#include <xlib/xlib/xvarianttype-private.h>

static xboolean x_variant_type_check(const XVariantType *type)
{
    if (type == NULL) {
        return FALSE;
    }

    return TRUE;
}

static xboolean variant_type_string_scan_internal(const xchar  *string, const xchar *limit, const xchar **endptr, xsize *depth, xsize depth_limit)
{
    xsize max_depth = 0, child_depth;

    x_return_val_if_fail(string != NULL, FALSE);

    if (string == limit || *string == '\0') {
        return FALSE;
    }

    switch (*string++) {
        case '(':
            while (string == limit || *string != ')') {
                if (depth_limit == 0 || !variant_type_string_scan_internal(string, limit, &string, &child_depth, depth_limit - 1)) {
                    return FALSE;
                }
                max_depth = MAX(max_depth, child_depth + 1);
            }
            string++;
            break;

        case '{':
            if (depth_limit == 0 || string == limit || *string == '\0' || !strchr("bynqihuxtdsog?", *string++) || 
                !variant_type_string_scan_internal(string, limit, &string, &child_depth, depth_limit - 1) || string == limit || *string++ != '}')
            {
                return FALSE;
            }
            max_depth = MAX(max_depth, child_depth + 1);
            break;

        case 'm': case 'a':
            if (depth_limit == 0 || !variant_type_string_scan_internal(string, limit, &string, &child_depth, depth_limit - 1)) {
                return FALSE;
            }
            max_depth = MAX(max_depth, child_depth + 1);
            break;

        case 'b': case 'y': case 'n': case 'q': case 'i': case 'u':
        case 'x': case 't': case 'd': case 's': case 'o': case 'g':
        case 'v': case 'r': case '*': case '?': case 'h':
            max_depth = MAX (max_depth, 1);
            break;

        default:
            return FALSE;
    }

    if (endptr != NULL) {
        *endptr = string;
    }

    if (depth != NULL) {
        *depth = max_depth;
    }

    return TRUE;
}

xboolean x_variant_type_string_scan(const xchar *string, const xchar *limit, const xchar **endptr)
{
    return variant_type_string_scan_internal(string, limit, endptr, NULL, X_VARIANT_MAX_RECURSION_DEPTH);
}

xsize x_variant_type_string_get_depth_(const xchar *type_string)
{
    xsize depth = 0;
    const xchar *endptr;

    x_return_val_if_fail(type_string != NULL, 0);

    if (!variant_type_string_scan_internal(type_string, NULL, &endptr, &depth, X_VARIANT_MAX_RECURSION_DEPTH) || *endptr != '\0') {
        return 0;
    }

    return depth;
}

xboolean x_variant_type_string_is_valid(const xchar *type_string)
{
    const xchar *endptr;

    x_return_val_if_fail(type_string != NULL, FALSE);

    if (!x_variant_type_string_scan(type_string, NULL, &endptr)) {
        return FALSE;
    }

    return *endptr == '\0';
}

void x_variant_type_free(XVariantType *type)
{
    x_return_if_fail(type == NULL || x_variant_type_check(type));
    x_free(type);
}

XVariantType *x_variant_type_copy(const XVariantType *type)
{
    xchar *newt;
    xsize length;

    x_return_val_if_fail(x_variant_type_check(type), NULL);

    length = x_variant_type_get_string_length(type);
    newt = (xchar *)x_malloc(length + 1);

    memcpy(newt, type, length);
    newt[length] = '\0';

    return (XVariantType *)newt;
}

XVariantType *x_variant_type_new(const xchar *type_string)
{
    x_return_val_if_fail(type_string != NULL, NULL);
    return x_variant_type_copy(X_VARIANT_TYPE(type_string));
}

xsize x_variant_type_get_string_length(const XVariantType *type)
{
    xsize index = 0;
    xint brackets = 0;
    const xchar *type_string = (const xchar *)type;

    x_return_val_if_fail(x_variant_type_check(type), 0);

    do {
        while (type_string[index] == 'a' || type_string[index] == 'm') {
            index++;
        }

        if (type_string[index] == '(' || type_string[index] == '{') {
            brackets++;
        } else if (type_string[index] == ')' || type_string[index] == '}') {
            brackets--;
        }

        index++;
    } while (brackets);

    return index;
}

const xchar *x_variant_type_peek_string(const XVariantType *type)
{
    x_return_val_if_fail(x_variant_type_check(type), NULL);
    return (const xchar *)type;
}

xchar *x_variant_type_dup_string(const XVariantType *type)
{
    x_return_val_if_fail(x_variant_type_check(type), NULL);
    return x_strndup(x_variant_type_peek_string(type), x_variant_type_get_string_length(type));
}

xboolean x_variant_type_is_definite(const XVariantType *type)
{
    xsize i;
    xsize type_length;
    const xchar *type_string;

    x_return_val_if_fail(x_variant_type_check(type), FALSE);

    type_length = x_variant_type_get_string_length(type);
    type_string = x_variant_type_peek_string(type);

    for (i = 0; i < type_length; i++) {
        if (type_string[i] == '*' || type_string[i] == '?' || type_string[i] == 'r') {
        return FALSE;
        }
    }

    return TRUE;
}

xboolean x_variant_type_is_container(const XVariantType *type)
{
    xchar first_char;

    x_return_val_if_fail(x_variant_type_check(type), FALSE);

    first_char = x_variant_type_peek_string(type)[0];
    switch (first_char) {
        case 'a':
        case 'm':
        case 'r':
        case '(':
        case '{':
        case 'v':
            return TRUE;

        default:
            return FALSE;
    }
}

xboolean x_variant_type_is_basic(const XVariantType *type)
{
    xchar first_char;

    x_return_val_if_fail(x_variant_type_check(type), FALSE);

    first_char = x_variant_type_peek_string(type)[0];
    switch (first_char) {
        case 'b':
        case 'y':
        case 'n':
        case 'q':
        case 'i':
        case 'h':
        case 'u':
        case 't':
        case 'x':
        case 'd':
        case 's':
        case 'o':
        case 'g':
        case '?':
            return TRUE;

        default:
            return FALSE;
    }
}

xboolean x_variant_type_is_maybe(const XVariantType *type)
{
    x_return_val_if_fail(x_variant_type_check(type), FALSE);
    return x_variant_type_peek_string(type)[0] == 'm';
}

xboolean x_variant_type_is_array(const XVariantType *type)
{
    x_return_val_if_fail(x_variant_type_check(type), FALSE);
    return x_variant_type_peek_string(type)[0] == 'a';
}

xboolean x_variant_type_is_tuple(const XVariantType *type)
{
    xchar type_char;

    x_return_val_if_fail(x_variant_type_check(type), FALSE);

    type_char = x_variant_type_peek_string(type)[0];
    return type_char == 'r' || type_char == '(';
}

xboolean x_variant_type_is_dict_entry(const XVariantType *type)
{
    x_return_val_if_fail(x_variant_type_check(type), FALSE);
    return x_variant_type_peek_string(type)[0] == '{';
}

xboolean x_variant_type_is_variant(const XVariantType *type)
{
    x_return_val_if_fail(x_variant_type_check(type), FALSE);
    return x_variant_type_peek_string(type)[0] == 'v';
}

xuint x_variant_type_hash(xconstpointer type)
{
    x_return_val_if_fail(x_variant_type_check((const XVariantType *)type), 0);
    return _x_variant_type_hash(type);
}

xboolean x_variant_type_equal(xconstpointer type1, xconstpointer type2)
{
    x_return_val_if_fail(x_variant_type_check((const XVariantType *)type1), FALSE);
    x_return_val_if_fail(x_variant_type_check((const XVariantType *)type2), FALSE);

    return _x_variant_type_equal(type1, type2);
}

xboolean x_variant_type_is_subtype_of(const XVariantType *type, const XVariantType *supertype)
{
    const xchar *type_string;
    const xchar *supertype_end;
    const xchar *supertype_string;

    x_return_val_if_fail(x_variant_type_check(type), FALSE);
    x_return_val_if_fail(x_variant_type_check(supertype), FALSE);

    supertype_string = x_variant_type_peek_string(supertype);
    type_string = x_variant_type_peek_string(type);

    if (type_string[0] == supertype_string[0]) {
        switch (type_string[0]) {
            case 'b': case 'y':
            case 'n': case 'q':
            case 'i': case 'h': case 'u':
            case 't': case 'x':
            case 's': case 'o': case 'g':
            case 'd':
                return TRUE;

            default:
                break;
        }
    }

    supertype_end = supertype_string + x_variant_type_get_string_length(supertype);

    while (supertype_string < supertype_end) {
        char supertype_char = *supertype_string++;

        if (supertype_char == *type_string) {
            type_string++;
        } else if (*type_string == ')') {
            return FALSE;
        } else {
            const XVariantType *target_type = (XVariantType *)type_string;

            switch (supertype_char) {
                case 'r':
                    if (!x_variant_type_is_tuple(target_type)) {
                        return FALSE;
                    }
                    break;

                case '*':
                    break;

                case '?':
                    if (!x_variant_type_is_basic(target_type)) {
                        return FALSE;
                    }
                    break;

                default:
                    return FALSE;
            }

            type_string += x_variant_type_get_string_length(target_type);
        }
    }

    return TRUE;
}

const XVariantType *x_variant_type_element(const XVariantType *type)
{
    const xchar *type_string;

    x_return_val_if_fail(x_variant_type_check(type), NULL);
    type_string = x_variant_type_peek_string(type);
    x_assert(type_string[0] == 'a' || type_string[0] == 'm');

    return (const XVariantType *)&type_string[1];
}

const XVariantType *x_variant_type_first(const XVariantType *type)
{
    const xchar *type_string;

    x_return_val_if_fail(x_variant_type_check(type), NULL);

    type_string = x_variant_type_peek_string(type);
    x_assert(type_string[0] == '(' || type_string[0] == '{');

    if (type_string[1] == ')') {
        return NULL;
    }

    return (const XVariantType *)&type_string[1];
}

const XVariantType *x_variant_type_next(const XVariantType *type)
{
    const xchar *type_string;

    x_return_val_if_fail(x_variant_type_check(type), NULL);

    type_string = x_variant_type_peek_string(type);
    type_string += x_variant_type_get_string_length(type);

    if (*type_string == ')' || *type_string == '}') {
        return NULL;
    }

    return (const XVariantType *)type_string;
}

xsize x_variant_type_n_items(const XVariantType *type)
{
    xsize count = 0;

    x_return_val_if_fail(x_variant_type_check(type), 0);
    for (type = x_variant_type_first(type); type; type = x_variant_type_next(type)) {
        count++;
    }

    return count;
}

const XVariantType *x_variant_type_key(const XVariantType *type)
{
    const xchar *type_string;

    x_return_val_if_fail(x_variant_type_check(type), NULL);

    type_string = x_variant_type_peek_string(type);
    x_assert(type_string[0] == '{');

    return (const XVariantType *)&type_string[1];
}

const XVariantType *x_variant_type_value(const XVariantType *type)
{
    const xchar *type_string;

    x_return_val_if_fail(x_variant_type_check(type), NULL);
    type_string = x_variant_type_peek_string(type);
    x_assert(type_string[0] == '{');

    return x_variant_type_next(x_variant_type_key(type));
}

static XVariantType *x_variant_type_new_tuple_slow(const XVariantType *const *items, xint length)
{
    xint i;
    XString *string;

    string = x_string_new("(");
    for (i = 0; i < length; i++) {
        xsize size;
        const XVariantType *type;

        x_return_val_if_fail(x_variant_type_check(items[i]), NULL);

        type = items[i];
        size = x_variant_type_get_string_length(type);
        x_string_append_len(string, (const xchar *)type, size);
    }
    x_string_append_c(string, ')');

    return (XVariantType *)x_string_free(string, FALSE);
}

XVariantType *x_variant_type_new_tuple(const XVariantType *const *items, xint length)
{
    xsize i;
    xsize offset;
    char buffer[1024];
    xsize length_unsigned;

    x_return_val_if_fail(length == 0 || items != NULL, NULL);

    if (length < 0) {
        for (length_unsigned = 0; items[length_unsigned] != NULL; length_unsigned++);
    } else {
        length_unsigned = (xsize)length;
    }

    offset = 0;
    buffer[offset++] = '(';

    for (i = 0; i < length_unsigned; i++) {
        xsize size;
        const XVariantType *type;

        x_return_val_if_fail(x_variant_type_check(items[i]), NULL);

        type = items[i];
        size = x_variant_type_get_string_length (type);

        if (offset + size >= sizeof buffer) {
            return x_variant_type_new_tuple_slow(items, length_unsigned);
        }

        memcpy(&buffer[offset], type, size);
        offset += size;
    }

    x_assert(offset < sizeof buffer);
    buffer[offset++] = ')';

    return (XVariantType *)x_memdup2(buffer, offset);
}

XVariantType *x_variant_type_new_array(const XVariantType *element)
{
    xsize size;
    xchar *newt;

    x_return_val_if_fail(x_variant_type_check(element), NULL);

    size = x_variant_type_get_string_length(element);
    newt = (xchar *)x_malloc(size + 1);

    newt[0] = 'a';
    memcpy(newt + 1, element, size);

    return (XVariantType *)newt;
}

XVariantType *x_variant_type_new_maybe(const XVariantType *element)
{
    xsize size;
    xchar *newt;

    x_return_val_if_fail(x_variant_type_check(element), NULL);

    size = x_variant_type_get_string_length(element);
    newt = (xchar *)x_malloc(size + 1);

    newt[0] = 'm';
    memcpy(newt + 1, element, size);

    return (XVariantType *)newt;
}

XVariantType *x_variant_type_new_dict_entry(const XVariantType *key, const XVariantType *value)
{
    xchar *newt;
    xsize keysize, valsize;

    x_return_val_if_fail(x_variant_type_check(key), NULL);
    x_return_val_if_fail(x_variant_type_check(value), NULL);

    keysize = x_variant_type_get_string_length(key);
    valsize = x_variant_type_get_string_length(value);

    newt = (xchar *)x_malloc(1 + keysize + valsize + 1);

    newt[0] = '{';
    memcpy(newt + 1, key, keysize);
    memcpy(newt + 1 + keysize, value, valsize);
    newt[1 + keysize + valsize] = '}';

    return (XVariantType *)newt;
}

const XVariantType *x_variant_type_checked_(const xchar *type_string)
{
    x_return_val_if_fail(x_variant_type_string_is_valid(type_string), NULL);
    return (const XVariantType *)type_string;
}
