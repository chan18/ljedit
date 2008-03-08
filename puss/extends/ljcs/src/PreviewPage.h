// PreviewPage.h
// 

#ifndef PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H
#define PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H

#include <gtk/gtk.h>
#include <string>

namespace cpp {
	class File;
}

struct Puss;

class PreviewPage {
public:
    gboolean create(Puss* app);
    void destroy();
};

#endif//PUSS_EXTEND_INC_LJCS_PREVIEWPAGE_H

