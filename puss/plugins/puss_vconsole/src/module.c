// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "IPuss.h"

#include <libintl.h>

#define TEXT_DOMAIN "plugin_vconsole"

#define _(str) dgettext(TEXT_DOMAIN, str)

#ifdef WIN32
	#include "module.c.win"
#else
	#include "module.c.linux"
#endif
