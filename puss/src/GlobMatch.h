// GlobMatch.h
//

#ifndef PUSS_INC_GLOBMATCH_H
#define PUSS_INC_GLOBMATCH_H

#include <gtksourceview/gtksourcelanguage.h>

// use glob match
// 

GHashTable*	puss_glob_create();
void		puss_glob_destroy(GHashTable* suffix_map);

GtkSourceLanguage* puss_glob_get_language_by_filename(GHashTable* suffix_map, const gchar* filepath);

#endif//PUSS_INC_GLOBMATCH_H

