// PreviewPage.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H
#define PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H

#include <glib.h>

namespace cpp {
	class File;
}

class Environ;
struct Puss;

class PreviewPage;

PreviewPage* preview_page_create(Puss* app, Environ* env);

void preview_page_destroy(PreviewPage* self);

void preview_page_preview(PreviewPage* self, const gchar* key, const gchar* key_text, cpp::File& file, size_t line);

#endif//PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H

