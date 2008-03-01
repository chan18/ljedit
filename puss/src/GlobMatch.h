// GlobMatch.h
//

#ifndef PUSS_INC_GLOBMATCH_H
#define PUSS_INC_GLOBMATCH_H

#include <gtksourceview/gtksourcelanguage.h>

struct Puss;

// use glob match
// 
GtkSourceLanguage* puss_glob_get_language_by_filename(Puss* app, const gchar* filepath);

#endif//PUSS_INC_GLOBMATCH_H

