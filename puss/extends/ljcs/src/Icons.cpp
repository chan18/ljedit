// Icons.cpp
// 

#include "Icons.h"

#include <glib/gi18n.h>

#include <glib.h>

#include "ljcs/ds.h"
#include "IPuss.h"


GdkPixbuf* load_icon(const gchar* app_path, const gchar* name) {
	gchar* filepath = g_build_filename(app_path, "extends", "ljcs_res", name, NULL);

	GdkPixbuf* icon = gdk_pixbuf_new_from_file(filepath, NULL);
	if( !icon )
		g_printerr(_("* ERROR : ljcs load icon(%s) failed!"), filepath);

	g_free(filepath);

	return icon;
}

void free_icon(GdkPixbuf* icon) {
	if( icon )
		g_object_unref(G_OBJECT(icon));
}

void Icons::create(Puss* app) {
	icon_keyword   = load_icon(app->module_path, "keyword.png");
	icon_var       = load_icon(app->module_path, "var.png");
	icon_fun       = load_icon(app->module_path, "fun.png");
	icon_macro     = load_icon(app->module_path, "macro.png");
	icon_class     = load_icon(app->module_path, "class.png");
	icon_typedef   = load_icon(app->module_path, "typedef.png");
	icon_namespace = load_icon(app->module_path, "namespace.png");
	icon_enum      = load_icon(app->module_path, "enum.png");
	icon_enumitem  = load_icon(app->module_path, "enumitem.png");
}

void Icons::destroy() {
	free_icon(icon_keyword);
	free_icon(icon_var);
	free_icon(icon_fun);
	free_icon(icon_macro);
	free_icon(icon_class);
	free_icon(icon_typedef);
	free_icon(icon_namespace);
	free_icon(icon_enum);
	free_icon(icon_enumitem);
}

GdkPixbuf* Icons::get_icon_from_elem(const cpp::Element& elem) {
	switch( elem.type ) {
	case cpp::ET_KEYWORD:	return icon_keyword;
	case cpp::ET_VAR:		return icon_var;
	case cpp::ET_FUN:		return icon_fun;
	case cpp::ET_MACRO:		return icon_macro;
	case cpp::ET_CLASS:		return icon_class;
	case cpp::ET_TYPEDEF:	return icon_typedef;
	case cpp::ET_NAMESPACE:	return icon_namespace;
	case cpp::ET_ENUM:		return icon_enum;
	case cpp::ET_ENUMITEM:	return icon_enumitem;
	}

	return 0;
}

