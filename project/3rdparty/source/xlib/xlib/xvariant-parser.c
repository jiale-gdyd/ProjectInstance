#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xerror.h>
#include <xlib/xlib/xquark.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xvariant.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xvarianttype.h>
#include <xlib/xlib/xvariant-core.h>
#include <xlib/xlib/xvariant-internal.h>

X_DEFINE_QUARK(x-variant-parse-error-quark, x_variant_parse_error)

XQuark x_variant_parser_get_error_quark(void)
{
    return x_variant_parse_error_quark();
}

typedef struct {
    xint start, end;
} SourceRef;

X_GNUC_PRINTF(5, 0)
static void parser_set_error_va(XError **error, SourceRef *location, SourceRef *other, xint code, const xchar *format, va_list ap)
{
    XString *msg = x_string_new(NULL);

    if (location->start == location->end) {
        x_string_append_printf(msg, "%d", location->start);
    } else {
        x_string_append_printf(msg, "%d-%d", location->start, location->end);
    }

    if (other != NULL) {
        x_assert(other->start != other->end);
        x_string_append_printf(msg, ",%d-%d", other->start, other->end);
    }
    x_string_append_c(msg, ':');

    x_string_append_vprintf(msg, format, ap);
    x_set_error_literal(error, X_VARIANT_PARSE_ERROR, code, msg->str);
    x_string_free(msg, TRUE);
}

X_GNUC_PRINTF(5, 6)
static void parser_set_error(XError **error, SourceRef *location, SourceRef *other, xint code, const xchar *format, ...)
{
    va_list ap;

    va_start(ap, format);
    parser_set_error_va(error, location, other, code, format, ap);
    va_end(ap);
}

typedef struct {
    const xchar *start;
    const xchar *stream;
    const xchar *end;
    const xchar *thist;
} TokenStream;

X_GNUC_PRINTF(5, 6)
static void token_stream_set_error(TokenStream *stream, XError **error, xboolean this_token, xint code, const xchar *format, ...)
{
    va_list ap;
    SourceRef ref;

    ref.start = stream->thist - stream->start;
    if (this_token) {
        ref.end = stream->stream - stream->start;
    } else {
        ref.end = ref.start;
    }

    va_start(ap, format);
    parser_set_error_va(error, &ref, NULL, code, format, ap);
    va_end(ap);
}

static xboolean token_stream_prepare(TokenStream *stream)
{
    const xchar *end;
    xint brackets = 0;

    if (stream->thist != NULL) {
        return TRUE;
    }

    while (stream->stream != stream->end && x_ascii_isspace(*stream->stream)) {
        stream->stream++;
    }

    if (stream->stream == stream->end || *stream->stream == '\0') {
        stream->thist = stream->stream;
        return FALSE;
    }

    switch (stream->stream[0]) {
        case '-': case '+': case '.': case '0': case '1': case '2':
        case '3': case '4': case '5': case '6': case '7': case '8':
        case '9':
            for (end = stream->stream; end != stream->end; end++) {
                if (!x_ascii_isalnum(*end) && *end != '-' && *end != '+' && *end != '.') {
                    break;
                }
            }
            break;

        case 'b':
            if (stream->stream + 1 != stream->end && (stream->stream[1] == '\'' || stream->stream[1] == '"')) {
                for (end = stream->stream + 2; end != stream->end; end++) {
                    if (*end == stream->stream[1] || *end == '\0' || (*end == '\\' && (++end == stream->end || *end == '\0'))) {
                        break;
                    }
                }

                if (end != stream->end && *end) {
                    end++;
                }
                break;
            }

        X_GNUC_FALLTHROUGH;

        case 'a': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':
            for (end = stream->stream; end != stream->end; end++) {
                if (!x_ascii_isalnum(*end)) {
                    break;
                }
            }
            break;

        case '\'': case '"':
            for (end = stream->stream + 1; end != stream->end; end++) {
                if (*end == stream->stream[0] || *end == '\0' || (*end == '\\' && (++end == stream->end || *end == '\0'))) {
                    break;
                }
            }

            if (end != stream->end && *end) {
                end++;
            }
            break;

        case '@': case '%':
            for (end = stream->stream + 1; end != stream->end && *end != '\0' && *end != ',' && *end != ':' && *end != '>' && *end != ']' && !x_ascii_isspace(*end); end++) {
                if (*end == '(' || *end == '{') {
                    brackets++;
                } else if ((*end == ')' || *end == '}') && !brackets--) {
                    break;
                }
            }
            break;

        default:
            end = stream->stream + 1;
            break;
    }

    stream->thist = stream->stream;
    stream->stream = end;

    x_assert(stream->stream - stream->thist >= 1);
    return TRUE;
}

static void token_stream_next(TokenStream *stream)
{
    stream->thist = NULL;
}

static xboolean token_stream_peek(TokenStream *stream, xchar first_char)
{
    if (!token_stream_prepare(stream)) {
        return FALSE;
    }

    return stream->stream - stream->thist >= 1 && stream->thist[0] == first_char;
}

static xboolean token_stream_peek2(TokenStream *stream, xchar first_char, xchar second_char)
{
    if (!token_stream_prepare(stream)) {
        return FALSE;
    }

    return stream->stream - stream->thist >= 2 && stream->thist[0] == first_char && stream->thist[1] == second_char;
}

static xboolean token_stream_is_keyword(TokenStream *stream)
{
    if (!token_stream_prepare(stream)) {
        return FALSE;
    }

    return stream->stream - stream->thist >= 2 && x_ascii_isalpha(stream->thist[0]) && x_ascii_isalpha(stream->thist[1]);
}

static xboolean token_stream_is_numeric(TokenStream *stream)
{
    if (!token_stream_prepare(stream)) {
        return FALSE;
    }

    return (stream->stream - stream->thist >= 1 && (x_ascii_isdigit(stream->thist[0]) || stream->thist[0] == '-' || stream->thist[0] == '+' || stream->thist[0] == '.'));
}

static xboolean token_stream_peek_string(TokenStream *stream, const xchar *token)
{
    xint length = strlen(token);
    return token_stream_prepare(stream) && stream->stream - stream->thist == length && memcmp(stream->thist, token, length) == 0;
}

static xboolean token_stream_consume(TokenStream *stream, const xchar *token)
{
    if (!token_stream_peek_string(stream, token)) {
        return FALSE;
    }

    token_stream_next(stream);
    return TRUE;
}

static xboolean token_stream_require(TokenStream  *stream, const xchar *token, const xchar *purpose, XError **error)
{
    if (!token_stream_consume(stream, token)) {
        token_stream_set_error(stream, error, FALSE, X_VARIANT_PARSE_ERROR_UNEXPECTED_TOKEN, "expected '%s'%s", token, purpose);
        return FALSE;
    }

    return TRUE;
}

static void token_stream_assert(TokenStream *stream, const xchar *token)
{
    xboolean correct_token X_GNUC_UNUSED;

    correct_token = token_stream_consume(stream, token);
    x_assert(correct_token);
}

static xchar *token_stream_get(TokenStream *stream)
{
    xchar *result;

    if (!token_stream_prepare(stream)) {
        return NULL;
    }

    result = x_strndup(stream->thist, stream->stream - stream->thist);
    return result;
}

