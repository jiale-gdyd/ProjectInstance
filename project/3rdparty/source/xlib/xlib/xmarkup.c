#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xmarkup.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

X_DEFINE_QUARK(x-markup-error-quark, x_markup_error)

typedef enum {
    STATE_START,
    STATE_AFTER_OPEN_ANGLE,
    STATE_AFTER_CLOSE_ANGLE,
    STATE_AFTER_ELISION_SLASH,
    STATE_INSIDE_OPEN_TAG_NAME,
    STATE_INSIDE_ATTRIBUTE_NAME,
    STATE_AFTER_ATTRIBUTE_NAME,
    STATE_BETWEEN_ATTRIBUTES,
    STATE_AFTER_ATTRIBUTE_EQUALS_SIGN,
    STATE_INSIDE_ATTRIBUTE_VALUE_SQ,
    STATE_INSIDE_ATTRIBUTE_VALUE_DQ,
    STATE_INSIDE_TEXT,
    STATE_AFTER_CLOSE_TAG_SLASH,
    STATE_INSIDE_CLOSE_TAG_NAME,
    STATE_AFTER_CLOSE_TAG_NAME,
    STATE_INSIDE_PASSTHROUGH,
    STATE_ERROR
} XMarkupParseState;

typedef struct {
    const char          *prev_element;
    const XMarkupParser *prev_parser;
    xpointer            prev_user_data;
} XMarkupRecursionTracker;

struct _XMarkupParseContext {
    const XMarkupParser *parser;
    xint                ref_count;
    XMarkupParseFlags   flags;
    xint                line_number;
    xint                char_number;
    XMarkupParseState   state;
    xpointer            user_data;
    XDestroyNotify      dnotify;
    XString             *partial_chunk;
    XSList              *spare_chunks;
    XSList              *tag_stack;
    XSList              *tag_stack_gstr;
    XSList              *spare_list_nodes;
    XString             **attr_names;
    XString             **attr_values;
    xint                cur_attr;
    xint                alloc_attrs;
    const xchar         *current_text;
    xssize              current_text_len;
    const xchar         *current_text_end;
    const xchar         *start;
    const xchar         *iter;
    xuint               document_empty : 1;
    xuint               parsing : 1;
    xuint               awaiting_pop : 1;
    xint                balance;
    XSList              *subparser_stack;
    const char          *subparser_element;
    xpointer            held_user_data;
};

static XSList *get_list_node(XMarkupParseContext *context, xpointer data)
{
    XSList *node;
    if (context->spare_list_nodes != NULL) {
        node = context->spare_list_nodes;
        context->spare_list_nodes = x_slist_remove_link(context->spare_list_nodes, node);
    } else {
        node = x_slist_alloc();
    }

    node->data = data;
    return node;
}

static void free_list_node(XMarkupParseContext *context, XSList *node)
{
    node->data = NULL;
    context->spare_list_nodes = x_slist_concat(node, context->spare_list_nodes);
}

XMarkupParseContext *x_markup_parse_context_new(const XMarkupParser *parser, XMarkupParseFlags flags, xpointer user_data, XDestroyNotify user_data_dnotify)
{
    XMarkupParseContext *context;

    x_return_val_if_fail(parser != NULL, NULL);

    context = x_new(XMarkupParseContext, 1);

    context->ref_count = 1;
    context->parser = parser;
    context->flags = flags;
    context->user_data = user_data;
    context->dnotify = user_data_dnotify;

    context->line_number = 1;
    context->char_number = 1;

    context->partial_chunk = NULL;
    context->spare_chunks = NULL;
    context->spare_list_nodes = NULL;

    context->state = STATE_START;
    context->tag_stack = NULL;
    context->tag_stack_gstr = NULL;
    context->attr_names = NULL;
    context->attr_values = NULL;
    context->cur_attr = -1;
    context->alloc_attrs = 0;

    context->current_text = NULL;
    context->current_text_len = -1;
    context->current_text_end = NULL;

    context->start = NULL;
    context->iter = NULL;

    context->document_empty = TRUE;
    context->parsing = FALSE;

    context->awaiting_pop = FALSE;
    context->subparser_stack = NULL;
    context->subparser_element = NULL;

    context->held_user_data = NULL;
    context->balance = 0;

    return context;
}

XMarkupParseContext *x_markup_parse_context_ref(XMarkupParseContext *context)
{
    x_return_val_if_fail(context != NULL, NULL);
    x_return_val_if_fail(context->ref_count > 0, NULL);

    x_atomic_int_inc(&context->ref_count);
    return context;
}

void x_markup_parse_context_unref(XMarkupParseContext *context)
{
    x_return_if_fail(context != NULL);
    x_return_if_fail(context->ref_count > 0);

    if (x_atomic_int_dec_and_test(&context->ref_count)) {
        x_markup_parse_context_free(context);
    }
}

static void string_full_free(xpointer ptr)
{
    x_string_free((XString *)ptr, TRUE);
}

static void clear_attributes(XMarkupParseContext *context);

void x_markup_parse_context_free(XMarkupParseContext *context)
{
    x_return_if_fail(context != NULL);
    x_return_if_fail(!context->parsing);
    x_return_if_fail(!context->subparser_stack);
    x_return_if_fail(!context->awaiting_pop);

    if (context->dnotify) {
        (* context->dnotify)(context->user_data);
    }

    clear_attributes(context);
    x_free(context->attr_names);
    x_free(context->attr_values);

    x_slist_free_full(context->tag_stack_gstr, string_full_free);
    x_slist_free(context->tag_stack);

    x_slist_free_full(context->spare_chunks, string_full_free);
    x_slist_free(context->spare_list_nodes);

    if (context->partial_chunk) {
        x_string_free(context->partial_chunk, TRUE);
    }

    x_free(context);
}

static void pop_subparser_stack(XMarkupParseContext *context);

static void mark_error(XMarkupParseContext *context, XError *error)
{
    context->state = STATE_ERROR;

    if (context->parser->error) {
        (*context->parser->error)(context, error, context->user_data);
    }

    while (context->subparser_stack) {
        pop_subparser_stack(context);
        context->awaiting_pop = FALSE;

        if (context->parser->error) {
            (*context->parser->error)(context, error, context->user_data);
        }
    }
}

static void set_error(XMarkupParseContext *context, XError **error, XMarkupError code, const xchar *format, ...) X_GNUC_PRINTF (4, 5);

static void set_error_literal(XMarkupParseContext *context, XError **error, XMarkupError code, const xchar *message)
{
    XError *tmp_error;

    tmp_error = x_error_new_literal(X_MARKUP_ERROR, code, message);
    x_prefix_error(&tmp_error, _("Error on line %d char %d: "), context->line_number, context->char_number);
    mark_error(context, tmp_error);

    x_propagate_error(error, tmp_error);
}

