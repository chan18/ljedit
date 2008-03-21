// OptionManager.h
//

#ifndef PUSS_INC_OPTION_MANAGER_H
#define PUSS_INC_OPTION_MANAGER_H

#include <gtk/gtk.h>

struct Option;

typedef gboolean	(*OptionSetter)(GtkWindow* parent, Option* option, gpointer tag);
typedef void		(*OptionChanged)(const Option* option, const gchar* old, gpointer tag);

gboolean		puss_option_manager_create();
void			puss_option_manager_destroy();

const Option*	puss_option_manager_find_option(const gchar* group, const gchar* key);

gboolean		puss_default_option_setter(GtkWindow* parent, Option* option, gpointer tag);

const Option*	puss_option_manager_option_reg(const gchar* group, const gchar* key, const gchar* default_value, OptionSetter fun, gpointer tag);
gboolean		puss_option_manager_monitor_reg(const Option* option, OptionChanged fun, gpointer tag);

void			puss_option_manager_active();

#endif//PUSS_INC_OPTION_MANAGER_H