static void token_stream_start_ref(TokenStream *stream, SourceRef *ref)
{
    token_stream_prepare(stream);
    ref->start = stream->thist - stream->start;
}

static void token_stream_end_ref(TokenStream *stream, SourceRef *ref)
{
    ref->end = stream->stream - stream->start;
}

static void pattern_copy(xchar **out, const xchar **in)
{
    xint brackets = 0;

    while (**in == 'a' || **in == 'm' || **in == 'M') {
        *(*out)++ = *(*in)++;
    }

    do {
        if (**in == '(' || **in == '{') {
            brackets++;
        } else if (**in == ')' || **in == '}') {
            brackets--;
        }

        *(*out)++ = *(*in)++;
    } while (brackets);
}

static xchar *pattern_coalesce(const xchar *left, const xchar *right)
{
    xchar *out;
    xchar *result;

    out = result = (xchar *)x_malloc(strlen(left) + strlen(right));
    while (*left && *right) {
        if (*left == *right) {
            *out++ = *left++;
            right++;
        } else {
            const xchar **one = &left, **the_other = &right;

again:
            if (**one == '*' && **the_other != ')') {
                pattern_copy(&out, the_other);
                (*one)++;
            } else if (**one == 'M' && **the_other == 'm') {
                *out++ = *(*the_other)++;
            } else if (**one == 'M' && **the_other != 'm' && **the_other != '*') {
                (*one)++;
            } else if (**one == 'N' && strchr("ynqiuxthd", **the_other)) {
                *out++ = *(*the_other)++;
                (*one)++;
            } else if (**one == 'S' && strchr("sog", **the_other)) {
                *out++ = *(*the_other)++;
                (*one)++;
            } else if (one == &left) {
                one = &right, the_other = &left;
                goto again;
             } else {
                break;
            }
        }
    }

    if (*left || *right) {
        x_free(result);
        result = NULL;
    } else {
        *out++ = '\0';
    }

    return result;
}

typedef struct _AST AST;

typedef void (*free_func)(AST *ast);
typedef xchar *(*get_pattern_func)(AST *ast, XError **error);
typedef XVariant *(*get_value_func)(AST *ast, const XVariant *type, XError **error);
typedef XVariant *(*get_base_value_func)(AST *ast, const XVariantType *type, XError **error);

typedef struct {
    xchar *(*get_pattern)(AST *ast, XError **error);
    XVariant *(*get_value)(AST *ast, const XVariantType *type, XError **error);
    XVariant *(*get_base_value)(AST *ast, const XVariantType *type, XError **error);
    void (*free)(AST *ast);
} ASTClass;

struct _AST {
    const ASTClass *classt;
    SourceRef      source_ref;
};

static xchar *ast_get_pattern(AST *ast, XError **error)
{
    return ast->classt->get_pattern(ast, error);
}

static XVariant *ast_get_value(AST *ast, const XVariantType *type, XError **error)
{
    return ast->classt->get_value(ast, type, error);
}

static void ast_free(AST *ast)
{
    ast->classt->free(ast);
}

X_GNUC_PRINTF(5, 6)
static void ast_set_error(AST *ast, XError **error, AST *other_ast, xint code, const xchar *format, ...)
{
    va_list ap;

    va_start(ap, format);
    parser_set_error_va(error, &ast->source_ref, other_ast ? & other_ast->source_ref : NULL, code, format, ap);
    va_end(ap);
}

static XVariant *ast_type_error(AST *ast, const XVariantType *type, XError **error)
{
    xchar *typestr;

    typestr = x_variant_type_dup_string(type);
    ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_TYPE_ERROR, "can not parse as value of type '%s'", typestr);
    x_free(typestr);

    return NULL;
}

static XVariant *ast_resolve(AST *ast, XError **error)
{
    xint i, j = 0;
    xchar *pattern;
    XVariant *value;

    pattern = ast_get_pattern(ast, error);

    if (pattern == NULL) {
        return NULL;
    }

    for (i = 0; pattern[i]; i++) {
        switch (pattern[i]) {
            case '*':
                ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_CANNOT_INFER_TYPE, "unable to infer type");
                x_free(pattern);
                return NULL;

            case 'M':
                break;

            case 'S':
                pattern[j++] = 's';
                break;

            case 'N':
                pattern[j++] = 'i';
                break;

            default:
                pattern[j++] = pattern[i];
                break;
        }
    }
    pattern[j++] = '\0';

    value = ast_get_value(ast, X_VARIANT_TYPE(pattern), error);
    x_free(pattern);

    return value;
}

static AST *parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error);

static void ast_array_append(AST ***array, xint *n_items, AST *ast)
{
    if ((*n_items & (*n_items - 1)) == 0) {
        *array = x_renew(AST *, *array, *n_items ? 2 ** n_items : 1);
    }
    (*array)[(*n_items)++] = ast;
}

static void ast_array_free(AST **array, xint n_items)
{
    xint i;

    for (i = 0; i < n_items; i++) {
        ast_free(array[i]);
    }
    x_free(array);
}

static xchar *ast_array_get_pattern(AST **array, xint n_items, XError **error)
{
    xint i;
    xchar *pattern;

    pattern = ast_get_pattern(array[0], error);

    if (pattern == NULL) {
        return NULL;
    }

    for (i = 1; i < n_items; i++) {
        xchar *tmp, *merged;

        tmp = ast_get_pattern(array[i], error);
        if (tmp == NULL) {
            x_free(pattern);
            return NULL;
        }

        merged = pattern_coalesce(pattern, tmp);
        x_free(pattern);
        pattern = merged;

        if (merged == NULL) {
            int j = 0;

            while (TRUE) {
                xchar *m;
                xchar *tmp2;

                if (j >= i) {
                    ast_set_error(array[i], error, NULL, X_VARIANT_PARSE_ERROR_NO_COMMON_TYPE, "unable to find a common type");
                    x_free(tmp);
                    return NULL;
                }

                tmp2 = ast_get_pattern(array[j], NULL);
                x_assert(tmp2 != NULL);

                m = pattern_coalesce(tmp, tmp2);
                x_free(tmp2);
                x_free(m);

                if (m == NULL) {
                    ast_set_error(array[j], error, array[i], X_VARIANT_PARSE_ERROR_NO_COMMON_TYPE, "unable to find a common type");
                    x_free(tmp);
                    return NULL;
                }

                j++;
            }
        }

        x_free(tmp);
    }

    return pattern;
}

typedef struct {
    AST ast;
    AST *child;
} Maybe;

static xchar *maybe_get_pattern(AST *ast, XError **error)
{
    Maybe *maybe = (Maybe *)ast;

    if (maybe->child != NULL) {
        xchar *pattern;
        xchar *child_pattern;

        child_pattern = ast_get_pattern(maybe->child, error);
        if (child_pattern == NULL) {
            return NULL;
        }

        pattern = x_strdup_printf("m%s", child_pattern);
        x_free(child_pattern);

        return pattern;
    }

    return x_strdup("m*");
}

static XVariant *maybe_get_value(AST *ast, const XVariantType *type, XError **error)
{
    XVariant *value;
    Maybe *maybe = (Maybe *)ast;

    if (!x_variant_type_is_maybe(type)) {
        return ast_type_error(ast, type, error);
    }

    type = x_variant_type_element(type);
    if (maybe->child) {
        value = ast_get_value(maybe->child, type, error);
        if (value == NULL) {
            return NULL;
        }
    } else {
        value = NULL;
    }

    return x_variant_new_maybe(type, value);
}

