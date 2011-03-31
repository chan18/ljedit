// Utils.h
//

#ifndef PUSS_INC_UTILS_H
#define PUSS_INC_UTILS_H

#include <gtk/gtk.h>

gboolean	puss_utils_create();
void		puss_utils_destroy();

void		puss_send_focus_change(GtkWidget* widget, gboolean in);

void		puss_active_panel_page(GtkNotebook* panel, gint page_num);

gboolean	puss_save_file(const gchar* filename, const gchar* text, gssize len, const gchar* charset, gboolean use_BOM);
gboolean	puss_load_file(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, gboolean* use_BOM);

gchar*		puss_format_filename(const gchar* filename);

#include <libintl.h>

#define TEXT_DOMAIN "puss"

#ifdef _
#error "puss need _ as i18n macro!"
#endif

#define _(str) dgettext(TEXT_DOMAIN, str)

#endif//PUSS_INC_UTILS_H

