// PanelOrder.h
// 

#ifndef PUSS_INC_PANELORDER_H
#define PUSS_INC_PANELORDER_H

#include "IPuss.h"

void puss_panel_order_load();
void puss_panel_order_save();

gboolean puss_panel_get_pos(GtkWidget* panel, GtkNotebook** parent, gint* page_num);
void puss_panel_append(GtkWidget* panel, GtkWidget* tab_label, const gchar* id, PanelPosition default_pos);
void puss_panel_remove(GtkWidget* panel);

#endif//PUSS_INC_PANELORDER_H

