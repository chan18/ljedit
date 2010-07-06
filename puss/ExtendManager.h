// ExtendManager.h
//

#ifndef PUSS_INC_EXTENDMANAGER_H
#define PUSS_INC_EXTENDMANAGER_H

#include <gtk/gtk.h>

gboolean	puss_extend_manager_create();
void		puss_extend_manager_destroy();

gpointer	puss_extend_manager_query(const gchar* ext_name, const gchar* interface_name);

#endif//PUSS_INC_EXTENDMANAGER_H

