// Utils.h
//

#ifndef PUSS_INC_UTILS_H
#define PUSS_INC_UTILS_H

#include <gtk/gtk.h>

void		puss_send_focus_change(GtkWidget* widget, gboolean in);

void		puss_active_panel_page(GtkNotebook* panel, gint page_num);

gboolean	puss_load_file(const gchar* filename, gchar** text, gsize* len, G_CONST_RETURN gchar** charset, GError** err);

#endif//PUSS_INC_UTILS_H

