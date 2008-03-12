// OutlinePage.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_OUTLINEPAGE_H
#define PUSS_EXTEND_INC_LJCS_OUTLINEPAGE_H

#include <glib.h>

namespace cpp {
	class File;
}

class Environ;
class Icons;
struct Puss;

class OutlinePage;

OutlinePage* outline_page_create(Puss* app, Environ* env, Icons* icons);

void outline_page_destroy(OutlinePage* self);

void preview_page_set_file(OutlinePage* self, cpp::File* file, gint line);

#endif//PUSS_EXTEND_INC_LJCS_OUTLINEPAGE_H

