#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xscanner.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>

#define to_lower(c)                                                 \
    ((xuchar) (                                                     \
    ((((xuchar)(c))>='A' && ((xuchar)(c))<='Z') * ('a'-'A')) |      \
    ((((xuchar)(c))>=192 && ((xuchar)(c))<=214) * (224-192)) |      \
    ((((xuchar)(c))>=216 && ((xuchar)(c))<=222) * (248-216)) |      \
    ((xuchar)(c))))

#define READ_BUFFER_SIZE            (4000)

typedef struct _XScannerKey XScannerKey;

struct _XScannerKey {
    xuint    scope_id;
    xchar    *symbol;
    xpointer value;
};

static const XScannerConfig x_scanner_config_template = {
    (
    " \t\r\n"
    ),
    (
    X_CSET_a_2_z
    "_"
    X_CSET_A_2_Z
    ),
    (
    X_CSET_a_2_z
    "_"
    X_CSET_A_2_Z
    X_CSET_DIGITS
    X_CSET_LATINS
    X_CSET_LATINC
    ),
    ( "#\n" ),

    FALSE,

    TRUE,
    TRUE,
    TRUE,
    TRUE,
    FALSE,
    FALSE,
    TRUE,
    FALSE,
    TRUE,
    TRUE,
    TRUE,
    FALSE,
    TRUE,
    TRUE,
    TRUE,
    FALSE,
    FALSE,
    TRUE,
    FALSE,
    FALSE,
    FALSE,
    0
};

static inline XScannerKey *x_scanner_lookup_internal(XScanner *scanner,xuint scope_id, const xchar *symbol);
static xboolean x_scanner_key_equal(xconstpointer v1, xconstpointer v2);
static xuint x_scanner_key_hash(xconstpointer v);
static void x_scanner_get_token_ll(XScanner *scanner, XTokenType *token_p, XTokenValue *value_p, xuint *line_p, xuint *position_p);
static void x_scanner_get_token_i(XScanner *scanner, XTokenType *token_p, XTokenValue *value_p, xuint *line_p, xuint *position_p);

static xuchar x_scanner_peek_next_char(XScanner *scanner);
static xuchar x_scanner_get_char(XScanner *scanner, xuint *line_p, xuint *position_p);
static void x_scanner_msg_handler(XScanner *scanner, xchar *message, xboolean is_error);

static inline xint x_scanner_char_2_num(xuchar c, xuchar base)
{
    if (c >= '0' && c <= '9') {
        c -= '0';
    } else if (c >= 'A' && c <= 'Z') {
        c -= 'A' - 10;
    } else if (c >= 'a' && c <= 'z') {
        c -= 'a' - 10;
    } else {
        return -1;
    }

    if (c < base) {
        return c;
    }

    return -1;
}

XScanner *x_scanner_new(const XScannerConfig *config_templ)
{
    XScanner *scanner;

    if (!config_templ) {
        config_templ = &x_scanner_config_template;
    }

    scanner = x_new0(XScanner, 1);

    scanner->user_data = NULL;
    scanner->max_parse_errors = 1;
    scanner->parse_errors = 0;
    scanner->input_name = NULL;
    x_datalist_init(&scanner->qdata);

    scanner->config = x_new0(XScannerConfig, 1);

    scanner->config->case_sensitive = config_templ->case_sensitive;
    scanner->config->cset_skip_characters = config_templ->cset_skip_characters;
    if (!scanner->config->cset_skip_characters) {
        scanner->config->cset_skip_characters = "";
    }
    scanner->config->cset_identifier_first = config_templ->cset_identifier_first;
    scanner->config->cset_identifier_nth = config_templ->cset_identifier_nth;
    scanner->config->cpair_comment_single = config_templ->cpair_comment_single;
    scanner->config->skip_comment_multi = config_templ->skip_comment_multi;
    scanner->config->skip_comment_single = config_templ->skip_comment_single;
    scanner->config->scan_comment_multi = config_templ->scan_comment_multi;
    scanner->config->scan_identifier = config_templ->scan_identifier;
    scanner->config->scan_identifier_1char = config_templ->scan_identifier_1char;
    scanner->config->scan_identifier_NULL = config_templ->scan_identifier_NULL;
    scanner->config->scan_symbols = config_templ->scan_symbols;
    scanner->config->scan_binary = config_templ->scan_binary;
    scanner->config->scan_octal = config_templ->scan_octal;
    scanner->config->scan_float = config_templ->scan_float;
    scanner->config->scan_hex = config_templ->scan_hex;
    scanner->config->scan_hex_dollar = config_templ->scan_hex_dollar;
    scanner->config->scan_string_sq = config_templ->scan_string_sq;
    scanner->config->scan_string_dq = config_templ->scan_string_dq;
    scanner->config->numbers_2_int = config_templ->numbers_2_int;
    scanner->config->int_2_float = config_templ->int_2_float;
    scanner->config->identifier_2_string = config_templ->identifier_2_string;
    scanner->config->char_2_token = config_templ->char_2_token;
    scanner->config->symbol_2_token = config_templ->symbol_2_token;
    scanner->config->scope_0_fallback = config_templ->scope_0_fallback;
    scanner->config->store_int64 = config_templ->store_int64;

    scanner->token = X_TOKEN_NONE;
    scanner->value.v_int64 = 0;
    scanner->line = 1;
    scanner->position = 0;

    scanner->next_token = X_TOKEN_NONE;
    scanner->next_value.v_int64 = 0;
    scanner->next_line = 1;
    scanner->next_position = 0;

    scanner->symbol_table = x_hash_table_new(x_scanner_key_hash, x_scanner_key_equal);
    scanner->input_fd = -1;
    scanner->text = NULL;
    scanner->text_end = NULL;
    scanner->buffer = NULL;
    scanner->scope_id = 0;
    scanner->msg_handler = x_scanner_msg_handler;

    return scanner;
}