static void maybe_free(AST *ast)
{
    Maybe *maybe = (Maybe *)ast;

    if (maybe->child != NULL) {
        ast_free(maybe->child);
    }

    x_slice_free(Maybe, maybe);
}

static AST *maybe_parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    static const ASTClass maybe_class = {
        maybe_get_pattern,
        maybe_get_value, NULL,
        maybe_free
    };
    Maybe *maybe;
    AST *child = NULL;

    if (token_stream_consume(stream, "just")) {
        child = parse(stream, max_depth - 1, app, error);
        if (child == NULL) {
            return NULL;
        }
    } else if (!token_stream_consume(stream, "nothing")) {
        token_stream_set_error(stream, error, TRUE, X_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD, "unknown keyword");
        return NULL;
    }

    maybe = x_slice_new(Maybe);
    maybe->ast.classt = &maybe_class;
    maybe->child = child;

    return (AST *) maybe;
}

static XVariant *maybe_wrapper(AST *ast, const XVariantType *type, XError **error)
{
    xsize i;
    xboolean trusted;
    unsigned int depth;
    XBytes *bytes = NULL;
    XVariant *base_value;
    XVariant *value = NULL;
    xuint8 *serialised = NULL;
    const XVariantType *base_type;
    XVariantTypeInfo *base_type_info = NULL;
    xsize base_serialised_fixed_size, base_serialised_size, serialised_size, n_suffix_zeros;

    for (depth = 0, base_type = type; x_variant_type_is_maybe(base_type); depth++, base_type = x_variant_type_element(base_type));

    base_value = ast->classt->get_base_value (ast, base_type, error);
    if ((base_value == NULL) || (depth == 0)) {
        return x_steal_pointer(&base_value);
    }

    trusted = x_variant_is_trusted(base_value);
    base_type_info = x_variant_type_info_get(base_type);
    x_variant_type_info_query(base_type_info, NULL, &base_serialised_fixed_size);
    x_variant_type_info_unref(base_type_info);

    base_serialised_size = x_variant_get_size(base_value);
    n_suffix_zeros = (base_serialised_fixed_size > 0) ? depth - 1 : depth;
    x_assert(base_serialised_size <= X_MAXSIZE - n_suffix_zeros);
    serialised_size = base_serialised_size + n_suffix_zeros;

    x_assert(serialised_size >= base_serialised_size);

    serialised = x_malloc(serialised_size);
    x_variant_store(base_value, serialised);

    for (i = base_serialised_size; i < serialised_size; i++) {
        serialised[i] = 0;
    }

    bytes = x_bytes_new_take(x_steal_pointer(&serialised), serialised_size);
    value = x_variant_new_from_bytes(type, bytes, trusted);

    x_bytes_unref(bytes);
    x_variant_unref(base_value);

    return x_steal_pointer(&value);
}

typedef struct {
    AST  ast;
    AST  **children;
    xint n_children;
} Array;

static xchar *array_get_pattern(AST *ast, XError **error)
{
    xchar *result;
    xchar *pattern;
    Array *array = (Array *)ast;

    if (array->n_children == 0) {
        return x_strdup("Ma*");
    }

    pattern = ast_array_get_pattern(array->children, array->n_children, error);
    if (pattern == NULL) {
        return NULL;
    }

    result = x_strdup_printf("Ma%s", pattern);
    x_free(pattern);

    return result;
}

static XVariant *array_get_value(AST *ast, const XVariantType  *type, XError **error)
{
    xint i;
    XVariantBuilder builder;
    Array *array = (Array *)ast;
    const XVariantType *childtype;

    if (!x_variant_type_is_array(type)) {
        return ast_type_error(ast, type, error);
    }

    x_variant_builder_init(&builder, type);
    childtype = x_variant_type_element(type);

    for (i = 0; i < array->n_children; i++) {
        XVariant *child;

        if (!(child = ast_get_value(array->children[i], childtype, error))) {
            x_variant_builder_clear(&builder);
            return NULL;
        }

        x_variant_builder_add_value(&builder, child);
    }

    return x_variant_builder_end(&builder);
}

static void array_free(AST *ast)
{
    Array *array = (Array *)ast;

    ast_array_free(array->children, array->n_children);
    x_slice_free(Array, array);
}

static AST *array_parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    static const ASTClass array_class = {
        array_get_pattern,
        maybe_wrapper, array_get_value,
        array_free
    };
    xboolean need_comma = FALSE;
    Array *array;

    array = x_slice_new(Array);
    array->ast.classt = &array_class;
    array->children = NULL;
    array->n_children = 0;

    token_stream_assert(stream, "[");
    while (!token_stream_consume(stream, "]")) {
        AST *child;

        if (need_comma && !token_stream_require (stream, ",", " or ']' to follow array element", error)) {
            goto error;
        }

        child = parse(stream, max_depth - 1, app, error);
        if (!child) {
            goto error;
        }

        ast_array_append(&array->children, &array->n_children, child);
        need_comma = TRUE;
    }

    return (AST *)array;

error:
    ast_array_free(array->children, array->n_children);
    x_slice_free(Array, array);

    return NULL;
}

typedef struct {
    AST  ast;
    AST  **children;
    xint n_children;
} Tuple;

static xchar *tuple_get_pattern(AST *ast, XError **error)
{
    xint i;
    xchar **parts;
    xchar *result = NULL;
    Tuple *tuple = (Tuple *)ast;

    parts = x_new(xchar *, tuple->n_children + 4);
    parts[tuple->n_children + 1] = (xchar *) ")";
    parts[tuple->n_children + 2] = NULL;
    parts[0] = (xchar *) "M(";

    for (i = 0; i < tuple->n_children; i++) {
        if (!(parts[i + 1] = ast_get_pattern(tuple->children[i], error))) {
            break;
        }
    }

    if (i == tuple->n_children) {
        result = x_strjoinv("", parts);
    }

    while (i) {
        x_free(parts[i--]);
    }
    x_free(parts);

    return result;
}

static XVariant *tuple_get_value(AST *ast, const XVariantType *type, XError **error)
{
    xint i;
    XVariantBuilder builder;
    Tuple *tuple = (Tuple *)ast;
    const XVariantType *childtype;

    if (!x_variant_type_is_tuple(type)) {
        return ast_type_error(ast, type, error);
    }

    x_variant_builder_init(&builder, type);
    childtype = x_variant_type_first(type);

    for (i = 0; i < tuple->n_children; i++) {
        XVariant *child;

        if (childtype == NULL) {
            x_variant_builder_clear(&builder);
            return ast_type_error(ast, type, error);
        }

        if (!(child = ast_get_value(tuple->children[i], childtype, error))) {
            x_variant_builder_clear(&builder);
            return FALSE;
        }

        x_variant_builder_add_value(&builder, child);
        childtype = x_variant_type_next(childtype);
    }

    if (childtype != NULL) {
        x_variant_builder_clear(&builder);
        return ast_type_error(ast, type, error);
    }

    return x_variant_builder_end(&builder);
}