X_GNUC_PRINTF(4, 5)
static void set_error(XMarkupParseContext *context, XError **error, XMarkupError code, const xchar *format, ...)
{
    xchar *s;
    va_list args;
    xchar *s_valid;

    va_start(args, format);
    s = x_strdup_vprintf(format, args);
    va_end(args);

    s_valid = x_utf8_make_valid(s, -1);
    set_error_literal(context, error, code, s);

    x_free(s);
    x_free(s_valid);
}

static void propagate_error(XMarkupParseContext *context, XError **dest, XError *src)
{
    if (context->flags & X_MARKUP_PREFIX_ERROR_POSITION) {
        x_prefix_error(&src, _("Error on line %d char %d: "), context->line_number, context->char_number);
    }

    mark_error(context, src);
    x_propagate_error(dest, src);
}

#define IS_COMMON_NAME_END_CHAR(c)      ((c) == '=' || (c) == '/' || (c) == '>' || (c) == ' ')

static xboolean slow_name_validate(XMarkupParseContext *context, const xchar *name, XError **error)
{
    const xchar *p = name;

    if (!x_utf8_validate(name, -1, NULL)) {
        set_error(context, error, X_MARKUP_ERROR_BAD_UTF8, _("Invalid UTF-8 encoded text in name — not valid “%s”"), name);
        return FALSE;
    }

    if (!(x_ascii_isalpha(*p) || (!IS_COMMON_NAME_END_CHAR(*p) && (*p == '_' || *p == ':' || x_unichar_isalpha(x_utf8_get_char(p)))))) {
        set_error(context, error, X_MARKUP_ERROR_PARSE, _("“%s” is not a valid name"), name);
        return FALSE;
    }

    for (p = x_utf8_next_char(name); *p != '\0'; p = x_utf8_next_char(p)) {
        if (!(x_ascii_isalnum(*p) || (!IS_COMMON_NAME_END_CHAR(*p) && (*p == '.' || *p == '-' || *p == '_' || *p == ':' || x_unichar_isalpha(x_utf8_get_char(p)))))) {
            set_error(context, error, X_MARKUP_ERROR_PARSE, _("“%s” is not a valid name: “%c”"), name, *p);
            return FALSE;
        }
    }

    return TRUE;
}

static xboolean name_validate(XMarkupParseContext *context, const xchar *name, XError **error)
{
    char mask;
    const char *p;

    p = name;
    if (X_UNLIKELY(IS_COMMON_NAME_END_CHAR(*p) || !(x_ascii_isalpha(*p) || *p == '_' || *p == ':'))) {
        goto slow_validate;
    }

    for (mask = *p++; *p != '\0'; p++) {
        mask |= *p;
        if (X_UNLIKELY(!(x_ascii_isalnum(*p) || (!IS_COMMON_NAME_END_CHAR(*p) && (*p == '.' || *p == '-' || *p == '_' || *p == ':'))))) {
            goto slow_validate;
        }
    }

    if (mask & 0x80) {
        goto slow_validate;
    }

    return TRUE;

slow_validate:
    return slow_name_validate(context, name, error);
}

static xboolean text_validate(XMarkupParseContext *context, const xchar *p, xint len, XError **error)
{
    if (!x_utf8_validate_len(p, len, NULL)) {
        set_error(context, error, X_MARKUP_ERROR_BAD_UTF8, _("Invalid UTF-8 encoded text in name — not valid “%s”"), p);
        return FALSE;
    } else {
        return TRUE;
    }
}

static xchar *char_str(xunichar c, xchar *buf)
{
    memset(buf, 0, 8);
    x_unichar_to_utf8(c, buf);
    return buf;
}

static xchar *utf8_str(const xchar *utf8, xsize max_len, xchar *buf)
{
    xunichar c = x_utf8_get_char_validated(utf8, max_len);
    if (c == (xunichar) -1 || c == (xunichar) -2) {
        xuchar ch = (max_len > 0) ? (xuchar)*utf8 : 0;
        xchar *temp = x_strdup_printf("\\x%02x", (xuint) ch);
        memset(buf, 0, 8);
        memcpy(buf, temp, strlen(temp));
        x_free(temp);
    } else {
        char_str(c, buf);
    }

    return buf;
}

X_GNUC_PRINTF(5, 6)
static void set_unescape_error(XMarkupParseContext *context, XError **error, const xchar *remaining_text, XMarkupError code, const xchar *format, ...)
{
    xchar *s;
    va_list args;
    const xchar *p;
    XError *tmp_error;
    xint remaining_newlines;

    remaining_newlines = 0;
    p = remaining_text;
    while (*p != '\0') {
        if (*p == '\n') {
            ++remaining_newlines;
        }

        ++p;
    }

    va_start(args, format);
    s = x_strdup_vprintf(format, args);
    va_end(args);

    tmp_error = x_error_new(X_MARKUP_ERROR, code, _("Error on line %d: %s"), context->line_number - remaining_newlines, s);
    x_free(s);

    mark_error(context, tmp_error);
    x_propagate_error(error, tmp_error);
}

