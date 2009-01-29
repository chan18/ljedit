// OptionSetup.h
//

#ifndef PUSS_INC_OPTION_SETUP_H
#define PUSS_INC_OPTION_SETUP_H

#include "IPuss.h"

gboolean	puss_option_setup_create();
void		puss_option_setup_destroy();

gboolean	puss_option_setup_reg(const gchar* id, const gchar* name, CreateSetupWidget creator, gpointer tag, GDestroyNotify tag_destroy);
void		puss_option_setup_unreg(const gchar* id);

void		puss_option_setup_show_dialog(const gchar* filter);

#endif//PUSS_INC_OPTION_SETUP_H