static void tuple_free(AST *ast)
{
    Tuple *tuple = (Tuple *)ast;

    ast_array_free(tuple->children, tuple->n_children);
    x_slice_free(Tuple, tuple);
}

static AST *tuple_parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    static const ASTClass tuple_class = {
        tuple_get_pattern,
        maybe_wrapper, tuple_get_value,
        tuple_free
    };

    Tuple *tuple;
    xboolean first = TRUE;
    xboolean need_comma = FALSE;

    tuple = x_slice_new(Tuple);
    tuple->ast.classt = &tuple_class;
    tuple->children = NULL;
    tuple->n_children = 0;

    token_stream_assert(stream, "(");
    while (!token_stream_consume(stream, ")")) {
        AST *child;

        if (need_comma && !token_stream_require(stream, ",", " or ')' to follow tuple element", error)) {
            goto error;
        }

        child = parse(stream, max_depth - 1, app, error);
        if (!child) {
            goto error;
        }

        ast_array_append(&tuple->children, &tuple->n_children, child);
        if (first) {
            if (!token_stream_require(stream, ",", " after first tuple element", error)) {
                goto error;
            }

            first = FALSE;
        } else {
            need_comma = TRUE;
        }
    }

    return (AST *) tuple;

 error:
    ast_array_free(tuple->children, tuple->n_children);
    x_slice_free(Tuple, tuple);

    return NULL;
}

typedef struct {
    AST ast;
    AST *value;
} Variant;

static xchar *variant_get_pattern(AST *ast, XError **error)
{
    return x_strdup("Mv");
}

static XVariant *variant_get_value(AST *ast, const XVariantType *type, XError **error)
{
    XVariant *child;
    Variant *variant = (Variant *)ast;

    if (!x_variant_type_equal(type, X_VARIANT_TYPE_VARIANT)) {
        return ast_type_error(ast, type, error);
    }

    child = ast_resolve(variant->value, error);
    if (child == NULL) {
        return NULL;
    }

    return x_variant_new_variant(child);
}

static void variant_free(AST *ast)
{
    Variant *variant = (Variant *)ast;

    ast_free(variant->value);
    x_slice_free(Variant, variant);
}

static AST *variant_parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    static const ASTClass variant_class = {
        variant_get_pattern,
        maybe_wrapper, variant_get_value,
        variant_free
    };

    AST *value;
    Variant *variant;

    token_stream_assert(stream, "<");
    value = parse(stream, max_depth - 1, app, error);
    if (!value) {
        return NULL;
    }

    if (!token_stream_require(stream, ">", " to follow variant value", error)) {
        ast_free(value);
        return NULL;
    }

    variant = x_slice_new(Variant);
    variant->ast.classt = &variant_class;
    variant->value = value;

    return (AST *)variant;
}

typedef struct {
    AST  ast;
    AST  **keys;
    AST  **values;
    xint n_children;
} Dictionary;

static xchar *dictionary_get_pattern(AST *ast, XError **error)
{
    xchar *result;
    xchar key_char;
    xchar *key_pattern;
    xchar *value_pattern;
    Dictionary *dict = (Dictionary *)ast;

    if (dict->n_children == 0) {
        return x_strdup("Ma{**}");
    }

    key_pattern = ast_array_get_pattern(dict->keys, abs(dict->n_children), error);
    if (key_pattern == NULL) {
        return NULL;
    }

    if (key_pattern[0] == 'M') {
        key_char = key_pattern[1];
    } else {
        key_char = key_pattern[0];
    }

    x_free(key_pattern);

    if (!strchr("bynqiuxthdsogNS", key_char)) {
        ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_BASIC_TYPE_EXPECTED, "dictionary keys must have basic types");
        return NULL;
    }

    value_pattern = ast_get_pattern(dict->values[0], error);
    if (value_pattern == NULL) {
        return NULL;
    }

    result = x_strdup_printf("M%s{%XVariant%s}", dict->n_children > 0 ? "a" : "", key_char, value_pattern);
    x_free(value_pattern);

    return result;
}

static XVariant *dictionary_get_value(AST *ast, const XVariantType *type, XError **error)
{
    Dictionary *dict = (Dictionary *)ast;

    if (dict->n_children == -1) {
        XVariant *subvalue;
        XVariantBuilder builder;
        const XVariantType *subtype;

        if (!x_variant_type_is_dict_entry(type)) {
            return ast_type_error(ast, type, error);
        }

        x_variant_builder_init(&builder, type);

        subtype = x_variant_type_key(type);
        if (!(subvalue = ast_get_value(dict->keys[0], subtype, error))) {
            x_variant_builder_clear(&builder);
            return NULL;
        }
        x_variant_builder_add_value(&builder, subvalue);

        subtype = x_variant_type_value(type);
        if (!(subvalue = ast_get_value(dict->values[0], subtype, error))) {
            x_variant_builder_clear(&builder);
            return NULL;
        }
        x_variant_builder_add_value(&builder, subvalue);

        return x_variant_builder_end(&builder);
    } else {
        xint i;
        XVariantBuilder builder;
        const XVariantType *entry, *key, *val;

        if (!x_variant_type_is_subtype_of(type, X_VARIANT_TYPE_DICTIONARY)) {
            return ast_type_error(ast, type, error);
        }

        entry = x_variant_type_element(type);
        key = x_variant_type_key(entry);
        val = x_variant_type_value(entry);

        x_variant_builder_init(&builder, type);

        for (i = 0; i < dict->n_children; i++) {
            XVariant *subvalue;

            x_variant_builder_open(&builder, entry);

            if (!(subvalue = ast_get_value(dict->keys[i], key, error))) {
                x_variant_builder_clear(&builder);
                return NULL;
            }
            x_variant_builder_add_value(&builder, subvalue);

            if (!(subvalue = ast_get_value(dict->values[i], val, error))) {
                x_variant_builder_clear(&builder);
                return NULL;
            }

            x_variant_builder_add_value(&builder, subvalue);
            x_variant_builder_close(&builder);
        }

        return x_variant_builder_end(&builder);
    }
}

static void dictionary_free(AST *ast)
{
    xint n_children;
    Dictionary *dict = (Dictionary *)ast;

    if (dict->n_children > -1) {
        n_children = dict->n_children;
    } else {
        n_children = 1;
    }

    ast_array_free(dict->keys, n_children);
    ast_array_free(dict->values, n_children);
    x_slice_free(Dictionary, dict);
}

