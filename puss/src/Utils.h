// Utils.h
//

#ifndef PUSS_INC_UTILS_H
#define PUSS_INC_UTILS_H

#include <gtk/gtk.h>

void	puss_send_focus_change(GtkWidget* widget, gboolean in);

void	puss_active_panel_page(GtkNotebook* panel, gint page_num);

#endif//PUSS_INC_UTILS_H