static inline void x_scanner_free_value(XTokenType *token_p, XTokenValue *value_p)
{
    switch (*token_p) {
        case X_TOKEN_STRING:
        case X_TOKEN_IDENTIFIER:
        case X_TOKEN_IDENTIFIER_NULL:
        case X_TOKEN_COMMENT_SINGLE:
        case X_TOKEN_COMMENT_MULTI:
            x_free(value_p->v_string);
            break;

        default:
            break;
    }

    *token_p = X_TOKEN_NONE;
}

static void x_scanner_destroy_symbol_table_entry(xpointer _key, xpointer _value, xpointer _data)
{
    XScannerKey *key = (XScannerKey *)_key;

    x_free(key->symbol);
    x_free(key);
}

void x_scanner_destroy(XScanner *scanner)
{
    x_return_if_fail(scanner != NULL);

    x_datalist_clear(&scanner->qdata);
    x_hash_table_foreach(scanner->symbol_table, x_scanner_destroy_symbol_table_entry, NULL);
    x_hash_table_destroy(scanner->symbol_table);
    x_scanner_free_value(&scanner->token, &scanner->value);
    x_scanner_free_value(&scanner->next_token, &scanner->next_value);
    x_free(scanner->config);
    x_free(scanner->buffer);
    x_free(scanner);
}

static void x_scanner_msg_handler(XScanner *scanner, xchar *message, xboolean is_error)
{
    x_return_if_fail(scanner != NULL);

    fprintf(stderr, "%s:%d: ", scanner->input_name ? scanner->input_name : "<memory>", scanner->line);
    if (is_error) {
        fprintf(stderr, "error: ");
    }
    fprintf(stderr, "%s\n", message);
}

void x_scanner_error(XScanner *scanner, const xchar *format, ...)
{
    x_return_if_fail(scanner != NULL);
    x_return_if_fail(format != NULL);

    scanner->parse_errors++;

    if (scanner->msg_handler) {
        va_list args;
        xchar *string;

        va_start(args, format);
        string = x_strdup_vprintf(format, args);
        va_end(args);

        scanner->msg_handler(scanner, string, TRUE);
        x_free(string);
    }
}

void x_scanner_warn(XScanner *scanner, const xchar *format, ...)
{
    x_return_if_fail(scanner != NULL);
    x_return_if_fail(format != NULL);

    if (scanner->msg_handler) {
        va_list args;
        xchar *string;

        va_start(args, format);
        string = x_strdup_vprintf(format, args);
        va_end (args);

        scanner->msg_handler(scanner, string, FALSE);
        x_free(string);
    }
}

static xboolean x_scanner_key_equal(xconstpointer v1, xconstpointer v2)
{
    const XScannerKey *key1 = (const XScannerKey *)v1;
    const XScannerKey *key2 = (const XScannerKey *)v2;

    return (key1->scope_id == key2->scope_id) && (strcmp(key1->symbol, key2->symbol) == 0);
}

static xuint x_scanner_key_hash(xconstpointer v)
{
    xuint h;
    xchar *c;
    const XScannerKey *key = (const XScannerKey *)v;

    h = key->scope_id;
    for (c = key->symbol; *c; c++) {
        h = (h << 5) - h + *c;
    }

    return h;
}

static inline XScannerKey *x_scanner_lookup_internal(XScanner *scanner, xuint scope_id, const xchar *symbol)
{
    XScannerKey key;
    XScannerKey *key_p;

    key.scope_id = scope_id;

    if (!scanner->config->case_sensitive) {
        xchar *d;
        const xchar *c;

        key.symbol = x_new(xchar, strlen(symbol) + 1);
        for (d = key.symbol, c = symbol; *c; c++, d++) {
            *d = to_lower(*c);
        }

        *d = 0;
        key_p = (XScannerKey *)x_hash_table_lookup(scanner->symbol_table, &key);
        x_free(key.symbol);
    } else {
        key.symbol = (xchar*) symbol;
        key_p = (XScannerKey *)x_hash_table_lookup(scanner->symbol_table, &key);
    }

    return key_p;
}

void x_scanner_scope_add_symbol(XScanner *scanner, xuint scope_id, const xchar *symbol, xpointer value)
{
    XScannerKey	*key;

    x_return_if_fail(scanner != NULL);
    x_return_if_fail(symbol != NULL);

    key = x_scanner_lookup_internal(scanner, scope_id, symbol);
    if (!key) {
        key = x_new(XScannerKey, 1);
        key->scope_id = scope_id;
        key->symbol = x_strdup(symbol);
        key->value = value;

        if (!scanner->config->case_sensitive) {
            xchar *c;

            c = key->symbol;
            while (*c != 0) {
                *c = to_lower(*c);
                c++;
            }
        }

        x_hash_table_add(scanner->symbol_table, key);
    } else {
        key->value = value;
    }
}

void x_scanner_scope_remove_symbol(XScanner *scanner, xuint scope_id, const xchar *symbol)
{
    XScannerKey *key;

    x_return_if_fail(scanner != NULL);
    x_return_if_fail(symbol != NULL);

    key = x_scanner_lookup_internal(scanner, scope_id, symbol);
    if (key) {
        x_hash_table_remove(scanner->symbol_table, key);
        x_free(key->symbol);
        x_free(key);
    }
}