static AST *dictionary_parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    static const ASTClass dictionary_class = {
        dictionary_get_pattern,
        maybe_wrapper, dictionary_get_value,
        dictionary_free
    };

    xint n_keys, n_values;
    xboolean only_one;
    Dictionary *dict;
    AST *first;

    dict = x_slice_new(Dictionary);
    dict->ast.classt = &dictionary_class;
    dict->keys = NULL;
    dict->values = NULL;
    n_keys = n_values = 0;

    token_stream_assert(stream, "{");

    if (token_stream_consume(stream, "}")) {
        dict->n_children = 0;
        return (AST *)dict;
    }

    if ((first = parse(stream, max_depth - 1, app, error)) == NULL) {
        goto error;
    }

    ast_array_append(&dict->keys, &n_keys, first);

    only_one = token_stream_consume(stream, ",");
    if (!only_one && !token_stream_require(stream, ":", " or ',' to follow dictionary entry key", error)) {
        goto error;
    }

    if ((first = parse(stream, max_depth - 1, app, error)) == NULL) {
        goto error;
    }

    ast_array_append(&dict->values, &n_values, first);

    if (only_one) {
        if (!token_stream_require(stream, "}", " at end of dictionary entry", error)) {
            goto error;
        }

        x_assert(n_keys == 1 && n_values == 1);
        dict->n_children = -1;

        return (AST *)dict;
    }

    while (!token_stream_consume(stream, "}")) {
        AST *child;

        if (!token_stream_require(stream, ",", " or '}' to follow dictionary entry", error)) {
            goto error;
        }

        child = parse(stream, max_depth - 1, app, error);
        if (!child) {
            goto error;
        }

        ast_array_append(&dict->keys, &n_keys, child);

        if (!token_stream_require(stream, ":", " to follow dictionary entry key", error)) {
            goto error;
        }

        child = parse(stream, max_depth - 1, app, error);
        if (!child) {
            goto error;
        }

        ast_array_append(&dict->values, &n_values, child);
    }

    x_assert(n_keys == n_values);
    dict->n_children = n_keys;

    return (AST *) dict;

error:
    ast_array_free(dict->keys, n_keys);
    ast_array_free(dict->values, n_values);
    x_slice_free(Dictionary, dict);

    return NULL;
}

typedef struct {
    AST   ast;
    xchar *string;
} String;

static xchar *string_get_pattern(AST *ast, XError **error)
{
    return x_strdup("MS");
}

static XVariant *string_get_value(AST *ast, const XVariantType *type, XError **error)
{
    String *string = (String *)ast;

    if (x_variant_type_equal(type, X_VARIANT_TYPE_STRING)) {
        return x_variant_new_string(string->string);
    } else if (x_variant_type_equal(type, X_VARIANT_TYPE_OBJECT_PATH)) {
        if (!x_variant_is_object_path(string->string)) {
            ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_INVALID_OBJECT_PATH, "not a valid object path");
            return NULL;
        }

        return x_variant_new_object_path(string->string);
    } else if (x_variant_type_equal(type, X_VARIANT_TYPE_SIGNATURE)) {
        if (!x_variant_is_signature(string->string)) {
            ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_INVALID_SIGNATURE, "not a valid signature");
            return NULL;
        }

        return x_variant_new_signature(string->string);
    } else {
        return ast_type_error(ast, type, error);
    }
}

static void string_free(AST *ast)
{
    String *string = (String *)ast;

    x_free(string->string);
    x_slice_free(String, string);
}

static xboolean unicode_unescape(const xchar *src, xint *src_ofs, xchar *dest, xint *dest_ofs, xsize length, SourceRef *ref, XError **error)
{
    xchar buffer[9];
    xuint64 value = 0;
    xchar *end = NULL;
    xsize n_valid_chars;

    (*src_ofs)++;

    x_assert(length < sizeof(buffer));
    strncpy(buffer, src + *src_ofs, length);
    buffer[length] = '\0';

    for (n_valid_chars = 0; n_valid_chars < length; n_valid_chars++) {
        if (!x_ascii_isxdigit(buffer[n_valid_chars])) {
            break;
        }
    }

    if (n_valid_chars == length) {
        value = x_ascii_strtoull(buffer, &end, 0x10);
    }

    if (value == 0 || end != buffer + length) {
        SourceRef escape_ref;

        escape_ref = *ref;
        escape_ref.start += *src_ofs;
        escape_ref.end = escape_ref.start + n_valid_chars;

        parser_set_error(error, &escape_ref, NULL, X_VARIANT_PARSE_ERROR_INVALID_CHARACTER, "invalid %" X_XSIZE_FORMAT "-character unicode escape", length);
        return FALSE;
    }

    x_assert(value <= X_MAXUINT32);

    *dest_ofs += x_unichar_to_utf8(value, dest + *dest_ofs);
    *src_ofs += length;

    return TRUE;
}

static AST *string_parse(TokenStream *stream, va_list *app, XError **error)
{
    static const ASTClass string_class = {
        string_get_pattern,
        maybe_wrapper, string_get_value,
        string_free
    };

    xint i, j;
    xchar *str;
    xchar quote;
    xchar *token;
    xsize length;
    SourceRef ref;
    String *string;

    token_stream_start_ref(stream, &ref);
    token = token_stream_get(stream);
    token_stream_end_ref(stream, &ref);
    length = strlen(token);
    quote = token[0];

    str = (xchar *)x_malloc(length);
    x_assert(quote == '"' || quote == '\'');
    j = 0;
    i = 1;

    while (token[i] != quote) {
        switch (token[i]) {
            case '\0':
                parser_set_error(error, &ref, NULL, X_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT, "unterminated string constant");
                x_free(token);
                x_free(str);
                return NULL;

            case '\\':
                switch (token[++i]) {
                    case '\0':
                        parser_set_error(error, &ref, NULL, X_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT, "unterminated string constant");
                        x_free(token);
                        x_free(str);
                        return NULL;

                    case 'u':
                        if (!unicode_unescape(token, &i, str, &j, 4, &ref, error)) {
                            x_free(token);
                            x_free(str);
                            return NULL;
                        }
                        continue;

                    case 'U':
                        if (!unicode_unescape(token, &i, str, &j, 8, &ref, error)) {
                            x_free(token);
                            x_free(str);
                            return NULL;
                        }
                        continue;

                    case 'a': str[j++] = '\a'; i++; continue;
                    case 'b': str[j++] = '\b'; i++; continue;
                    case 'f': str[j++] = '\f'; i++; continue;
                    case 'n': str[j++] = '\n'; i++; continue;
                    case 'r': str[j++] = '\r'; i++; continue;
                    case 't': str[j++] = '\t'; i++; continue;
                    case 'v': str[j++] = '\v'; i++; continue;
                    case '\n': i++; continue;
                }

                X_GNUC_FALLTHROUGH;

            default:
                str[j++] = token[i++];
        }
    }

    str[j++] = '\0';
    x_free(token);

    string = x_slice_new(String);
    string->ast.classt = &string_class;
    string->string = str;

    token_stream_next(stream);
    return (AST *)string;
}

typedef struct {
    AST   ast;
    xchar *string;
} ByteString;

static xchar *bytestring_get_pattern(AST *ast, XError **error)
{
    return x_strdup("May");
}

static XVariant *bytestring_get_value(AST *ast, const XVariantType *type, XError **error)
{
    ByteString *string = (ByteString *) ast;

    if (!x_variant_type_equal(type, X_VARIANT_TYPE_BYTESTRING)) {
        return ast_type_error(ast, type, error);
    }

    return x_variant_new_bytestring(string->string);
}

static void bytestring_free(AST *ast)
{
    ByteString *string = (ByteString *)ast;

    x_free(string->string);
    x_slice_free(ByteString, string);
}

