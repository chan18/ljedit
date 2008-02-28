// GlobMatch.cpp
//

#include "GlobMatch.h"

#include <glib.h>

void glob_matcher_add_glob(GHashTable* suffix_map, const gchar* glob, GtkSourceLanguage* lang) {
	const gchar* p = glob;

	if( !p || !lang )
		return;

	if( p[0]=='\0' )
		return;

	if( p[0]=='*' && p[1]=='.' ) {
		// now ignore glob like these *.f9[05] ...
		for( p+=2; *p; ++p )
			if( *p=='[' || *p=='.' )
				return;

		p = glob + 2;

	} else {
		// now ignore glob like these [Mm]akefile
		for( p+=2; *p; ++p )
			if( *p=='[' )
				return;

		p = glob;
	}

	GtkSourceLanguage* res = (GtkSourceLanguage*)g_hash_table_find(suffix_map, g_str_equal, p);
	if( res ) {
		if( res != lang )
			g_hash_table_insert(suffix_map, p, lang);

	} else {
		gchar* suffix = g_strdup(p);
		if( suffix )
			g_hash_table_insert(suffix_map, p, lang);
	}
}

GHashTable* puss_glob_create() {
	GHashTable* suffix_map = g_hash_table_new_full( &g_str_hash, &g_str_equal, &g_free, NULL );
	if( !suffix_map )
		return 0;

	GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
	glob_matcher_add_glob(suffix_map, "makefile", gtk_source_language_manager_get_language(lm, "makefile"));
	glob_matcher_add_glob(suffix_map, "Makefile", gtk_source_language_manager_get_language(lm, "makefile"));
	glob_matcher_add_glob(suffix_map, "GNUMakefile", gtk_source_language_manager_get_language(lm, "makefile"));
	glob_matcher_add_glob(suffix_map, "*.mk", gtk_source_language_manager_get_language(lm, "makefile"));

	glob_matcher_add_glob(suffix_map, "*.f90", gtk_source_language_manager_get_language(lm, "fortran"));
	glob_matcher_add_glob(suffix_map, "*.f95", gtk_source_language_manager_get_language(lm, "fortran"));

	const gchar** languages = gtk_source_language_manager_get_language_ids(lm);
	for( const gchar** it = languages; *it; ++it ) {
		GtkSourceLanguage* lang = gtk_source_language_manager_get_language(*it);
		gchar** globs = gtk_source_language_get_globs(lang);
		if( !globs )
			continue;

		for( gchar** git = globs; *git; ++git )
			glob_matcher_add_glob(suffix_map, *git, lang)
			
		g_strfreev(globs);
	}

	return suffix_map;
}

void puss_glob_destroy(GHashTable* suffix_map) {
	if( !suffix_map )
		return;

	g_hash_table_destroy(suffix_map);
}

GtkSourceLanguage* puss_glob_get_language_by_filename(GHashTable* suffix_map, const gchar* filepath) {
	GtkSourceLanguage* lang = 0;
	gchar* filename = g_path_get_basename(filepath);
	if( filename ) {
		gchar* suffix = filename + strlen(filename);
		for( ; suffix > filename; --suffix )
			if( *(suffix-1)=='.' )
				break;

		lang = g_hash_table_lookup(suffix_map, suffix);
		g_free(filename);
	}

	return lang;
}

