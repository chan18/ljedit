// Tips.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_TIPS_H
#define PUSS_EXTEND_INC_LJCS_TIPS_H

#include <glib.h>
#include <string>
#include "ljcs/ljcs.h"

class Environ;
class Icons;
typedef struct _Puss Puss;

struct Tips;

typedef std::set<std::string> StringSet;

Tips* tips_create(Puss* app, Environ* env, Icons* icons);
void tips_destroy(Tips* self);

gboolean tips_include_is_visible(Tips* self);
gboolean tips_list_is_visible(Tips* self);
gboolean tips_decl_is_visible(Tips* self);

void tips_include_tip_show(Tips* self, gint x, gint y, StringSet& files);
void tips_include_tip_hide(Tips* self);

void tips_list_tip_show(Tips* self, gint x, gint y, cpp::ElementSet& mset);
void tips_list_tip_hide(Tips* self);

void tips_decl_tip_show(Tips* self, gint x, gint y, cpp::ElementSet& mset);
void tips_decl_tip_hide(Tips* self);

inline void tips_hide_all( Tips* self ) {
	tips_include_tip_hide(self);
	tips_list_tip_hide(self);
	tips_decl_tip_hide(self);
}

gboolean tips_locate_sub(Tips* self, gint x, gint y, const gchar* key);

cpp::Element* tips_list_get_selected(Tips* self);
const gchar* tips_include_get_selected(Tips* self);

void tips_select_next(Tips* self);
void tips_select_prev(Tips* self);

#endif//PUSS_EXTEND_INC_LJCS_TIPS_H