static AST *bytestring_parse(TokenStream *stream, va_list *app, XError **error)
{
    static const ASTClass bytestring_class = {
        bytestring_get_pattern,
        maybe_wrapper, bytestring_get_value,
        bytestring_free
    };

    xint i, j;
    xchar *str;
    xchar quote;
    xchar *token;
    xsize length;
    SourceRef ref;
    ByteString *string;

    token_stream_start_ref(stream, &ref);
    token = token_stream_get(stream);
    token_stream_end_ref(stream, &ref);
    x_assert(token[0] == 'b');
    length = strlen(token);
    quote = token[1];

    str = (xchar *)x_malloc(length);
    x_assert(quote == '"' || quote == '\'');
    j = 0;
    i = 2;

    while (token[i] != quote) {
        switch (token[i]) {
            case '\0':
                parser_set_error(error, &ref, NULL, X_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT, "unterminated string constant");
                x_free(str);
                x_free(token);
                return NULL;

            case '\\':
                switch (token[++i]) {
                    case '\0':
                        parser_set_error(error, &ref, NULL, X_VARIANT_PARSE_ERROR_UNTERMINATED_STRING_CONSTANT, "unterminated string constant");
                        x_free(str);
                        x_free(token);
                        return NULL;

                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7': {
                        xuchar val = token[i++] - '0';

                        if ('0' <= token[i] && token[i] < '8') {
                            val = (val << 3) | (token[i++] - '0');
                        }

                        if ('0' <= token[i] && token[i] < '8') {
                            val = (val << 3) | (token[i++] - '0');
                        }
                        str[j++] = val;
                    }
                    continue;

                    case 'a': str[j++] = '\a'; i++; continue;
                    case 'b': str[j++] = '\b'; i++; continue;
                    case 'f': str[j++] = '\f'; i++; continue;
                    case 'n': str[j++] = '\n'; i++; continue;
                    case 'r': str[j++] = '\r'; i++; continue;
                    case 't': str[j++] = '\t'; i++; continue;
                    case 'v': str[j++] = '\v'; i++; continue;
                    case '\n': i++; continue;
                }

                X_GNUC_FALLTHROUGH;

            default:
                str[j++] = token[i++];
        }
    }

    str[j++] = '\0';
    x_free(token);

    string = x_slice_new(ByteString);
    string->ast.classt = &bytestring_class;
    string->string = str;

    token_stream_next(stream);
    return (AST *)string;
}

typedef struct {
    AST   ast;
    xchar *token;
} Number;

static xchar *number_get_pattern(AST *ast, XError **error)
{
    Number *number = (Number *)ast;

    if (strchr(number->token, '.')
        || (!x_str_has_prefix(number->token, "0x") && strchr(number->token, 'e'))
        || strstr(number->token, "inf")
        || strstr(number->token, "nan"))
    {
        return x_strdup("Md");
    }

    return x_strdup("MN");
}

static XVariant *number_overflow(AST *ast, const XVariantType *type, XError **error)
{
    ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_NUMBER_OUT_OF_RANGE, "number out of range for type '%XVariant'", x_variant_type_peek_string(type)[0]);
    return NULL;
}

static XVariant *number_get_value(AST *ast, const XVariantType *type, XError **error)
{
    xchar *end;
    xuint64 abs_val;
    xdouble dbl_val;
    xboolean negative;
    xboolean floating;
    const xchar *token;
    Number *number = (Number *)ast;

    token = number->token;
    if (x_variant_type_equal(type, X_VARIANT_TYPE_DOUBLE)) {
        floating = TRUE;

        errno = 0;
        dbl_val = x_ascii_strtod(token, &end);
        if (dbl_val != 0.0 && errno == ERANGE) {
            ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG, "number too big for any type");
            return NULL;
        }

        negative = FALSE;
        abs_val = 0;
    } else {
        floating = FALSE;
        negative = token[0] == '-';
        if (token[0] == '-') {
            token++;
        }

        errno = 0;
        abs_val = x_ascii_strtoull(token, &end, 0);
        if (abs_val == X_MAXUINT64 && errno == ERANGE) {
            ast_set_error(ast, error, NULL, X_VARIANT_PARSE_ERROR_NUMBER_TOO_BIG, "integer too big for any type");
            return NULL;
        }

        if (abs_val == 0) {
            negative = FALSE;
        }

        dbl_val = 0.0;
    }

    if (*end != '\0') {
        SourceRef ref;

        ref = ast->source_ref;
        ref.start += end - number->token;
        ref.end = ref.start + 1;

        parser_set_error(error, &ref, NULL, X_VARIANT_PARSE_ERROR_INVALID_CHARACTER, "invalid character in number");
        return NULL;
    }

    if (floating) {
        return x_variant_new_double(dbl_val);
    }

    switch (*x_variant_type_peek_string(type)) {
        case 'y':
            if (negative || abs_val > X_MAXUINT8) {
                return number_overflow(ast, type, error);
            }
            return x_variant_new_byte(abs_val);

        case 'n':
            if (abs_val - negative > X_MAXINT16) {
                return number_overflow(ast, type, error);
            }

            if (negative && abs_val > X_MAXINT16) {
                return x_variant_new_int16(X_MININT16);
            }
            return x_variant_new_int16(negative ? -((xint16) abs_val) : ((xint16)abs_val));

        case 'q':
            if (negative || abs_val > X_MAXUINT16) {
                return number_overflow(ast, type, error);
            }
            return x_variant_new_uint16(abs_val);

        case 'i':
            if (abs_val - negative > X_MAXINT32) {
                return number_overflow(ast, type, error);
            }

            if (negative && abs_val > X_MAXINT32) {
                return x_variant_new_int32(X_MININT32);
            }
            return x_variant_new_int32(negative ? -((xint32) abs_val) : ((xint32)abs_val));

        case 'u':
            if (negative || abs_val > X_MAXUINT32) {
                return number_overflow(ast, type, error);
            }
            return x_variant_new_uint32(abs_val);

        case 'x':
            if (abs_val - negative > X_MAXINT64) {
                return number_overflow(ast, type, error);
            }

            if (negative && abs_val > X_MAXINT64) {
                return x_variant_new_int64(X_MININT64);
            }
            return x_variant_new_int64(negative ? -((xint64)abs_val) : ((xint64)abs_val));

        case 't':
            if (negative) {
                return number_overflow(ast, type, error);
            }
            return x_variant_new_uint64(abs_val);

        case 'h':
            if (abs_val - negative > X_MAXINT32) {
                return number_overflow(ast, type, error);
            }

            if (negative && abs_val > X_MAXINT32) {
                return x_variant_new_handle(X_MININT32);
            }
            return x_variant_new_handle(negative ? -((xint32)abs_val) : ((xint32)abs_val));

        default:
            return ast_type_error(ast, type, error);
    }
}

static void number_free(AST *ast)
{
    Number *number = (Number *)ast;

    x_free(number->token);
    x_slice_free(Number, number);
}

static AST *number_parse(TokenStream *stream, va_list *app, XError **error)
{
    static const ASTClass number_class = {
        number_get_pattern,
        maybe_wrapper, number_get_value,
        number_free
    };
    Number *number;

    number = x_slice_new(Number);
    number->ast.classt = &number_class;
    number->token = token_stream_get(stream);
    token_stream_next (stream);

    return (AST *) number;
}

typedef struct {
    AST      ast;
    xboolean value;
} Boolean;

static xchar *boolean_get_pattern(AST *ast, XError **error)
{
    return x_strdup("Mb");
}