static xboolean unescape_gstring_inplace(XMarkupParseContext *context, XString *string, xboolean *is_ascii, XError **error)
{
    char mask, *to;
    const char *from;
    xboolean normalize_attribute;

    *is_ascii = FALSE;

    if (context->state == STATE_INSIDE_ATTRIBUTE_VALUE_SQ || context->state == STATE_INSIDE_ATTRIBUTE_VALUE_DQ) {
        normalize_attribute = TRUE;
    } else {
        normalize_attribute = FALSE;
    }

    mask = 0;
    for (from = to = string->str; *from != '\0'; from++, to++) {
        *to = *from;

        mask |= *to;
        if (normalize_attribute && (*to == '\t' || *to == '\n')) {
            *to = ' ';
        }

        if (*to == '\r') {
            *to = normalize_attribute ? ' ' : '\n';
            if (from[1] == '\n') {
                from++;
            }
        }

        if (*from == '&') {
            from++;
            if (*from == '#') {
                xulong l;
                xint base = 10;
                xchar *end = NULL;

                from++;
                if (*from == 'x') {
                    base = 16;
                    from++;
                }

                errno = 0;
                l = strtoul(from, &end, base);

                if (end == from || errno != 0) {
                    set_unescape_error(context, error,
                                        from, X_MARKUP_ERROR_PARSE,
                                        _("Failed to parse “%-.*s”, which "
                                            "should have been a digit "
                                            "inside a character reference "
                                            "(&#234; for example) — perhaps "
                                            "the digit is too large"),
                                        (int)(end - from), from);
                    return FALSE;
                } else if (*end != ';') {
                    set_unescape_error(context, error,
                                        from, X_MARKUP_ERROR_PARSE,
                                        _("Character reference did not end with a "
                                            "semicolon; "
                                            "most likely you used an ampersand "
                                            "character without intending to start "
                                            "an entity — escape ampersand as &amp;"));
                    return FALSE;
                } else {
                    if ((0 < l && l <= 0xD7FF) || (0xE000 <= l && l <= 0xFFFD) || (0x10000 <= l && l <= 0x10FFFF)) {
                        xchar buf[8];

                        char_str(l, buf);
                        strcpy (to, buf);
                        to += strlen(buf) - 1;
                        from = end;
                        if (l >= 0x80) {
                            mask |= 0x80;
                        }
                    } else {
                        set_unescape_error(context, error,
                                            from, X_MARKUP_ERROR_PARSE,
                                            _("Character reference “%-.*s” does not "
                                                "encode a permitted character"),
                                            (int)(end - from), from);
                        return FALSE;
                    }
                }
            } else if (strncmp(from, "lt;", 3) == 0) {
                *to = '<';
                from += 2;
            } else if (strncmp(from, "gt;", 3) == 0) {
                *to = '>';
                from += 2;
            } else if (strncmp(from, "amp;", 4) == 0) {
                *to = '&';
                from += 3;
            } else if (strncmp(from, "quot;", 5) == 0) {
                *to = '"';
                from += 4;
            } else if (strncmp(from, "apos;", 5) == 0) {
                *to = '\'';
                from += 4;
            } else {
                if (*from == ';') {
                    set_unescape_error(context, error, from, X_MARKUP_ERROR_PARSE, _("Empty entity “&;” seen; valid entities are: &amp; &quot; &lt; &gt; &apos;"));
                } else {
                    const char *end = strchr(from, ';');
                    if (end) {
                        set_unescape_error(context, error, from, X_MARKUP_ERROR_PARSE, _("Entity name “%-.*s” is not known"), (int)(end - from), from);
                    } else {
                        set_unescape_error(context, error, from, X_MARKUP_ERROR_PARSE, _("Entity did not end with a semicolon; most likely you used an ampersand character without intending to start an entity — escape ampersand as &amp;"));
                    }
                }

                return FALSE;
            }
        }
    }

    x_assert(to - string->str <= (xssize) string->len);
    if (to - string->str != (xssize) string->len) {
        x_string_truncate(string, to - string->str);
    }
    *is_ascii = !(mask & 0x80);

    return TRUE;
}

static inline xboolean advance_char(XMarkupParseContext *context)
{
    context->iter++;
    context->char_number++;

    if (X_UNLIKELY(context->iter == context->current_text_end)) {
        return FALSE;
    } else if (X_UNLIKELY(*context->iter == '\n')) {
        context->line_number++;
        context->char_number = 1;
    }

    return TRUE;
}

static inline xboolean xml_isspace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_spaces(XMarkupParseContext *context)
{
    do {
        if (!xml_isspace(*context->iter))
            return;
    } while (advance_char(context));
}

static void advance_to_name_end(XMarkupParseContext *context)
{
    do {
        if (IS_COMMON_NAME_END_CHAR(*(context->iter))) {
            return;
        }

        if (xml_isspace(*(context->iter))) {
            return;
        }
    } while (advance_char(context));
}

static void release_chunk(XMarkupParseContext *context, XString *str)
{
    XSList *node;

    if (!str) {
        return;
    }

    if (str->allocated_len > 256) {
        x_string_free(str, TRUE);
        return;
    }

    x_string_truncate(str, 0);
    node = get_list_node(context, str);
    context->spare_chunks = x_slist_concat(node, context->spare_chunks);
}

static void add_to_partial(XMarkupParseContext *context, const xchar *text_start, const xchar *text_end)
{
    if (context->partial_chunk == NULL) {
        if (context->spare_chunks != NULL) {
            XSList *node = context->spare_chunks;
            context->spare_chunks = x_slist_remove_link(context->spare_chunks, node);
            context->partial_chunk = (XString *)node->data;
            free_list_node(context, node);
        } else {
            context->partial_chunk = x_string_sized_new(MAX (28, text_end - text_start));
        }
    }

    if (text_start != text_end) {
        x_string_append_len(context->partial_chunk, text_start, text_end - text_start);
    }
}

static inline void truncate_partial(XMarkupParseContext *context)
{
    if (context->partial_chunk != NULL) {
        x_string_truncate(context->partial_chunk, 0);
    }
}

static inline const xchar *current_element(XMarkupParseContext *context)
{
    return (const xchar *)context->tag_stack->data;
}

static void pop_subparser_stack(XMarkupParseContext *context)
{
    XMarkupRecursionTracker *tracker;

    x_assert(context->subparser_stack);

    tracker = (XMarkupRecursionTracker *)context->subparser_stack->data;

    context->awaiting_pop = TRUE;
    context->held_user_data = context->user_data;

    context->user_data = tracker->prev_user_data;
    context->parser = tracker->prev_parser;
    context->subparser_element = tracker->prev_element;
    x_slice_free(XMarkupRecursionTracker, tracker);

    context->subparser_stack = x_slist_delete_link(context->subparser_stack, context->subparser_stack);
}

static void push_partial_as_tag(XMarkupParseContext *context)
{
    XString *str = context->partial_chunk;

    context->tag_stack = x_slist_concat(get_list_node(context, str->str), context->tag_stack);
    context->tag_stack_gstr = x_slist_concat(get_list_node(context, str), context->tag_stack_gstr);
    context->partial_chunk = NULL;
}

static void pop_tag(XMarkupParseContext *context)
{
    XSList *nodea, *nodeb;

    nodea = context->tag_stack;
    nodeb = context->tag_stack_gstr;
    release_chunk(context, (XString *)nodeb->data);
    context->tag_stack = x_slist_remove_link(context->tag_stack, nodea);
    context->tag_stack_gstr = x_slist_remove_link(context->tag_stack_gstr, nodeb);
    free_list_node (context, nodea);
    free_list_node (context, nodeb);
}

static void possibly_finish_subparser(XMarkupParseContext *context)
{
    if (current_element(context) == context->subparser_element) {
        pop_subparser_stack(context);
    }
}

static void ensure_no_outstanding_subparser(XMarkupParseContext *context)
{
    if (context->awaiting_pop) {
        x_critical("During the first end_element call after invoking a "
                    "subparser you must pop the subparser stack and handle "
                    "the freeing of the subparser user_data.  This can be "
                    "done by calling the end function of the subparser.  "
                    "Very probably, your program just leaked memory.");
    }

    context->held_user_data = NULL;
    context->awaiting_pop = FALSE;
}

static const xchar *current_attribute(XMarkupParseContext *context)
{
    x_assert(context->cur_attr >= 0);
    return context->attr_names[context->cur_attr]->str;
}

