
#ifndef __X_I18N_H__
#define __X_I18N_H__

#include <string.h>
#include "../xlib.h"
#include "libintl.h"

#define  _(String)              gettext(String)
#define Q_(String)              x_dpgettext(NULL, String, 0)
#define N_(String)              (String)
#define C_(Context, String)     x_dpgettext(NULL, Context "\004" String, strlen(Context) + 1)
#define NC_(Context, String)    (String)

#endif