static XVariant *boolean_get_value(AST *ast, const XVariantType *type, XError **error)
{
    Boolean *boolean = (Boolean *)ast;

    if (!x_variant_type_equal(type, X_VARIANT_TYPE_BOOLEAN)) {
        return ast_type_error(ast, type, error);
    }

    return x_variant_new_boolean(boolean->value);
}

static void boolean_free(AST *ast)
{
    Boolean *boolean = (Boolean *)ast;
    x_slice_free(Boolean, boolean);
}

static AST *boolean_new(xboolean value)
{
    static const ASTClass boolean_class = {
        boolean_get_pattern,
        maybe_wrapper, boolean_get_value,
        boolean_free
    };

    Boolean *boolean;
    boolean = x_slice_new(Boolean);
    boolean->ast.classt = &boolean_class;
    boolean->value = value;

    return (AST *)boolean;
}

typedef struct {
    AST      ast;
    XVariant *value;
} Positional;

static xchar *positional_get_pattern(AST *ast, XError **error)
{
    Positional *positional = (Positional *)ast;
    return x_strdup(x_variant_get_type_string(positional->value));
}

static XVariant *positional_get_value(AST *ast, const XVariantType *type, XError **error)
{
    XVariant *value;
    Positional *positional = (Positional *)ast;

    x_assert(positional->value != NULL);

    if X_UNLIKELY(!x_variant_is_of_type(positional->value, type)) {
        return ast_type_error(ast, type, error);
    }

    x_assert(positional->value != NULL);
    value = positional->value;
    positional->value = NULL;

    return value;
}

static void positional_free (AST *ast)
{
    Positional *positional = (Positional *)ast;
    x_slice_free(Positional, positional);
}

static AST *positional_parse(TokenStream *stream, va_list *app, XError **error)
{
    static const ASTClass positional_class = {
        positional_get_pattern,
        positional_get_value, NULL,
        positional_free
    };

    xchar *token;
    const xchar *endptr;
    Positional *positional;

    token = token_stream_get(stream);
    x_assert(token[0] == '%');

    positional = x_slice_new(Positional);
    positional->ast.classt = &positional_class;
    positional->value = x_variant_new_va(token + 1, &endptr, app);

    if (*endptr || positional->value == NULL) {
        token_stream_set_error(stream, error, TRUE, X_VARIANT_PARSE_ERROR_INVALID_FORMAT_STRING, "invalid XVariant format string");
        return NULL;
    }

    token_stream_next(stream);
    x_free(token);

    return (AST *)positional;
}

typedef struct {
    AST          ast;
    XVariantType *type;
    AST          *child;
} TypeDecl;

static xchar *typedecl_get_pattern(AST *ast, XError **error)
{
    TypeDecl *decl = (TypeDecl *)ast;
    return x_variant_type_dup_string(decl->type);
}

static XVariant *typedecl_get_value(AST *ast, const XVariantType *type, XError **error)
{
    TypeDecl *decl = (TypeDecl *)ast;
    return ast_get_value(decl->child, type, error);
}

static void typedecl_free(AST *ast)
{
    TypeDecl *decl = (TypeDecl *)ast;

    ast_free(decl->child);
    x_variant_type_free(decl->type);
    x_slice_free(TypeDecl, decl);
}

static AST *typedecl_parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    static const ASTClass typedecl_class = {
        typedecl_get_pattern,
        typedecl_get_value, NULL,
        typedecl_free
    };

    AST *child;
    TypeDecl *decl;
    XVariantType *type;

    if (token_stream_peek(stream, '@')) {
        xchar *token;

        token = token_stream_get(stream);
        if (!x_variant_type_string_is_valid(token + 1)) {
            token_stream_set_error(stream, error, TRUE, X_VARIANT_PARSE_ERROR_INVALID_TYPE_STRING, "invalid type declaration");
            x_free(token);

            return NULL;
        }

        if (x_variant_type_string_get_depth_(token + 1) > max_depth) {
            token_stream_set_error(stream, error, TRUE, X_VARIANT_PARSE_ERROR_RECURSION, "type declaration recurses too deeply");
            x_free(token);

            return NULL;
        }

        type = x_variant_type_new(token + 1);
        if (!x_variant_type_is_definite(type)) {
            token_stream_set_error(stream, error, TRUE, X_VARIANT_PARSE_ERROR_DEFINITE_TYPE_EXPECTED, "type declarations must be definite");
            x_variant_type_free(type);
            x_free(token);

            return NULL;
        }

        token_stream_next(stream);
        x_free(token);
    } else {
        if (token_stream_consume (stream, "boolean")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_BOOLEAN);
        } else if (token_stream_consume(stream, "byte")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_BYTE);
        } else if (token_stream_consume(stream, "int16")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_INT16);
        } else if (token_stream_consume(stream, "uint16")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_UINT16);
        } else if (token_stream_consume(stream, "int32")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_INT32);
        } else if (token_stream_consume(stream, "handle")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_HANDLE);
        } else if (token_stream_consume(stream, "uint32")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_UINT32);
        } else if (token_stream_consume(stream, "int64")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_INT64);
        } else if (token_stream_consume(stream, "uint64")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_UINT64);
        } else if (token_stream_consume(stream, "double")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_DOUBLE);
        } else if (token_stream_consume(stream, "string")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_STRING);
        } else if (token_stream_consume(stream, "objectpath")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_OBJECT_PATH);
        } else if (token_stream_consume(stream, "signature")) {
            type = x_variant_type_copy(X_VARIANT_TYPE_SIGNATURE);
        } else {
            token_stream_set_error(stream, error, TRUE, X_VARIANT_PARSE_ERROR_UNKNOWN_KEYWORD, "unknown keyword");
            return NULL;
        }
    }

    if ((child = parse(stream, max_depth - 1, app, error)) == NULL) {
        x_variant_type_free(type);
        return NULL;
    }

    decl = x_slice_new(TypeDecl);
    decl->ast.classt = &typedecl_class;
    decl->type = type;
    decl->child = child;

    return (AST *)decl;
}

static AST *parse(TokenStream *stream, xuint max_depth, va_list *app, XError **error)
{
    AST *result;
    SourceRef source_ref;

    if (max_depth == 0) {
        token_stream_set_error(stream, error, FALSE, X_VARIANT_PARSE_ERROR_RECURSION, "variant nested too deeply");
        return NULL;
    }

    token_stream_prepare(stream);
    token_stream_start_ref(stream, &source_ref);

    if (token_stream_peek(stream, '[')) {
        result = array_parse(stream, max_depth, app, error);
    } else if (token_stream_peek(stream, '(')) {
        result = tuple_parse(stream, max_depth, app, error);
    } else if (token_stream_peek(stream, '<')) {
        result = variant_parse(stream, max_depth, app, error);
    } else if (token_stream_peek(stream, '{')) {
        result = dictionary_parse(stream, max_depth, app, error);
    } else if (app && token_stream_peek(stream, '%')) {
        result = positional_parse(stream, app, error);
    } else if (token_stream_consume(stream, "true")) {
        result = boolean_new(TRUE);
    } else if (token_stream_consume(stream, "false")) {
        result = boolean_new(FALSE);
    } else if (token_stream_is_numeric(stream) || token_stream_peek_string(stream, "inf") || token_stream_peek_string(stream, "nan")) {
        result = number_parse(stream, app, error);
    } else if (token_stream_peek(stream, 'n') || token_stream_peek(stream, 'j')) {
        result = maybe_parse(stream, max_depth, app, error);
    } else if (token_stream_peek(stream, '@') || token_stream_is_keyword(stream)) {
        result = typedecl_parse(stream, max_depth, app, error);
    } else if (token_stream_peek(stream, '\'') || token_stream_peek(stream, '"')) {
        result = string_parse(stream, app, error);
    } else if (token_stream_peek2(stream, 'b', '\'') || token_stream_peek2(stream, 'b', '"')) {
        result = bytestring_parse(stream, app, error);
    } else {
        token_stream_set_error(stream, error, FALSE, X_VARIANT_PARSE_ERROR_VALUE_EXPECTED, "expected value");
        return NULL;
    }

    if (result != NULL) {
        token_stream_end_ref(stream, &source_ref);
        result->source_ref = source_ref;
    }

    return result;
}

