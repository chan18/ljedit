// LJCSIcons.h
// 

#ifndef LJED_INC_LJCSICONS_H
#define LJED_INC_LJCSICONS_H

#include "LJEditor.h"
#include "LJCSEnv.h"


class LJCSIcons {
public:
	static LJCSIcons& self();

	void create(const std::string& plugin_path);

	Glib::RefPtr<Gdk::Pixbuf> get_icon_from_elem(const cpp::Element& elem);

	Glib::RefPtr<Gdk::Pixbuf>	icon_var;
	Glib::RefPtr<Gdk::Pixbuf>	icon_fun;
	Glib::RefPtr<Gdk::Pixbuf>	icon_macro;
	Glib::RefPtr<Gdk::Pixbuf>	icon_class;
	Glib::RefPtr<Gdk::Pixbuf>	icon_typedef;
	Glib::RefPtr<Gdk::Pixbuf>	icon_namespace;
	Glib::RefPtr<Gdk::Pixbuf>	icon_enum;
	Glib::RefPtr<Gdk::Pixbuf>	icon_enumitem;

private:
	LJCSIcons() {}
	~LJCSIcons() {}

	// no implements
	LJCSIcons(const LJCSIcons& o);
	LJCSIcons& operator = (const LJCSIcons& o);
};

#endif//LJED_INC_LJCSICONS_H

