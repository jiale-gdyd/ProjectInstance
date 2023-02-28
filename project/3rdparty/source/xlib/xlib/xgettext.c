#include <string.h>
#include <locale.h>
#include <libintl.h>

#include <xlib/xlib/config.h>

#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xalloca.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xgettext.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xlib-private.h>

static void ensure_gettext_initialized(void)
{
    static xsize initialised;

    if (x_once_init_enter(&initialised)) {
        bindtextdomain(GETTEXT_PACKAGE, XLIB_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
        bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
        x_once_init_leave(&initialised, TRUE);
    }
}

const xchar *xlib_gettext(const xchar *str)
{
    ensure_gettext_initialized();
    return x_dgettext(GETTEXT_PACKAGE, str);
}

const xchar *xlib_pgettext(const xchar *msgctxtid, xsize msgidoffset)
{
    ensure_gettext_initialized();
    return x_dpgettext(GETTEXT_PACKAGE, msgctxtid, msgidoffset);
}

const xchar *x_strip_context(const xchar *msgid, const xchar *msgval)
{
    if (msgval == msgid) {
        const char *c = strchr(msgid, '|');
        if (c != NULL) {
            return c + 1;
        }
    }

    return msgval;
}

const xchar *x_dpgettext(const xchar *domain, const xchar *msgctxtid, xsize msgidoffset)
{
    xchar *sep;
    const xchar *translation;

    translation = x_dgettext(domain, msgctxtid);

    if (translation == msgctxtid) {
        if (msgidoffset > 0) {
            return msgctxtid + msgidoffset;
        }

        sep = (xchar *)strchr((const char *)msgctxtid, '|');
        if (sep) {
            xchar *tmp = (xchar *)x_alloca(strlen(msgctxtid) + 1);
            strcpy(tmp, msgctxtid);
            tmp[sep - msgctxtid] = '\004';

            translation = x_dgettext(domain, tmp);
            if (translation == tmp) {
                return sep + 1;
            }
        }
    }

    return translation;
}

const xchar *x_dpgettext2(const xchar *domain, const xchar *msgctxt, const xchar *msgid)
{
    char *msg_ctxt_id;
    const char *translation;
    size_t msgid_len = strlen(msgid) + 1;
    size_t msgctxt_len = strlen(msgctxt) + 1;

    msg_ctxt_id = (char *)x_alloca(msgctxt_len + msgid_len);

    memcpy(msg_ctxt_id, msgctxt, msgctxt_len - 1);
    msg_ctxt_id[msgctxt_len - 1] = '\004';
    memcpy(msg_ctxt_id + msgctxt_len, msgid, msgid_len);

    translation = x_dgettext(domain, msg_ctxt_id);

    if (translation == msg_ctxt_id) {
        msg_ctxt_id[msgctxt_len - 1] = '|';
        translation = x_dgettext(domain, msg_ctxt_id);

        if (translation == msg_ctxt_id) {
            return msgid;
        }
    }

    return translation;
}

static xboolean _x_dgettext_should_translate(void)
{
    static xsize translate = 0;
    enum {
        SHOULD_TRANSLATE     = 1,
        SHOULD_NOT_TRANSLATE = 2
    };

    if (X_UNLIKELY(x_once_init_enter(&translate))) {
        xboolean should_translate = TRUE;

        const char *default_domain = textdomain(NULL);
        const char *translator_comment = gettext("");
        const char *translate_locale = setlocale(LC_MESSAGES, NULL);

        if (!default_domain || !translator_comment || !translate_locale
            || (0 != strcmp(default_domain, "messages")
            && '\0' == *translator_comment
            && 0 != strncmp(translate_locale, "en_", 3)
            && 0 != strcmp(translate_locale, "C")))
        {
            should_translate = FALSE;
        }

        x_once_init_leave(&translate, should_translate ? SHOULD_TRANSLATE : SHOULD_NOT_TRANSLATE);
    }

    return translate == SHOULD_TRANSLATE;
}

const xchar *x_dgettext (const xchar *domain, const xchar *msgid)
{
    if (domain && X_UNLIKELY(!_x_dgettext_should_translate())) {
        return msgid;
    }

    return dgettext(domain, msgid);
}

const xchar *x_dcgettext(const xchar *domain, const xchar *msgid, xint category)
{
    if (domain && X_UNLIKELY(!_x_dgettext_should_translate())) {
        return msgid;
    }

    return dcgettext(domain, msgid, category);
}

const xchar *x_dngettext(const xchar *domain, const xchar *msgid, const xchar *msgid_plural, xulong n)
{
    if (domain && X_UNLIKELY(!_x_dgettext_should_translate())) {
        return n == 1 ? msgid : msgid_plural;
    }

    return dngettext(domain, msgid, msgid_plural, n);
}