XVariant *x_variant_parse(const XVariantType *type, const xchar *text, const xchar *limit, const xchar **endptr, XError **error)
{
    AST *ast;
    XVariant *result = NULL;
    TokenStream stream = { 0, };

    x_return_val_if_fail(text != NULL, NULL);
    x_return_val_if_fail(text == limit || text != NULL, NULL);

    stream.start = text;
    stream.stream = text;
    stream.end = limit;

    if ((ast = parse(&stream, X_VARIANT_MAX_RECURSION_DEPTH, NULL, error))) {
        if (type == NULL) {
            result = ast_resolve(ast, error);
        } else {
            result = ast_get_value(ast, type, error);
        }

        if (result != NULL) {
            x_variant_ref_sink(result);
            if (endptr == NULL) {
                while (stream.stream != limit && x_ascii_isspace(*stream.stream)) {
                    stream.stream++;
                }

                if (stream.stream != limit && *stream.stream != '\0') {
                    SourceRef ref = { stream.stream - text, stream.stream - text };

                    parser_set_error(error, &ref, NULL, X_VARIANT_PARSE_ERROR_INPUT_NOT_AT_END, "expected end of input");
                    x_variant_unref(result);

                    result = NULL;
                }
            } else {
                *endptr = stream.stream;
            }
        }

        ast_free(ast);
    }

    return result;
}

XVariant *x_variant_new_parsed_va(const xchar *format, va_list *app)
{
    AST *ast;
    XError *error = NULL;
    XVariant *result = NULL;
    TokenStream stream = { 0, };

    x_return_val_if_fail(format != NULL, NULL);
    x_return_val_if_fail(app != NULL, NULL);

    stream.start = format;
    stream.stream = format;
    stream.end = NULL;

    if ((ast = parse(&stream, X_VARIANT_MAX_RECURSION_DEPTH, app, &error))) {
        result = ast_resolve(ast, &error);
        ast_free(ast);
    }

    if (error != NULL) {
        x_error("x_variant_new_parsed: %s", error->message);
    }

    if (*stream.stream) {
        x_error("x_variant_new_parsed: trailing text after value");
    }

    x_clear_error(&error);
    return result;
}

XVariant *x_variant_new_parsed(const xchar *format, ...)
{
    va_list ap;
    XVariant *result;

    va_start(ap, format);
    result = x_variant_new_parsed_va(format, &ap);
    va_end(ap);

    return result;
}

void x_variant_builder_add_parsed(XVariantBuilder *builder, const xchar *format, ...)
{
    va_list ap;

    va_start(ap, format);
    x_variant_builder_add_value(builder, x_variant_new_parsed_va(format, &ap));
    va_end(ap);
}

static xboolean parse_num(const xchar *num, const xchar *limit, xuint *result)
{
    xchar *endptr;
    xint64 bignum;

    bignum = x_ascii_strtoll(num, &endptr, 10);
    if (endptr != limit) {
        return FALSE;
    }

    if (bignum < 0 || bignum > X_MAXINT) {
        return FALSE;
    }
    *result = (xuint)bignum;

    return TRUE;
}

static void add_last_line(XString *err, const xchar *str)
{
    xint i;
    xchar *chomped;
    const xchar *last_nl;

    chomped = x_strchomp(x_strdup(str));
    last_nl = strrchr(chomped, '\n');
    if (last_nl == NULL) {
        last_nl = chomped;
    } else {
        last_nl++;
    }

    x_string_append(err, "  ");
    if (last_nl[0]) {
        x_string_append(err, last_nl);
    } else {
        x_string_append(err, "(empty input)");
    }

    x_string_append(err, "\n  ");
    for (i = 0; last_nl[i]; i++) {
        x_string_append_c(err, ' ');
    }
    x_string_append(err, "^\n");
    x_free(chomped);
}

static void add_lines_from_range(XString *err, const xchar *str, const xchar *start1, const xchar *end1, const xchar *start2, const xchar *end2)
{
    while (str < end1 || str < end2) {
        const xchar *nl;

        nl = str + strcspn(str, "\n");
        if ((start1 < nl && str < end1) || (start2 < nl && str < end2)) {
            const xchar *s;

            x_string_append(err, "  ");
            x_string_append_len(err, str, nl - str);
            x_string_append(err, "\n  ");

            for (s = str; s < nl; s++) {
                if ((start1 <= s && s < end1) || (start2 <= s && s < end2)) {
                    x_string_append_c(err, '^');
                } else {
                    x_string_append_c(err, ' ');
                }
            }

            x_string_append_c(err, '\n');
        }

        if (!*nl) {
            break;
        }

        str = nl + 1;
    }
}

xchar *x_variant_parse_error_print_context(XError *error, const xchar *source_str)
{
    XString *err;
    xboolean success = FALSE;
    const xchar *colon, *dash, *comma;

    x_return_val_if_fail(error->domain == X_VARIANT_PARSE_ERROR, FALSE);

    colon = strchr(error->message, ':');
    dash = strchr(error->message, '-');
    comma = strchr(error->message, ',');

    if (!colon) {
        return NULL;
    }

    err = x_string_new(colon + 1);
    x_string_append(err, ":\n");

    if (dash == NULL || colon < dash) {
        xuint point;

        if (!parse_num(error->message, colon, &point)) {
            goto out;
        }

        if (point >= strlen(source_str)) {
            add_last_line(err, source_str);
        } else {
            add_lines_from_range(err, source_str, source_str + point, source_str + point + 1, NULL, NULL);
        }
    } else {
        if (comma && comma < colon) {
            const xchar *dash2;
            xuint start1, end1, start2, end2;

            dash2 = strchr(comma, '-');

            if (!parse_num(error->message, dash, &start1) || !parse_num(dash + 1, comma, &end1)
                || !parse_num(comma + 1, dash2, &start2) || !parse_num(dash2 + 1, colon, &end2))
            {
                goto out;
            }

            add_lines_from_range(err, source_str, source_str + start1, source_str + end1, source_str + start2, source_str + end2);
        } else {
            xuint start, end;
            if (!parse_num(error->message, dash, &start) || !parse_num(dash + 1, colon, &end)) {
                goto out;
            }

            add_lines_from_range(err, source_str, source_str + start, source_str + end, NULL, NULL);
        }
    }

    success = TRUE;
out:
    return x_string_free(err, !success);
}
