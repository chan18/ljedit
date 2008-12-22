// Icons.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_ICONS_H
#define PUSS_EXTEND_INC_LJCS_ICONS_H

#include <gdk/gdkpixbuf.h>

typedef struct _Puss Puss;

namespace cpp {
	class Element;
}

class Icons
{
public:
	void create(Puss* app);

	void destroy();

	GdkPixbuf* get_icon_from_elem(const cpp::Element& elem);

private:
	GdkPixbuf*	icon_keyword;
	GdkPixbuf*	icon_var;
	GdkPixbuf*	icon_fun;
	GdkPixbuf*	icon_macro;
	GdkPixbuf*	icon_class;
	GdkPixbuf*	icon_typedef;
	GdkPixbuf*	icon_namespace;
	GdkPixbuf*	icon_enum;
	GdkPixbuf*	icon_enumitem;
};

#endif//PUSS_EXTEND_INC_LJCS_ICONS_H