xpointer x_scanner_lookup_symbol(XScanner *scanner, const xchar *symbol)
{
    xuint scope_id;
    XScannerKey *key;

    x_return_val_if_fail(scanner != NULL, NULL);

    if (!symbol) {
        return NULL;
    }

    scope_id = scanner->scope_id;
    key = x_scanner_lookup_internal(scanner, scope_id, symbol);
    if (!key && scope_id && scanner->config->scope_0_fallback) {
        key = x_scanner_lookup_internal(scanner, 0, symbol);
    }

    if (key) {
        return key->value;
    } else {
        return NULL;
    }
}

xpointer x_scanner_scope_lookup_symbol(XScanner *scanner, xuint scope_id, const xchar *symbol)
{
    XScannerKey *key;

    x_return_val_if_fail(scanner != NULL, NULL);

    if (!symbol) {
        return NULL;
    }

    key = x_scanner_lookup_internal(scanner, scope_id, symbol);
    if (key) {
        return key->value;
    } else {
        return NULL;
    }
}

xuint x_scanner_set_scope(XScanner *scanner, xuint scope_id)
{
    xuint old_scope_id;

    x_return_val_if_fail(scanner != NULL, 0);

    old_scope_id = scanner->scope_id;
    scanner->scope_id = scope_id;

    return old_scope_id;
}

static void x_scanner_foreach_internal(xpointer _key, xpointer _value, xpointer _user_data)
{
    xpointer *d;
    XHFunc func;
    xuint *scope_id;
    XScannerKey *key;
    xpointer user_data;

    d = (xpointer *)_user_data;
    func = (XHFunc)d[0];
    user_data = d[1];
    scope_id = (xuint *)d[2];
    key = (XScannerKey *)_value;

    if (key->scope_id == *scope_id) {
        func(key->symbol, key->value, user_data);
    }
}

void x_scanner_scope_foreach_symbol(XScanner *scanner, xuint scope_id, XHFunc func, xpointer user_data)
{
    xpointer d[3];

    x_return_if_fail(scanner != NULL);

    d[0] = (xpointer) func;
    d[1] = user_data;
    d[2] = &scope_id;

    x_hash_table_foreach(scanner->symbol_table, x_scanner_foreach_internal, d);
}

XTokenType x_scanner_peek_next_token(XScanner *scanner)
{
    x_return_val_if_fail(scanner != NULL, X_TOKEN_EOF);

    if (scanner->next_token == X_TOKEN_NONE) {
        scanner->next_line = scanner->line;
        scanner->next_position = scanner->position;
        x_scanner_get_token_i(scanner, &scanner->next_token, &scanner->next_value, &scanner->next_line, &scanner->next_position);
    }

    return scanner->next_token;
}

XTokenType x_scanner_get_next_token(XScanner *scanner)
{
    x_return_val_if_fail(scanner != NULL, X_TOKEN_EOF);

    if (scanner->next_token != X_TOKEN_NONE) {
        x_scanner_free_value(&scanner->token, &scanner->value);

        scanner->token = scanner->next_token;
        scanner->value = scanner->next_value;
        scanner->line = scanner->next_line;
        scanner->position = scanner->next_position;
        scanner->next_token = X_TOKEN_NONE;
    } else {
        x_scanner_get_token_i(scanner, &scanner->token, &scanner->value, &scanner->line, &scanner->position);
    }

    return scanner->token;
}

XTokenType x_scanner_cur_token(XScanner *scanner)
{
    x_return_val_if_fail(scanner != NULL, X_TOKEN_EOF);
    return scanner->token;
}

XTokenValue x_scanner_cur_value(XScanner *scanner)
{
    XTokenValue v;

    v.v_int64 = 0;
    x_return_val_if_fail(scanner != NULL, v);
    v = scanner->value;

    return v;
}

xuint x_scanner_cur_line(XScanner *scanner)
{
    x_return_val_if_fail(scanner != NULL, 0);
    return scanner->line;
}

xuint x_scanner_cur_position(XScanner *scanner)
{
    x_return_val_if_fail(scanner != NULL, 0);
    return scanner->position;
}

xboolean x_scanner_eof(XScanner *scanner)
{
    x_return_val_if_fail(scanner != NULL, TRUE);
    return scanner->token == X_TOKEN_EOF || scanner->token == X_TOKEN_ERROR;
}

void x_scanner_input_file(XScanner *scanner, xint input_fd)
{
    x_return_if_fail(scanner != NULL);
    x_return_if_fail(input_fd >= 0);

    if (scanner->input_fd >= 0) {
        x_scanner_sync_file_offset(scanner);
    }

    scanner->token = X_TOKEN_NONE;
    scanner->value.v_int64 = 0;
    scanner->line = 1;
    scanner->position = 0;
    scanner->next_token = X_TOKEN_NONE;

    scanner->input_fd = input_fd;
    scanner->text = NULL;
    scanner->text_end = NULL;

    if (!scanner->buffer) {
        scanner->buffer = x_new(xchar, READ_BUFFER_SIZE + 1);
    }
}

void x_scanner_input_text(XScanner *scanner, const xchar *text, xuint text_len)
{
    x_return_if_fail(scanner != NULL);

    if (text_len) {
        x_return_if_fail(text != NULL);
    } else {
        text = NULL;
    }

    if (scanner->input_fd >= 0) {
        x_scanner_sync_file_offset(scanner);
    }

    scanner->token = X_TOKEN_NONE;
    scanner->value.v_int64 = 0;
    scanner->line = 1;
    scanner->position = 0;
    scanner->next_token = X_TOKEN_NONE;

    scanner->input_fd = -1;
    scanner->text = text;
    scanner->text_end = text + text_len;

    if (scanner->buffer) {
        x_free(scanner->buffer);
        scanner->buffer = NULL;
    }
}