static xboolean add_attribute(XMarkupParseContext *context, XString *str)
{
    if (context->cur_attr >= 1000) {
        return FALSE;
    }

    if (context->cur_attr + 2 >= context->alloc_attrs) {
        context->alloc_attrs += 5;
        context->attr_names = (XString **)x_realloc(context->attr_names, sizeof(XString *)*context->alloc_attrs);
        context->attr_values = (XString **)x_realloc(context->attr_values, sizeof(XString *)*context->alloc_attrs);
    }

    context->cur_attr++;
    context->attr_names[context->cur_attr] = str;
    context->attr_values[context->cur_attr] = NULL;
    context->attr_names[context->cur_attr+1] = NULL;
    context->attr_values[context->cur_attr+1] = NULL;

    return TRUE;
}

static void clear_attributes(XMarkupParseContext *context)
{
    for (; context->cur_attr >= 0; context->cur_attr--) {
        int pos = context->cur_attr;
        release_chunk(context, context->attr_names[pos]);
        release_chunk(context, context->attr_values[pos]);
        context->attr_names[pos] = context->attr_values[pos] = NULL;
    }

    x_assert(context->cur_attr == -1);
    x_assert(context->attr_names == NULL || context->attr_names[0] == NULL);
    x_assert(context->attr_values == NULL || context->attr_values[0] == NULL);
}

static inline void emit_start_element(XMarkupParseContext *context, XError **error)
{
    int i, j = 0;
    XError *tmp_error;
    const xchar *start_name;
    const xchar **attr_names;
    const xchar **attr_values;

    if ((context->flags & X_MARKUP_IGNORE_QUALIFIED) && strchr(current_element(context), ':')) {
        static const XMarkupParser ignore_parser = { 0 };
        x_markup_parse_context_push(context, &ignore_parser, NULL);
        clear_attributes(context);
        return;
    }

    attr_names = x_newa(const xchar *, context->cur_attr + 2);
    attr_values = x_newa(const xchar *, context->cur_attr + 2);
    for (i = 0; i < context->cur_attr + 1; i++) {
        if ((context->flags & X_MARKUP_IGNORE_QUALIFIED) && strchr(context->attr_names[i]->str, ':')) {
            continue;
        }

        attr_names[j] = context->attr_names[i]->str;
        attr_values[j] = context->attr_values[i]->str;
        j++;
    }
    attr_names[j] = NULL;
    attr_values[j] = NULL;

    tmp_error = NULL;
    start_name = current_element(context);

    if (!name_validate(context, start_name, error)) {
        return;
    }

    if (context->parser->start_element) {
        (* context->parser->start_element)(context, start_name, (const xchar **)attr_names, (const xchar **)attr_values, context->user_data, &tmp_error);
    }
    clear_attributes(context);

    if (tmp_error != NULL) {
        propagate_error(context, error, tmp_error);
    }
}

static void emit_end_element(XMarkupParseContext *context, XError **error)
{
    XError *tmp_error = NULL;

    x_assert(context->tag_stack != NULL);

    possibly_finish_subparser(context);

    if ((context->flags & X_MARKUP_IGNORE_QUALIFIED) && strchr(current_element(context), ':')) {
        x_markup_parse_context_pop(context);
        pop_tag(context);
        return;
    }

    tmp_error = NULL;
    if (context->parser->end_element) {
        (*context->parser->end_element)(context, current_element(context), context->user_data, &tmp_error);
    }
    ensure_no_outstanding_subparser(context);

    if (tmp_error) {
        mark_error(context, tmp_error);
        x_propagate_error(error, tmp_error);
    }

    pop_tag(context);
}

