// GlobMatch.cpp
//

#include "GlobMatch.h"

#include <glib.h>
#include <string.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

#include "Puss.h"


void glob_matcher_add_glob(GHashTable* suffix_map, const gchar* glob, GtkSourceLanguage* lang) {
	gchar* suffix;
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

	suffix = g_strdup(p);
	if( suffix )
		g_hash_table_replace(suffix_map, suffix, lang);
}

GHashTable* glob_get_suffix_map() {
	static GQuark quark_glob_suffix_map = 0;

	GtkSourceLanguageManager* lm;
	G_CONST_RETURN gchar* G_CONST_RETURN* languages;
	G_CONST_RETURN gchar* G_CONST_RETURN* it;
	GtkSourceLanguage* lang;
	gchar** globs;
	gchar** git;

	GObject* owner;
	GHashTable* suffix_map;

	if( quark_glob_suffix_map==0 )
		quark_glob_suffix_map = g_quark_from_static_string("puss_glob_suffix_map");

	owner = G_OBJECT(puss_app->doc_panel);
	suffix_map = (GHashTable*)g_object_get_qdata(owner, quark_glob_suffix_map);
	if( !suffix_map ) {
		suffix_map = g_hash_table_new_full( &g_str_hash, &g_str_equal, &g_free, NULL );
		if( suffix_map ) {
			g_object_set_qdata_full(owner, quark_glob_suffix_map, suffix_map, (GDestroyNotify)&g_hash_table_destroy);

			lm = gtk_source_language_manager_get_default();
			glob_matcher_add_glob(suffix_map, "makefile", gtk_source_language_manager_get_language(lm, "makefile"));
			glob_matcher_add_glob(suffix_map, "Makefile", gtk_source_language_manager_get_language(lm, "makefile"));
			glob_matcher_add_glob(suffix_map, "GNUMakefile", gtk_source_language_manager_get_language(lm, "makefile"));
			glob_matcher_add_glob(suffix_map, "*.mk", gtk_source_language_manager_get_language(lm, "makefile"));

			glob_matcher_add_glob(suffix_map, "*.f90", gtk_source_language_manager_get_language(lm, "fortran"));
			glob_matcher_add_glob(suffix_map, "*.f95", gtk_source_language_manager_get_language(lm, "fortran"));

			languages = gtk_source_language_manager_get_language_ids(lm);
			for( it = languages; *it; ++it ) {
				lang = gtk_source_language_manager_get_language(lm, *it);
				globs = gtk_source_language_get_globs(lang);
				if( !globs )
					continue;

				for( git = globs; *git; ++git )
					glob_matcher_add_glob(suffix_map, *git, lang);
					
				g_strfreev(globs);
			}

		}
	}

	return suffix_map;
}

GtkSourceLanguage* puss_glob_get_language_by_filename(const gchar* filepath) {
	gchar* suffix;
	GHashTable* suffix_map;
	GtkSourceLanguage* lang = 0;
	gchar* filename = g_path_get_basename(filepath);
	if( filename ) {
		suffix = filename + strlen(filename);
		for( ; suffix > filename; --suffix )
			if( *(suffix-1)=='.' )
				break;

		suffix_map = glob_get_suffix_map();
		lang = (GtkSourceLanguage*)g_hash_table_lookup(suffix_map, suffix);
		g_free(filename);
	}

	return lang;
}