static xuchar x_scanner_peek_next_char(XScanner *scanner)
{
    if (scanner->text < scanner->text_end) {
        return *scanner->text;
    } else if (scanner->input_fd >= 0) {
        xint count;
        xchar *buffer;

        buffer = scanner->buffer;
        do {
            count = read(scanner->input_fd, buffer, READ_BUFFER_SIZE);
        } while (count == -1 && (errno == EINTR || errno == EAGAIN));

        if (count < 1) {
            scanner->input_fd = -1;
            return 0;
        } else {
            scanner->text = buffer;
            scanner->text_end = buffer + count;

            return *buffer;
        }
    } else {
        return 0;
    }
}

void x_scanner_sync_file_offset(XScanner *scanner)
{
    x_return_if_fail(scanner != NULL);

    if (scanner->input_fd >= 0 && scanner->text_end > scanner->text) {
        xint buffered;

        buffered = scanner->text_end - scanner->text;
        if (lseek(scanner->input_fd, - buffered, SEEK_CUR) >= 0) {
            scanner->text = NULL;
            scanner->text_end = NULL;
        } else {
            errno = 0;
        }
    }
}

static xuchar x_scanner_get_char(XScanner *scanner, xuint *line_p, xuint *position_p)
{
    xuchar fchar;

    if (scanner->text < scanner->text_end) {
        fchar = *(scanner->text++);
    } else if (scanner->input_fd >= 0) {
        xint count;
        xchar *buffer;

        buffer = scanner->buffer;
        do {
            count = read(scanner->input_fd, buffer, READ_BUFFER_SIZE);
        } while (count == -1 && (errno == EINTR || errno == EAGAIN));

        if (count < 1) {
            scanner->input_fd = -1;
            fchar = 0;
        } else {
            scanner->text = buffer + 1;
            scanner->text_end = buffer + count;
            fchar = *buffer;
            if (!fchar) {
                x_scanner_sync_file_offset(scanner);
                scanner->text_end = scanner->text;
                scanner->input_fd = -1;
            }
        }
    } else {
        fchar = 0;
    }

    if (fchar == '\n') {
        (*position_p) = 0;
        (*line_p)++;
    } else if (fchar) {
        (*position_p)++;
    }

    return fchar;
}

