// LJCSIcons.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "LJCSIcons.h"

LJCSIcons& LJCSIcons::self() {
	static LJCSIcons me_;
	return me_;
}

Glib::RefPtr<Gdk::Pixbuf> load_icon(const std::string& path, const std::string& filename) {
	try {
		return Gdk::Pixbuf::create_from_file( Glib::build_filename(path, filename) );
	} catch( const Glib::Exception& ) {
	}

	Glib::RefPtr<Gdk::Pixbuf> empty_icon;
	return empty_icon;
}

void LJCSIcons::create(const std::string& plugin_path) {
	icon_keyword   = load_icon(plugin_path, "keyword.png");
	icon_var       = load_icon(plugin_path, "var.png");
	icon_fun       = load_icon(plugin_path, "fun.png");
	icon_macro     = load_icon(plugin_path, "macro.png");
	icon_class     = load_icon(plugin_path, "class.png");
	icon_typedef   = load_icon(plugin_path, "typedef.png");
	icon_namespace = load_icon(plugin_path, "namespace.png");
	icon_enum      = load_icon(plugin_path, "enum.png");
	icon_enumitem  = load_icon(plugin_path, "enumitem.png");
}

Glib::RefPtr<Gdk::Pixbuf> LJCSIcons::get_icon_from_elem(const cpp::Element& elem) {
	switch( elem.type ) {
	case cpp::ET_KEYWORD:	return LJCSIcons::self().icon_keyword;
	case cpp::ET_VAR:		return LJCSIcons::self().icon_var;
	case cpp::ET_FUN:		return LJCSIcons::self().icon_fun;
	case cpp::ET_MACRO:		return LJCSIcons::self().icon_macro;
	case cpp::ET_CLASS:		return LJCSIcons::self().icon_class;
	case cpp::ET_TYPEDEF:	return LJCSIcons::self().icon_typedef;
	case cpp::ET_NAMESPACE:	return LJCSIcons::self().icon_namespace;
	case cpp::ET_ENUM:		return LJCSIcons::self().icon_enum;
	case cpp::ET_ENUMITEM:	return LJCSIcons::self().icon_enumitem;
	}

	Glib::RefPtr<Gdk::Pixbuf> empty_icon;
	return empty_icon;
}

