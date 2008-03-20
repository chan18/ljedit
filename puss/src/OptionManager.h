// OptionManager.h
//

#ifndef PUSS_INC_OPTION_MANAGER_H
#define PUSS_INC_OPTION_MANAGER_H

#include <gtk/gtk.h>

typedef gboolean	(*OptionSetter)(GtkWindow* parent, GKeyFile* options, const gchar* group, const gchar* key, gpointer tag);
typedef void		(*OptionChanged)(const gchar* group, const gchar* key, const gchar* new_value, const gchar* current_value, gpointer tag);

gboolean	puss_option_manager_create();
void		puss_option_manager_destroy();

gboolean	puss_default_option_setter(GtkWindow* parent, GKeyFile* options, const gchar* group, const gchar* key, gpointer tag);

gboolean	puss_option_manager_option_reg(const gchar* group, const gchar* key, const gchar* default_value, OptionSetter fun, gpointer tag);
gboolean	puss_option_manager_monitor_reg(const gchar* group, const gchar* key, OptionChanged fun, gpointer tag);

void		puss_option_manager_active();

#endif//PUSS_INC_OPTION_MANAGER_H