xboolean x_markup_parse_context_parse(XMarkupParseContext *context, const xchar *text, xssize text_len, XError **error)
{
    x_return_val_if_fail(context != NULL, FALSE);
    x_return_val_if_fail(text != NULL, FALSE);
    x_return_val_if_fail(context->state != STATE_ERROR, FALSE);
    x_return_val_if_fail(!context->parsing, FALSE);

    if (text_len < 0) {
        text_len = strlen(text);
    }

    if (text_len == 0) {
        return TRUE;
    }

    context->parsing = TRUE;
    context->current_text = text;
    context->current_text_len = text_len;
    context->current_text_end = context->current_text + text_len;
    context->iter = context->current_text;
    context->start = context->iter;

    while (context->iter != context->current_text_end) {
        switch (context->state) {
            case STATE_START:
                x_assert(context->tag_stack == NULL);

                skip_spaces(context);

                if (context->iter != context->current_text_end) {
                    if (*context->iter == '<') {
                        advance_char(context);

                        context->state = STATE_AFTER_OPEN_ANGLE;
                        context->start = context->iter;
                        context->document_empty = FALSE;
                } else {
                    set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document must begin with an element (e.g. <book>)"));
                }
            }
            break;

            case STATE_AFTER_OPEN_ANGLE:
                if (*context->iter == '?' || *context->iter == '!') {
                    const xchar *openangle = "<";
                    add_to_partial(context, openangle, openangle + 1);
                    context->start = context->iter;
                    context->balance = 1;
                    context->state = STATE_INSIDE_PASSTHROUGH;
                } else if (*context->iter == '/') {
                    advance_char(context);
                    context->state = STATE_AFTER_CLOSE_TAG_SLASH;
                } else if (!IS_COMMON_NAME_END_CHAR(*(context->iter))) {
                    context->state = STATE_INSIDE_OPEN_TAG_NAME;
                    context->start = context->iter;
                } else {
                    xchar buf[8];
                    set_error(context, error, X_MARKUP_ERROR_PARSE, _("“%s” is not a valid character following a “<” character; it may not begin an element name"), utf8_str(context->iter, context->current_text_end - context->iter, buf));
                }
                break;

            case STATE_AFTER_CLOSE_ANGLE:
                if (context->tag_stack == NULL) {
                    context->start = NULL;
                    context->state = STATE_START;
                } else {
                    context->start = context->iter;
                    context->state = STATE_INSIDE_TEXT;
                }
                break;

            case STATE_AFTER_ELISION_SLASH:
                if (*context->iter == '>') {
                    advance_char(context);
                    context->state = STATE_AFTER_CLOSE_ANGLE;
                    emit_end_element(context, error);
                } else {
                    xchar buf[8];
                    set_error(context, error, X_MARKUP_ERROR_PARSE, _("Odd character “%s”, expected a “>” character to end the empty-element tag “%s”"), utf8_str(context->iter, context->current_text_end - context->iter, buf), current_element(context));
                }
                break;

            case STATE_INSIDE_OPEN_TAG_NAME:
                advance_to_name_end(context);

                if (context->iter == context->current_text_end) {
                    add_to_partial(context, context->start, context->iter);
                } else {
                    add_to_partial(context, context->start, context->iter);
                    push_partial_as_tag(context);

                    context->state = STATE_BETWEEN_ATTRIBUTES;
                    context->start = NULL;
                }
                break;

            case STATE_INSIDE_ATTRIBUTE_NAME:
                advance_to_name_end(context);
                add_to_partial(context, context->start, context->iter);

                if (context->iter != context->current_text_end) {
                    context->state = STATE_AFTER_ATTRIBUTE_NAME;
                }
                break;

            case STATE_AFTER_ATTRIBUTE_NAME:
                skip_spaces(context);

                if (context->iter != context->current_text_end) {
                    if (!name_validate(context, context->partial_chunk->str, error)) {
                        break;
                    }

                    if (!add_attribute(context, context->partial_chunk)) {
                        set_error(context, error, X_MARKUP_ERROR_PARSE, _("Too many attributes in element “%s”"), current_element(context));
                        break;
                    }

                    context->partial_chunk = NULL;
                    context->start = NULL;

                    if (*context->iter == '=') {
                        advance_char(context);
                        context->state = STATE_AFTER_ATTRIBUTE_EQUALS_SIGN;
                    } else {
                        xchar buf[8];
                        set_error(context, error, X_MARKUP_ERROR_PARSE, _("Odd character “%s”, expected a “=” after attribute name “%s” of element “%s”"), utf8_str(context->iter, context->current_text_end - context->iter, buf), current_attribute(context), current_element(context));
                    }
                }
                break;

            case STATE_BETWEEN_ATTRIBUTES:
                skip_spaces(context);

                if (context->iter != context->current_text_end) {
                    if (*context->iter == '/') {
                        advance_char(context);
                        context->state = STATE_AFTER_ELISION_SLASH;
                    } else if (*context->iter == '>') {
                        advance_char(context);
                        context->state = STATE_AFTER_CLOSE_ANGLE;
                    } else if (!IS_COMMON_NAME_END_CHAR(*(context->iter))) {
                        context->state = STATE_INSIDE_ATTRIBUTE_NAME;
                        context->start = context->iter;
                    } else {
                        xchar buf[8];
                        set_error(context,
                                error,
                                X_MARKUP_ERROR_PARSE,
                                _("Odd character “%s”, expected a “>” or “/” "
                                "character to end the start tag of "
                                "element “%s”, or optionally an attribute; "
                                "perhaps you used an invalid character in "
                                "an attribute name"),
                                utf8_str(context->iter, context->current_text_end - context->iter, buf), current_element(context));
                    }

                    if (context->state == STATE_AFTER_ELISION_SLASH || context->state == STATE_AFTER_CLOSE_ANGLE) {
                        emit_start_element(context, error);
                    }
                }
                break;

            case STATE_AFTER_ATTRIBUTE_EQUALS_SIGN:
                skip_spaces(context);

                if (context->iter != context->current_text_end) {
                    if (*context->iter == '"') {
                        advance_char(context);
                        context->state = STATE_INSIDE_ATTRIBUTE_VALUE_DQ;
                        context->start = context->iter;
                    } else if (*context->iter == '\'') {
                        advance_char(context);
                        context->state = STATE_INSIDE_ATTRIBUTE_VALUE_SQ;
                        context->start = context->iter;
                    } else {
                        xchar buf[8];
                        set_error(context,
                                error,
                                X_MARKUP_ERROR_PARSE,
                                _("Odd character “%s”, expected an open quote mark "
                                "after the equals sign when giving value for "
                                "attribute “%s” of element “%s”"),
                                utf8_str(context->iter, context->current_text_end - context->iter, buf), current_attribute(context), current_element(context));
                    }
                }
                break;

            case STATE_INSIDE_ATTRIBUTE_VALUE_SQ:
            case STATE_INSIDE_ATTRIBUTE_VALUE_DQ: {
                xchar delim;

                if (context->state == STATE_INSIDE_ATTRIBUTE_VALUE_SQ) {
                    delim = '\'';
                } else {
                    delim = '"';
                }

                do {
                    if (*context->iter == delim) {
                        break;
                    }
                } while (advance_char(context));
            }

            if (context->iter == context->current_text_end) {
                add_to_partial(context, context->start, context->iter);
            } else {
                xboolean is_ascii;
                add_to_partial(context, context->start, context->iter);

                x_assert(context->cur_attr >= 0);

                if (unescape_gstring_inplace(context, context->partial_chunk, &is_ascii, error) && (is_ascii || text_validate(context, context->partial_chunk->str, context->partial_chunk->len, error))) {
                    context->attr_values[context->cur_attr] = context->partial_chunk;
                    context->partial_chunk = NULL;
                    advance_char(context);
                    context->state = STATE_BETWEEN_ATTRIBUTES;
                    context->start = NULL;
                }

                truncate_partial(context);
            }
            break;

            case STATE_INSIDE_TEXT:
                do {
                    if (*context->iter == '<') {
                        break;
                    }
                } while (advance_char(context));

                add_to_partial(context, context->start, context->iter);

                if (context->iter != context->current_text_end) {
                    xboolean is_ascii;

                    if (unescape_gstring_inplace(context, context->partial_chunk, &is_ascii, error) && (is_ascii || text_validate(context, context->partial_chunk->str, context->partial_chunk->len, error))) {
                        XError *tmp_error = NULL;

                        if (context->parser->text) {
                            (*context->parser->text)(context, context->partial_chunk->str, context->partial_chunk->len, context->user_data, &tmp_error);
                        }

                        if (tmp_error == NULL) {
                            advance_char(context);
                            context->state = STATE_AFTER_OPEN_ANGLE;
                            context->start = context->iter;
                        } else {
                            propagate_error(context, error, tmp_error);
                        }
                    }

                    truncate_partial(context);
                }
                break;

            case STATE_AFTER_CLOSE_TAG_SLASH:
                if (!IS_COMMON_NAME_END_CHAR(*(context->iter))) {
                    context->state = STATE_INSIDE_CLOSE_TAG_NAME;
                    context->start = context->iter;
                } else {
                    xchar buf[8];
                    set_error(context,
                            error,
                            X_MARKUP_ERROR_PARSE,
                            _("“%s” is not a valid character following "
                            "the characters “</”; “%s” may not begin an "
                            "element name"),
                            utf8_str(context->iter, context->current_text_end - context->iter, buf),
                            utf8_str(context->iter, context->current_text_end - context->iter, buf));
                }
                break;

            case STATE_INSIDE_CLOSE_TAG_NAME:
                advance_to_name_end(context);
                add_to_partial(context, context->start, context->iter);

                if (context->iter != context->current_text_end) {
                    context->state = STATE_AFTER_CLOSE_TAG_NAME;
                }
                break;

            case STATE_AFTER_CLOSE_TAG_NAME:
                skip_spaces(context);

                if (context->iter != context->current_text_end) {
                    XString *close_name;

                    close_name = context->partial_chunk;
                    context->partial_chunk = NULL;

                    if (*context->iter != '>') {
                        xchar buf[8];
                        set_error(context,
                                error,
                                X_MARKUP_ERROR_PARSE,
                                _("“%s” is not a valid character following "
                                "the close element name “%s”; the allowed "
                                "character is “>”"),
                                utf8_str(context->iter, context->current_text_end - context->iter, buf), close_name->str);
                    } else if (context->tag_stack == NULL) {
                        set_error(context,
                                error,
                                X_MARKUP_ERROR_PARSE,
                                _("Element “%s” was closed, no element "
                                "is currently open"),
                                close_name->str);
                    } else if (strcmp(close_name->str, current_element(context)) != 0) {
                        set_error(context,
                                error,
                                X_MARKUP_ERROR_PARSE,
                                _("Element “%s” was closed, but the currently "
                                "open element is “%s”"),
                                close_name->str,
                                current_element(context));
                    } else {
                        advance_char(context);
                        context->state = STATE_AFTER_CLOSE_ANGLE;
                        context->start = NULL;

                        emit_end_element(context, error);
                    }

                    context->partial_chunk = close_name;
                    truncate_partial(context);
                }
                break;

            case STATE_INSIDE_PASSTHROUGH:
                do {
                    if (*context->iter == '<') {
                        context->balance++;
                    }

                    if (*context->iter == '>'){
                        xsize len;
                        xchar *str;

                        context->balance--;
                        add_to_partial(context, context->start, context->iter);
                        context->start = context->iter;

                        str = context->partial_chunk->str;
                    len = context->partial_chunk->len;

                        if (str[1] == '?' && str[len - 1] == '?') {
                            break;
                        }

                        if (strncmp(str, "<!--", 4) == 0 && strcmp(str + len - 2, "--") == 0) {
                            break;
                        }

                        if (strncmp(str, "<![CDATA[", 9) == 0 && strcmp(str + len - 2, "]]") == 0) {
                            break;
                        }

                        if (strncmp(str, "<!DOCTYPE", 9) == 0 && context->balance == 0) {
                            break;
                        }
                    }
                } while (advance_char(context));

                if (context->iter == context->current_text_end) {
                    add_to_partial(context, context->start, context->iter);
                } else {
                    XError *tmp_error = NULL;

                    advance_char(context);
                    add_to_partial(context, context->start, context->iter);

                    if (context->flags & X_MARKUP_TREAT_CDATA_AS_TEXT && strncmp(context->partial_chunk->str, "<![CDATA[", 9) == 0) {
                        if (context->parser->text && text_validate(context, context->partial_chunk->str + 9, context->partial_chunk->len - 12, error)) {
                            (*context->parser->text)(context, context->partial_chunk->str + 9, context->partial_chunk->len - 12, context->user_data, &tmp_error);
                        }
                    } else if (context->parser->passthrough && text_validate(context, context->partial_chunk->str, context->partial_chunk->len, error)) {
                        (*context->parser->passthrough)(context, context->partial_chunk->str, context->partial_chunk->len, context->user_data, &tmp_error);
                    }

                    truncate_partial(context);

                    if (tmp_error == NULL) {
                        context->state = STATE_AFTER_CLOSE_ANGLE;
                        context->start = context->iter;
                    } else {
                        propagate_error(context, error, tmp_error);
                    }
                }
                break;

            case STATE_ERROR:
                goto finished;
                break;

            default:
                x_assert_not_reached();
                break;
        }
     }

finished:
    context->parsing = FALSE;
    return context->state != STATE_ERROR;
}