void x_scanner_unexp_token(XScanner *scanner, XTokenType  expected_token, const xchar *identifier_spec, const xchar *symbol_spec, const xchar *symbol_name, const xchar *message, xint is_error)
{
    xchar *token_string;
    xboolean print_unexp;
    xchar *message_prefix;
    xuint token_string_len;
    xchar *expected_string;
    xuint expected_string_len;

    void (*msg_handler)(XScanner *, const xchar *, ...);

    x_return_if_fail(scanner != NULL);

    if (is_error) {
        msg_handler = x_scanner_error;
    } else {
        msg_handler = x_scanner_warn;
    }

    if (!identifier_spec) {
        identifier_spec = "identifier";
    }

    if (!symbol_spec) {
        symbol_spec = "symbol";
    }

    token_string_len = 56;
    token_string = x_new(xchar, token_string_len + 1);
    expected_string_len = 64;
    expected_string = x_new(xchar, expected_string_len + 1);
    print_unexp = TRUE;

    switch (scanner->token) {
        case X_TOKEN_EOF:
            snprintf(token_string, token_string_len, "end of file");
            break;

        default:
            if (scanner->token >= 1 && scanner->token <= 255) {
                if ((scanner->token >= ' ' && scanner->token <= '~') || strchr(scanner->config->cset_identifier_first, scanner->token) || strchr(scanner->config->cset_identifier_nth, scanner->token)) {
                    snprintf(token_string, token_string_len, "character '%c'", scanner->token);
                } else {
                    snprintf(token_string, token_string_len, "character '\\%o'", scanner->token);
                }

                break;
            } else if (!scanner->config->symbol_2_token) {
                snprintf(token_string, token_string_len, "(unknown) token <%d>", scanner->token);
                break;
            }
            X_GNUC_FALLTHROUGH;

        case X_TOKEN_SYMBOL:
            if (expected_token == X_TOKEN_SYMBOL || (scanner->config->symbol_2_token && expected_token > X_TOKEN_LAST)) {
                print_unexp = FALSE;
            }

            if (symbol_name) {
                snprintf(token_string, token_string_len, "%s%s '%s'", print_unexp ? "" : "invalid ", symbol_spec, symbol_name);
            } else {
                snprintf(token_string, token_string_len, "%s%s", print_unexp ? "" : "invalid ", symbol_spec);
            }
            break;

        case X_TOKEN_ERROR:
            print_unexp = FALSE;
            expected_token = X_TOKEN_NONE;
            switch (scanner->value.v_error) {
                case X_ERR_UNEXP_EOF:
                    snprintf(token_string, token_string_len, "scanner: unexpected end of file");
                    break;

                case X_ERR_UNEXP_EOF_IN_STRING:
                    snprintf(token_string, token_string_len, "scanner: unterminated string constant");
                    break;

                case X_ERR_UNEXP_EOF_IN_COMMENT:
                    snprintf(token_string, token_string_len, "scanner: unterminated comment");
                    break;

                case X_ERR_NON_DIGIT_IN_CONST:
                    snprintf(token_string, token_string_len, "scanner: non digit in constant");
                    break;

                case X_ERR_FLOAT_RADIX:
                    snprintf(token_string, token_string_len, "scanner: invalid radix for floating constant");
                    break;

                case X_ERR_FLOAT_MALFORMED:
                    snprintf(token_string, token_string_len, "scanner: malformed floating constant");
                    break;

                case X_ERR_DIGIT_RADIX:
                    snprintf(token_string, token_string_len, "scanner: digit is beyond radix");
                    break;

                case X_ERR_UNKNOWN:
                default:
                    snprintf(token_string, token_string_len, "scanner: unknown error");
                    break;
            }
            break;

        case X_TOKEN_CHAR:
            snprintf(token_string, token_string_len, "character '%c'", scanner->value.v_char);
            break;

        case X_TOKEN_IDENTIFIER:
        case X_TOKEN_IDENTIFIER_NULL:
            if (expected_token == X_TOKEN_IDENTIFIER || expected_token == X_TOKEN_IDENTIFIER_NULL) {
                print_unexp = FALSE;
            }
            snprintf(token_string, token_string_len, "%s%s '%s'", print_unexp ? "" : "invalid ", identifier_spec, scanner->token == X_TOKEN_IDENTIFIER ? scanner->value.v_string : "null");
            break;

        case X_TOKEN_BINARY:
        case X_TOKEN_OCTAL:
        case X_TOKEN_INT:
        case X_TOKEN_HEX:
            if (scanner->config->store_int64) {
                snprintf(token_string, token_string_len, "number '%" X_XUINT64_FORMAT "'", scanner->value.v_int64);
            } else {
                snprintf(token_string, token_string_len, "number '%lu'", scanner->value.v_int);
            }
            break;

        case X_TOKEN_FLOAT:
            snprintf(token_string, token_string_len, "number '%.3f'", scanner->value.v_float);
            break;

        case X_TOKEN_STRING:
            if (expected_token == X_TOKEN_STRING) {
                print_unexp = FALSE;
            }
            snprintf(token_string, token_string_len, "%s%sstring constant \"%s\"", print_unexp ? "" : "invalid ", scanner->value.v_string[0] == 0 ? "empty " : "", scanner->value.v_string);
            token_string[token_string_len - 2] = '"';
            token_string[token_string_len - 1] = 0;
            break;

        case X_TOKEN_COMMENT_SINGLE:
        case X_TOKEN_COMMENT_MULTI:
            snprintf(token_string, token_string_len, "comment");
            break;

        case X_TOKEN_NONE:
            x_assert_not_reached();
            break;
    }

    switch (expected_token) {
        xchar *tstring;
        xboolean need_valid;

        case X_TOKEN_EOF:
            snprintf(expected_string, expected_string_len, "end of file");
            break;

        default:
            if (expected_token >= 1 && expected_token <= 255) {
                if ((expected_token >= ' ' && expected_token <= '~') || strchr(scanner->config->cset_identifier_first, expected_token) || strchr(scanner->config->cset_identifier_nth, expected_token)) {
                    snprintf(expected_string, expected_string_len, "character '%c'", expected_token);
                } else {
                    snprintf(expected_string, expected_string_len, "character '\\%o'", expected_token);
                }

                break;
            } else if (!scanner->config->symbol_2_token) {
                snprintf(expected_string, expected_string_len, "(unknown) token <%d>", expected_token);
                break;
            }
            X_GNUC_FALLTHROUGH;

        case X_TOKEN_SYMBOL:
            need_valid = (scanner->token == X_TOKEN_SYMBOL || (scanner->config->symbol_2_token && scanner->token > X_TOKEN_LAST));
            snprintf(expected_string, expected_string_len, "%s%s", need_valid ? "valid " : "", symbol_spec);
            break;

        case X_TOKEN_CHAR:
            snprintf(expected_string, expected_string_len, "%scharacter", scanner->token == X_TOKEN_CHAR ? "valid " : "");
            break;

        case X_TOKEN_BINARY:
            tstring = "binary";
            snprintf(expected_string, expected_string_len, "%snumber (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_OCTAL:
            tstring = "octal";
            snprintf(expected_string, expected_string_len, "%snumber (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_INT:
            tstring = "integer";
            snprintf(expected_string, expected_string_len, "%snumber (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_HEX:
            tstring = "hexadecimal";
            snprintf(expected_string, expected_string_len, "%snumber (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_FLOAT:
            tstring = "float";
            snprintf(expected_string, expected_string_len, "%snumber (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_STRING:
            snprintf(expected_string, expected_string_len, "%sstring constant", scanner->token == X_TOKEN_STRING ? "valid " : "");
            break;

        case X_TOKEN_IDENTIFIER:
        case X_TOKEN_IDENTIFIER_NULL:
            need_valid = (scanner->token == X_TOKEN_IDENTIFIER_NULL || scanner->token == X_TOKEN_IDENTIFIER);
            snprintf(expected_string, expected_string_len, "%s%s", need_valid ? "valid " : "", identifier_spec);
            break;

        case X_TOKEN_COMMENT_SINGLE:
            tstring = "single-line";
            snprintf(expected_string, expected_string_len, "%scomment (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_COMMENT_MULTI:
            tstring = "multi-line";
            snprintf(expected_string, expected_string_len, "%scomment (%s)", scanner->token == expected_token ? "valid " : "", tstring);
            break;

        case X_TOKEN_NONE:
        case X_TOKEN_ERROR:
            break;
    }

    if (message && message[0] != 0) {
        message_prefix = " - ";
    } else {
        message_prefix = "";
        message = "";
    }

    if (expected_token == X_TOKEN_ERROR) {
        msg_handler(scanner, "failure around %s%s%s", token_string, message_prefix, message);
    } else if (expected_token == X_TOKEN_NONE) {
        if (print_unexp) {
            msg_handler(scanner, "unexpected %s%s%s", token_string, message_prefix, message);
        } else {
            msg_handler(scanner, "%s%s%s", token_string, message_prefix, message);
        }
    } else {
        if (print_unexp) {
            msg_handler(scanner, "unexpected %s, expected %s%s%s", token_string, expected_string, message_prefix, message);
        } else {
            msg_handler(scanner, "%s, expected %s%s%s", token_string, expected_string, message_prefix, message);
        }
    }

    x_free(token_string);
    x_free(expected_string);
}

static void x_scanner_get_token_i(XScanner *scanner, XTokenType *token_p, XTokenValue *value_p, xuint *line_p, xuint *position_p)
{
    do {
        x_scanner_free_value(token_p, value_p);
        x_scanner_get_token_ll(scanner, token_p, value_p, line_p, position_p);
    } while (((*token_p > 0 && *token_p < 256) && strchr(scanner->config->cset_skip_characters, *token_p)) || (*token_p == X_TOKEN_CHAR && strchr(scanner->config->cset_skip_characters, value_p->v_char)) || (*token_p == X_TOKEN_COMMENT_MULTI && scanner->config->skip_comment_multi) || (*token_p == X_TOKEN_COMMENT_SINGLE && scanner->config->skip_comment_single));

    switch (*token_p) {
        case X_TOKEN_IDENTIFIER:
            if (scanner->config->identifier_2_string) {
                *token_p = X_TOKEN_STRING;
            }
            break;

        case X_TOKEN_SYMBOL:
            if (scanner->config->symbol_2_token) {
                *token_p = (XTokenType)((size_t)value_p->v_symbol);
            }
            break;

        case X_TOKEN_BINARY:
        case X_TOKEN_OCTAL:
        case X_TOKEN_HEX:
            if (scanner->config->numbers_2_int) {
                *token_p = X_TOKEN_INT;
            }
            break;

        default:
            break;
    }

    if (*token_p == X_TOKEN_INT && scanner->config->int_2_float) {
        *token_p = X_TOKEN_FLOAT;

        if (scanner->config->store_int64) {
            xint64 temp = value_p->v_int64;
            value_p->v_float = temp;
        } else {
            xint temp = value_p->v_int;
            value_p->v_float = temp;
         }
    }

    errno = 0;
}

static void x_scanner_get_token_ll(XScanner *scanner, XTokenType *token_p, XTokenValue *value_p, xuint *line_p, xuint *position_p)
{
    xuchar ch;
    XString *gstring;
    XTokenType token;
    XTokenValue value;
    xboolean in_string_sq;
    xboolean in_string_dq;
    XScannerConfig *config;
    xboolean in_comment_multi;
    xboolean in_comment_single;

    config = scanner->config;
    (*value_p).v_int64 = 0;

    if ((scanner->text >= scanner->text_end && scanner->input_fd < 0) || scanner->token == X_TOKEN_EOF) {
        *token_p = X_TOKEN_EOF;
        return;
    }

    in_comment_multi = FALSE;
    in_comment_single = FALSE;
    in_string_sq = FALSE;
    in_string_dq = FALSE;
    gstring = NULL;

    do {
        xboolean dotted_float = FALSE;

        ch = x_scanner_get_char(scanner, line_p, position_p);
        value.v_int64 = 0;
        token = X_TOKEN_NONE;

        if (config->scan_identifier && ch && strchr(config->cset_identifier_first, ch)) {
            goto identifier_precedence;
        }

        switch (ch) {
            case 0:
                token = X_TOKEN_EOF;
                (*position_p)++;
                break;

            case '/':
                if (!config->scan_comment_multi || x_scanner_peek_next_char (scanner) != '*') {
                    goto default_case;
                }
    
                x_scanner_get_char(scanner, line_p, position_p);
                token = X_TOKEN_COMMENT_MULTI;
                in_comment_multi = TRUE;
                gstring = x_string_new(NULL);
                while ((ch = x_scanner_get_char(scanner, line_p, position_p)) != 0) {
                    if (ch == '*' && x_scanner_peek_next_char(scanner) == '/') {
                        x_scanner_get_char(scanner, line_p, position_p);
                        in_comment_multi = FALSE;
                        break;
                    } else {
                        gstring = x_string_append_c(gstring, ch);
                    }
                }
                ch = 0;
                break;

            case '\'':
                if (!config->scan_string_sq) {
                    goto default_case;
                }

                token = X_TOKEN_STRING;
                in_string_sq = TRUE;
                gstring = x_string_new(NULL);
                while ((ch = x_scanner_get_char(scanner, line_p, position_p)) != 0) {
                    if (ch == '\'') {
                        in_string_sq = FALSE;
                        break;
                    } else {
                        gstring = x_string_append_c(gstring, ch);
                    }
                }
                ch = 0;
                break;

            case '"':
                if (!config->scan_string_dq) {
                    goto default_case;
                }

                token = X_TOKEN_STRING;
                in_string_dq = TRUE;
                gstring = x_string_new(NULL);

                while ((ch = x_scanner_get_char(scanner, line_p, position_p)) != 0) {
                    if (ch == '"') {
                        in_string_dq = FALSE;
                        break;
                    } else {
                        if (ch == '\\') {
                            ch = x_scanner_get_char(scanner, line_p, position_p);
                            switch (ch) {
                                xuint i;
                                xuint fchar;

                                case 0:
                                    break;

                                case '\\':
                                    gstring = x_string_append_c(gstring, '\\');
                                    break;

                                case 'n':
                                    gstring = x_string_append_c(gstring, '\n');
                                    break;

                                case 't':
                                    gstring = x_string_append_c(gstring, '\t');
                                    break;

                                case 'r':
                                    gstring = x_string_append_c(gstring, '\r');
                                    break;

                                case 'b':
                                    gstring = x_string_append_c(gstring, '\b');
                                    break;

                                case 'f':
                                    gstring = x_string_append_c(gstring, '\f');
                                    break;

                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                    i = ch - '0';
                                    fchar = x_scanner_peek_next_char(scanner);
                                    if (fchar >= '0' && fchar <= '7') {
                                        ch = x_scanner_get_char(scanner, line_p, position_p);
                                        i = i * 8 + ch - '0';
                                        fchar = x_scanner_peek_next_char(scanner);
                                        if (fchar >= '0' && fchar <= '7') {
                                            ch = x_scanner_get_char(scanner, line_p, position_p);
                                            i = i * 8 + ch - '0';
                                        }
                                    }
                                    gstring = x_string_append_c(gstring, i);
                                    break;

                                default:
                                    gstring = x_string_append_c(gstring, ch);
                                    break;
                            }
                        } else {
                            gstring = x_string_append_c(gstring, ch);
                        }
                    }
                }
                ch = 0;
                break;

            case '.':
                if (!config->scan_float) {
                    goto default_case;
                }
                token = X_TOKEN_FLOAT;
                dotted_float = TRUE;
                ch = x_scanner_get_char(scanner, line_p, position_p);
                goto number_parsing;

            case '$':
                if (!config->scan_hex_dollar) {
                    goto default_case;
                }
                token = X_TOKEN_HEX;
                ch = x_scanner_get_char(scanner, line_p, position_p);
                goto number_parsing;

            case '0':
                if (config->scan_octal) {
                    token = X_TOKEN_OCTAL;
                } else {
                    token = X_TOKEN_INT;
                }

                ch = x_scanner_peek_next_char(scanner);
                if (config->scan_hex && (ch == 'x' || ch == 'X')) {
                    token = X_TOKEN_HEX;
                    x_scanner_get_char(scanner, line_p, position_p);

                    ch = x_scanner_get_char(scanner, line_p, position_p);
                    if (ch == 0) {
                        token = X_TOKEN_ERROR;
                        value.v_error = X_ERR_UNEXP_EOF;
                        (*position_p)++;
                        break;
                    }

                    if (x_scanner_char_2_num(ch, 16) < 0) {
                        token = X_TOKEN_ERROR;
                        value.v_error = X_ERR_DIGIT_RADIX;
                        ch = 0;
                        break;
                    }
                } else if (config->scan_binary && (ch == 'b' || ch == 'B')) {
                    token = X_TOKEN_BINARY;
                    x_scanner_get_char(scanner, line_p, position_p);

                    ch = x_scanner_get_char(scanner, line_p, position_p);
                    if (ch == 0) {
                        token = X_TOKEN_ERROR;
                        value.v_error = X_ERR_UNEXP_EOF;
                        (*position_p)++;
                        break;
                    }

                    if (x_scanner_char_2_num(ch, 10) < 0) {
                        token = X_TOKEN_ERROR;
                        value.v_error = X_ERR_NON_DIGIT_IN_CONST;
                        ch = 0;
                        break;
                    }
                } else {
                    ch = '0';
                }
                X_GNUC_FALLTHROUGH;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
number_parsing: {
                    xchar *endptr;
                    xboolean in_number = TRUE;

                    if (token == X_TOKEN_NONE) {
                        token = X_TOKEN_INT;
                    }

                    gstring = x_string_new(dotted_float ? "0." : "");
                    gstring = x_string_append_c(gstring, ch);

                    do {
                        xboolean is_E;

                        is_E = token == X_TOKEN_FLOAT && (ch == 'e' || ch == 'E');
                        ch = x_scanner_peek_next_char(scanner);

                        if (x_scanner_char_2_num(ch, 36) >= 0 || (config->scan_float && ch == '.') || (is_E && (ch == '+' || ch == '-'))) {
                            ch = x_scanner_get_char(scanner, line_p, position_p);
                            switch (ch) {
                                case '.':
                                    if (token != X_TOKEN_INT && token != X_TOKEN_OCTAL) {
                                        value.v_error = token == X_TOKEN_FLOAT ? X_ERR_FLOAT_MALFORMED : X_ERR_FLOAT_RADIX;
                                        token = X_TOKEN_ERROR;
                                        in_number = FALSE;
                                    } else {
                                        token = X_TOKEN_FLOAT;
                                        gstring = x_string_append_c(gstring, ch);
                                    }
                                    break;

                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                    gstring = x_string_append_c(gstring, ch);
                                    break;

                                case '-':
                                case '+':
                                    if (token != X_TOKEN_FLOAT) {
                                        token = X_TOKEN_ERROR;
                                        value.v_error = X_ERR_NON_DIGIT_IN_CONST;
                                        in_number = FALSE;
                                    } else {
                                        gstring = x_string_append_c(gstring, ch);
                                    }
                                    break;

                                case 'e':
                                case 'E':
                                    if ((token != X_TOKEN_HEX && !config->scan_float) || (token != X_TOKEN_HEX && token != X_TOKEN_OCTAL && token != X_TOKEN_FLOAT && token != X_TOKEN_INT)) {
                                        token = X_TOKEN_ERROR;
                                        value.v_error = X_ERR_NON_DIGIT_IN_CONST;
                                        in_number = FALSE;
                                    } else {
                                        if (token != X_TOKEN_HEX) {
                                            token = X_TOKEN_FLOAT;
                                        }

                                        gstring = x_string_append_c(gstring, ch);
                                    }
                                    break;

                                default:
                                    if (token != X_TOKEN_HEX) {
                                        token = X_TOKEN_ERROR;
                                        value.v_error = X_ERR_NON_DIGIT_IN_CONST;
                                        in_number = FALSE;
                                    } else {
                                        gstring = x_string_append_c(gstring, ch);
                                    }
                                    break;
                            }
                        } else {
                            in_number = FALSE;
                        }
                    } while (in_number);

                    endptr = NULL;
                    if (token == X_TOKEN_FLOAT) {
                        value.v_float = x_strtod(gstring->str, &endptr);
                    } else {
                        xuint64 ui64 = 0;
                        switch (token) {
                            case X_TOKEN_BINARY:
                                ui64 = x_ascii_strtoull(gstring->str, &endptr, 2);
                                break;

                            case X_TOKEN_OCTAL:
                                ui64 = x_ascii_strtoull(gstring->str, &endptr, 8);
                                break;

                            case X_TOKEN_INT:
                                ui64 = x_ascii_strtoull(gstring->str, &endptr, 10);
                                break;

                            case X_TOKEN_HEX:
                                ui64 = x_ascii_strtoull(gstring->str, &endptr, 16);
                                break;

                            default:;
                        }
    
                        if (scanner->config->store_int64) {
                            value.v_int64 = ui64;
                        } else {
                            value.v_int = ui64;
                        }
                    }

                    if (endptr && *endptr) {
                        token = X_TOKEN_ERROR;
                        if (*endptr == 'e' || *endptr == 'E') {
                            value.v_error = X_ERR_NON_DIGIT_IN_CONST;
                        } else {
                            value.v_error = X_ERR_DIGIT_RADIX;
                        }
                    }

                    x_string_free(gstring, TRUE);
                    gstring = NULL;
                    ch = 0;
                }
                break;

            default:
            default_case: {
                if (config->cpair_comment_single && ch == config->cpair_comment_single[0]) {
                    token = X_TOKEN_COMMENT_SINGLE;
                    in_comment_single = TRUE;
                    gstring = x_string_new(NULL);

                    ch = x_scanner_get_char(scanner, line_p, position_p);
                    while (ch != 0) {
                        if (ch == config->cpair_comment_single[1]) {
                            in_comment_single = FALSE;
                            ch = 0;
                            break;
                        }
            
                        gstring = x_string_append_c(gstring, ch);
                        ch = x_scanner_get_char(scanner, line_p, position_p);
                    }

                    if (in_comment_single && config->cpair_comment_single[1] == '\n') {
                        in_comment_single = FALSE;
                    }
                } else if (config->scan_identifier && ch && strchr(config->cset_identifier_first, ch)) {
identifier_precedence:
                    if (config->cset_identifier_nth && ch && strchr(config->cset_identifier_nth, x_scanner_peek_next_char (scanner))) {
                        token = X_TOKEN_IDENTIFIER;
                        gstring = x_string_new(NULL);
                        gstring = x_string_append_c(gstring, ch);

                        do {
                            ch = x_scanner_get_char(scanner, line_p, position_p);
                            gstring = x_string_append_c(gstring, ch);
                            ch = x_scanner_peek_next_char(scanner);
                        } while (ch && strchr(config->cset_identifier_nth, ch));

                        ch = 0;
                    } else if (config->scan_identifier_1char) {
                        token = X_TOKEN_IDENTIFIER;
                        value.v_identifier = x_new0(xchar, 2);
                        value.v_identifier[0] = ch;
                        ch = 0;
                    }
                }

                if (ch) {
                    if (config->char_2_token) {
                        token = (XTokenType)ch;
                    } else {
                        token = X_TOKEN_CHAR;
                        value.v_char = ch;
                    }

                    ch = 0;
                }
            }
            break;
        }

        x_assert(ch == 0 && token != X_TOKEN_NONE);
    } while (ch != 0);

    if (in_comment_multi || in_comment_single || in_string_sq || in_string_dq) {
        token = X_TOKEN_ERROR;
        if (gstring) {
            x_string_free(gstring, TRUE);
            gstring = NULL;
        }

        (*position_p)++;
        if (in_comment_multi || in_comment_single) {
            value.v_error = X_ERR_UNEXP_EOF_IN_COMMENT;
        } else {
            value.v_error = X_ERR_UNEXP_EOF_IN_STRING;
        }
    }

    if (gstring) {
        value.v_string = x_string_free(gstring, FALSE);
        gstring = NULL;
    }

    if (token == X_TOKEN_IDENTIFIER) {
        if (config->scan_symbols) {
            xuint scope_id;
            XScannerKey *key;

            scope_id = scanner->scope_id;
            key = x_scanner_lookup_internal(scanner, scope_id, value.v_identifier);
            if (!key && scope_id && scanner->config->scope_0_fallback) {
                key = x_scanner_lookup_internal(scanner, 0, value.v_identifier);
            }

            if (key) {
                x_free(value.v_identifier);
                token = X_TOKEN_SYMBOL;
                value.v_symbol = key->value;
            }
        }

        if (token == X_TOKEN_IDENTIFIER && config->scan_identifier_NULL && strlen(value.v_identifier) == 4) {
            xchar *null_upper = "NULL";
            xchar *null_lower = "null";

            if (scanner->config->case_sensitive) {
                if (value.v_identifier[0] == null_upper[0] && value.v_identifier[1] == null_upper[1] && value.v_identifier[2] == null_upper[2] && value.v_identifier[3] == null_upper[3]) {
                    token = X_TOKEN_IDENTIFIER_NULL;
                }
            } else {
                if ((value.v_identifier[0] == null_upper[0] ||
                    value.v_identifier[0] == null_lower[0]) &&
                    (value.v_identifier[1] == null_upper[1] ||
                    value.v_identifier[1] == null_lower[1]) &&
                    (value.v_identifier[2] == null_upper[2] ||
                    value.v_identifier[2] == null_lower[2]) &&
                    (value.v_identifier[3] == null_upper[3] ||
                    value.v_identifier[3] == null_lower[3]))
                {
                    token = X_TOKEN_IDENTIFIER_NULL;
                }
            }
        }
    }

    *token_p = token;
    *value_p = value;
}
