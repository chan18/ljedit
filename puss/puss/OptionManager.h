// OptionManager.h
//

#ifndef PUSS_INC_OPTION_MANAGER_H
#define PUSS_INC_OPTION_MANAGER_H

#include "IPuss.h"

gboolean		puss_option_manager_create();
void			puss_option_manager_destroy();

/*
void			puss_option_manager_active();
*/

const Option*	puss_option_manager_option_reg(const gchar* group, const gchar* key, const gchar* default_value);
const Option*	puss_option_manager_option_find(const gchar* group, const gchar* key);
void			puss_option_manager_option_set(const Option* option, const gchar* value);

gpointer		puss_option_manager_monitor_reg(const Option* option, OptionChanged fun, gpointer tag, GFreeFunc tag_free_fun);
void			puss_option_manager_monitor_unreg(gpointer handler);

#endif//PUSS_INC_OPTION_MANAGER_H

