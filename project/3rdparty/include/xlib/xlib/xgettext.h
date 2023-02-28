#ifndef __X_GETTEXT_H__
#define __X_GETTEXT_H__

#include "xtypes.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
const xchar *x_strip_context(const xchar *msgid, const xchar *msgval) X_GNUC_FORMAT(1);

XLIB_AVAILABLE_IN_ALL
const xchar *x_dgettext(const xchar *domain, const xchar *msgid) X_GNUC_FORMAT(2);

XLIB_AVAILABLE_IN_ALL
const xchar *x_dcgettext(const xchar *domain, const xchar *msgid, xint category) X_GNUC_FORMAT(2);

XLIB_AVAILABLE_IN_ALL
const xchar *x_dngettext(const xchar *domain, const xchar *msgid, const xchar *msgid_plural, xulong n) X_GNUC_FORMAT(3);

XLIB_AVAILABLE_IN_ALL
const xchar *x_dpgettext(const xchar *domain, const xchar *msgctxtid, xsize msgidoffset) X_GNUC_FORMAT(2);

XLIB_AVAILABLE_IN_ALL
const xchar *x_dpgettext2(const xchar *domain, const xchar *context, const xchar *msgid) X_GNUC_FORMAT(3);

X_END_DECLS

#endif
