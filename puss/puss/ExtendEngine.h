// ExtendEngine.h
//

#ifndef PUSS_INC_EXTENDENGINE_H
#define PUSS_INC_EXTENDENGINE_H

#include <gtk/gtk.h>

gboolean	puss_extend_engine_create();
void		puss_extend_engine_destroy();

gpointer	puss_extend_engine_query(const gchar* ext_name, const gchar* interface_name);

#endif//PUSS_INC_EXTENDENGINE_H