xboolean x_markup_parse_context_end_parse(XMarkupParseContext *context, XError **error)
{
    x_return_val_if_fail(context != NULL, FALSE);
    x_return_val_if_fail(!context->parsing, FALSE);
    x_return_val_if_fail(context->state != STATE_ERROR, FALSE);

    if (context->partial_chunk != NULL) {
        x_string_free(context->partial_chunk, TRUE);
        context->partial_chunk = NULL;
    }

    if (context->document_empty) {
        set_error_literal(context, error, X_MARKUP_ERROR_EMPTY, _("Document was empty or contained only whitespace"));
        return FALSE;
    }

    context->parsing = TRUE;

    switch (context->state) {
        case STATE_START:
            break;

        case STATE_AFTER_OPEN_ANGLE:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly just after an open angle bracket “<”"));
            break;

        case STATE_AFTER_CLOSE_ANGLE:
            if (context->tag_stack != NULL) {
                set_error(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly with elements still open — “%s” was the last element opened"), current_element(context));
            }
            break;

        case STATE_AFTER_ELISION_SLASH:
            set_error(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly, expected to see a close angle bracket ending the tag <%s/>"), current_element(context));
            break;

        case STATE_INSIDE_OPEN_TAG_NAME:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly inside an element name"));
            break;

        case STATE_INSIDE_ATTRIBUTE_NAME:
        case STATE_AFTER_ATTRIBUTE_NAME:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly inside an attribute name"));
            break;

        case STATE_BETWEEN_ATTRIBUTES:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly inside an element-opening tag."));
            break;

        case STATE_AFTER_ATTRIBUTE_EQUALS_SIGN:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly after the equals sign following an attribute name; no attribute value"));
            break;

        case STATE_INSIDE_ATTRIBUTE_VALUE_SQ:
        case STATE_INSIDE_ATTRIBUTE_VALUE_DQ:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly while inside an attribute value"));
            break;

        case STATE_INSIDE_TEXT:
            x_assert(context->tag_stack != NULL);
            set_error(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly with elements still open — “%s” was the last element opened"), current_element(context));
            break;

        case STATE_AFTER_CLOSE_TAG_SLASH:
        case STATE_INSIDE_CLOSE_TAG_NAME:
        case STATE_AFTER_CLOSE_TAG_NAME:
            if (context->tag_stack != NULL) {
                set_error(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly inside the close tag for element “%s”"), current_element(context));
            } else {
                set_error(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly inside the close tag for an unopened element"));
            }
            break;

        case STATE_INSIDE_PASSTHROUGH:
            set_error_literal(context, error, X_MARKUP_ERROR_PARSE, _("Document ended unexpectedly inside a comment or processing instruction"));
            break;

        case STATE_ERROR:
        default:
            x_assert_not_reached();
            break;
    }

    context->parsing = FALSE;
    return context->state != STATE_ERROR;
}

const xchar *x_markup_parse_context_get_element(XMarkupParseContext *context)
{
    x_return_val_if_fail(context != NULL, NULL);

    if (context->tag_stack == NULL) {
        return NULL;
    } else {
        return current_element(context);
    }
}

const XSList *x_markup_parse_context_get_element_stack(XMarkupParseContext *context)
{
    x_return_val_if_fail(context != NULL, NULL);
    return context->tag_stack;
}

void x_markup_parse_context_get_position(XMarkupParseContext *context, xint *line_number, xint *char_number)
{
    x_return_if_fail(context != NULL);

    if (line_number) {
        *line_number = context->line_number;
    }

    if (char_number) {
        *char_number = context->char_number;
    }
}

xpointer x_markup_parse_context_get_user_data(XMarkupParseContext *context)
{
    return context->user_data;
}

void x_markup_parse_context_push(XMarkupParseContext *context, const XMarkupParser *parser, xpointer user_data)
{
    XMarkupRecursionTracker *tracker;

    tracker = x_slice_new(XMarkupRecursionTracker);
    tracker->prev_element = context->subparser_element;
    tracker->prev_parser = context->parser;
    tracker->prev_user_data = context->user_data;

    context->subparser_element = current_element(context);
    context->parser = parser;
    context->user_data = user_data;

    context->subparser_stack = x_slist_prepend(context->subparser_stack, tracker);
}

xpointer x_markup_parse_context_pop(XMarkupParseContext *context)
{
    xpointer user_data;

    if (!context->awaiting_pop) {
        possibly_finish_subparser(context);
    }
    x_assert(context->awaiting_pop);

    context->awaiting_pop = FALSE;

    user_data = context->held_user_data;
    context->held_user_data = NULL;

    return user_data;
}

#define APPEND_TEXT_AND_SEEK(_str, _start, _end)                \
    X_STMT_START {                                              \
        if (_end > _start) {                                    \
            x_string_append_len(_str, _start, _end - _start);   \
        }                                                       \
        _start = ++_end;                                        \
    } X_STMT_END

static void append_escaped_text(XString *str, const xchar *text, xssize length)
{
    const xchar *end;
    const xchar *p, *pending;

    p = pending = text;
    end = text + length;

    while (p < end && pending < end) {
        xuchar c = (xuchar)*pending;

        switch (c) {
            case '&':
                APPEND_TEXT_AND_SEEK(str, p, pending);
                x_string_append(str, "&amp;");
                break;

            case '<':
                APPEND_TEXT_AND_SEEK(str, p, pending);
                x_string_append(str, "&lt;");
                break;

            case '>':
                APPEND_TEXT_AND_SEEK(str, p, pending);
                x_string_append(str, "&gt;");
                break;

            case '\'':
                APPEND_TEXT_AND_SEEK(str, p, pending);
                x_string_append(str, "&apos;");
                break;

            case '"':
                APPEND_TEXT_AND_SEEK(str, p, pending);
                x_string_append(str, "&quot;");
                break;

            default:
                if ((0x1 <= c && c <= 0x8) || (0xb <= c && c  <= 0xc) || (0xe <= c && c <= 0x1f) || (c == 0x7f)) {
                    APPEND_TEXT_AND_SEEK(str, p, pending);
                    x_string_append_printf(str, "&#x%x;", c);
                } else if (c == 0xc2) {
                    xunichar u = x_utf8_get_char(pending);

                    if ((0x7f < u && u <= 0x84) || (0x86 <= u && u <= 0x9f)) {
                        APPEND_TEXT_AND_SEEK(str, p, pending);
                        x_string_append_printf(str, "&#x%x;", u);
                        p++;
                    } else {
                        pending++;
                    }
                } else {
                    pending++;
                }
                break;
        }
    }

    if (pending > p) {
        x_string_append_len(str, p, pending - p);
    }
}

#undef APPEND_TEXT_AND_SEEK

xchar *x_markup_escape_text(const xchar *text, xssize length)
{
    XString *str;

    x_return_val_if_fail(text != NULL, NULL);

    if (length < 0) {
        length = strlen(text);
    }

    str = x_string_sized_new(length);
    append_escaped_text(str, text, length);

    return x_string_free(str, FALSE);
}

static const char *find_conversion(const char *format, const char **after)
{
    const char *cp;
    const char *start = format;

    while (*start != '\0' && *start != '%') {
        start++;
    }

    if (*start == '\0') {
        *after = start;
        return NULL;
    }

    cp = start + 1;
    if (*cp == '\0') {
        *after = cp;
        return NULL;
    }

    if (*cp >= '0' && *cp <= '9') {
        const char *np;

        for (np = cp; *np >= '0' && *np <= '9'; np++);
        if (*np == '$') {
            cp = np + 1;
        }
    }

    for (; ;) {
        if (*cp == '\'' || *cp == '-' || *cp == '+' || *cp == ' ' || *cp == '#' || *cp == '0') {
            cp++;
        } else {
            break;
        }
    }

    if (*cp == '*') {
        cp++;

        if (*cp >= '0' && *cp <= '9') {
            const char *np;

            for (np = cp; *np >= '0' && *np <= '9'; np++);
            if (*np == '$') {
                cp = np + 1;
            }
        }
    } else {
        for (; *cp >= '0' && *cp <= '9'; cp++);
    }

    if (*cp == '.') {
        cp++;
        if (*cp == '*') {
            if (*cp >= '0' && *cp <= '9') {
                const char *np;

                for (np = cp; *np >= '0' && *np <= '9'; np++);
                if (*np == '$') {
                    cp = np + 1;
                }
            }
        } else {
            for (; *cp >= '0' && *cp <= '9'; cp++);
        }
    }

    while (*cp == 'h' ||
            *cp == 'L' ||
            *cp == 'l' ||
            *cp == 'j' ||
            *cp == 'z' ||
            *cp == 'Z' ||
            *cp == 't')
    {
        cp++;
    }
    cp++;

    *after = cp;
    return start;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

xchar *x_markup_vprintf_escaped(const xchar *format, va_list args)
{
    va_list args2;
    XString *format1;
    XString *format2;
    xchar *output1 = NULL;
    xchar *output2 = NULL;
    XString *result = NULL;
    const char *p, *op1, *op2;

    format1 = x_string_new(NULL);
    format2 = x_string_new(NULL);
    p = format;

    while (TRUE) {
        const char *after;
        const char *conv = find_conversion(p, &after);
        if (!conv) {
            break;
        }

        x_string_append_len(format1, conv, after - conv);
        x_string_append_c(format1, 'X');
        x_string_append_len(format2, conv, after - conv);
        x_string_append_c(format2, 'Y');

        p = after;
    }

    va_copy(args2, args);
    output1 = x_strdup_vprintf(format1->str, args);

    if (!output1) {
        va_end(args2);
        goto cleanup;
    }

    output2 = x_strdup_vprintf(format2->str, args2);
    va_end(args2);
    if (!output2) {
        goto cleanup;
    }
    result = x_string_new(NULL);

    op1 = output1;
    op2 = output2;
    p = format;

    while (TRUE) {
        char *escaped;
        const char *after;
        const char *output_start;
        const char *conv = find_conversion(p, &after);

        if (!conv) {
            x_string_append_len(result, p, after - p);
            break;
        }

        x_string_append_len(result, p, conv - p);
        output_start = op1;
        while (*op1 == *op2) {
            op1++;
            op2++;
        }

        escaped = x_markup_escape_text(output_start, op1 - output_start);
        x_string_append(result, escaped);
        x_free(escaped);

        p = after;
        op1++;
        op2++;
    }

    cleanup:
    x_string_free(format1, TRUE);
    x_string_free(format2, TRUE);
    x_free(output1);
    x_free(output2);

    if (result) {
        return x_string_free(result, FALSE);
    } else {
        return NULL;
    }
}

#pragma GCC diagnostic pop

xchar *x_markup_printf_escaped(const xchar *format, ...)
{
    char *result;
    va_list args;

    va_start(args, format);
    result = x_markup_vprintf_escaped(format, args);
    va_end(args);

    return result;
}

static xboolean x_markup_parse_boolean(const char *string, xboolean *value)
{
    xsize i;
    char const *const trues[] = { "true", "t", "yes", "y", "1" };
    char const *const falses[] = { "false", "f", "no", "n", "0" };

    for (i = 0; i < X_N_ELEMENTS(falses); i++) {
        if (x_ascii_strcasecmp(string, falses[i]) == 0) {
            if (value != NULL) {
                *value = FALSE;
            }

            return TRUE;
        }
    }

    for (i = 0; i < X_N_ELEMENTS(trues); i++) {
        if (x_ascii_strcasecmp(string, trues[i]) == 0) {
            if (value != NULL) {
                *value = TRUE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

xboolean x_markup_collect_attributes(const xchar *element_name, const xchar **attribute_names, const xchar **attribute_values, XError **error, XMarkupCollectType first_type, const xchar *first_attr, ...)
{
    int i;
    va_list ap;
    int written;
    const xchar *attr;
    xuint64 collected;
    XMarkupCollectType type;

    type = first_type;
    attr = first_attr;
    collected = 0;
    written = 0;

    va_start(ap, first_attr);
    while (type != X_MARKUP_COLLECT_INVALID) {
        xboolean mandatory;
        const xchar *value;

        mandatory = !(type & X_MARKUP_COLLECT_OPTIONAL);
        type = (XMarkupCollectType)(type & (X_MARKUP_COLLECT_OPTIONAL - 1));

        if (type == X_MARKUP_COLLECT_TRISTATE) {
            mandatory = FALSE;
        }

        for (i = 0; attribute_names[i]; i++) {
            if (i >= 40 || !(collected & (X_XUINT64_CONSTANT(1) << i))) {
                if (!strcmp(attribute_names[i], attr)) {
                    break;
                }
            }
        }

        if (i < 40) {
            collected |= (X_XUINT64_CONSTANT(1) << i);
        }

        value = attribute_values[i];
        if (value == NULL && mandatory) {
            x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_MISSING_ATTRIBUTE, "element '%s' requires attribute '%s'", element_name, attr);
            va_end(ap);
            goto failure;
        }

        switch (type) {
            case X_MARKUP_COLLECT_STRING: {
                const char **str_ptr;

                str_ptr = va_arg(ap, const char **);
                if (str_ptr != NULL) {
                    *str_ptr = value;
                }
            }
            break;

            case X_MARKUP_COLLECT_STRDUP: {
                char **str_ptr;

                str_ptr = va_arg(ap, char **);
                if (str_ptr != NULL) {
                    *str_ptr = x_strdup(value);
                }
            }
            break;

            case X_MARKUP_COLLECT_BOOLEAN:
            case X_MARKUP_COLLECT_TRISTATE:
                if (value == NULL) {
                    xboolean *bool_ptr;

                    bool_ptr = va_arg(ap, xboolean *);
                    if (bool_ptr != NULL) {
                        if (type == X_MARKUP_COLLECT_TRISTATE) {
                            *bool_ptr = -1;
                        } else {
                            *bool_ptr = FALSE;
                        }
                    }
                } else {
                    if (!x_markup_parse_boolean(value, va_arg(ap, xboolean *))) {
                        x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, "element '%s', attribute '%s', value '%s' cannot be parsed as a boolean value", element_name, attr, value);
                        va_end(ap);
                        goto failure;
                    }
                }
                break;

            default:
                x_assert_not_reached();
        }

        written++;
        type = (XMarkupCollectType)va_arg(ap, int/*XMarkupCollectType*/);
        if (type != X_MARKUP_COLLECT_INVALID) {
            attr = va_arg(ap, const char *);
        }
    }
    va_end(ap);

    for (i = 0; attribute_names[i]; i++) {
        if ((collected & (X_XUINT64_CONSTANT(1) << i)) == 0) {
            int j;

            for (j = 0; j < i; j++) {
                if (strcmp(attribute_names[i], attribute_names[j]) == 0) {
                    break;
                }
            }

            if (i == j) {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, "attribute '%s' invalid for element '%s'", attribute_names[i], element_name);
            } else {
                x_set_error(error, X_MARKUP_ERROR, X_MARKUP_ERROR_INVALID_CONTENT, "attribute '%s' given multiple times for element '%s'", attribute_names[i], element_name);
            }

            goto failure;
        }
    }

    return TRUE;

failure:
    type = first_type;

    va_start(ap, first_attr);
    while (type != X_MARKUP_COLLECT_INVALID) {
        xpointer ptr;

        ptr = va_arg(ap, xpointer);
        if (ptr != NULL) {
            switch (type & (X_MARKUP_COLLECT_OPTIONAL - 1)) {
                case X_MARKUP_COLLECT_STRDUP:
                    if (written) {
                        x_free(*(char **)ptr);
                    }
                    *(char **) ptr = NULL;
                    break;

                case X_MARKUP_COLLECT_STRING:
                    *(char **)ptr = NULL;
                    break;

                case X_MARKUP_COLLECT_BOOLEAN:
                    *(xboolean *)ptr = FALSE;
                    break;

                case X_MARKUP_COLLECT_TRISTATE:
                    *(xboolean *)ptr = -1;
                    break;
             }
        }

        type = (XMarkupCollectType)va_arg(ap, int/*XMarkupCollectType*/);
        if (type != X_MARKUP_COLLECT_INVALID) {
            attr = va_arg(ap, const char *);
            (void) attr;
         }
    }
    va_end(ap);

    return FALSE;
}
