// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>

#ifdef G_OS_WIN32
	#include <windows.h>

	void open_url(const gchar* link)
		{ ShellExecuteA(HWND_DESKTOP, "open", "C:/Program Files/Mozilla Firefox 3 Beta 4/firefox",  link, NULL, SW_SHOWNORMAL); }

#endif

struct DocModuleNode {
	gchar*		path;
	GHashTable*	index;
};

void doc_module_node_free(DocModuleNode* node, gpointer) {
	g_free(node->path);
	g_hash_table_destroy(node->index);
}

struct GtkDocHelper {
	Puss*		app;
	GRegex*		re_dt; 

	GList*		modules;	// DocModuleNode list
};

void parse_doc_module(GtkDocHelper* self, const gchar* path, const gchar* index_html_file) {
	gchar* index_filename = g_build_filename(path, index_html_file, NULL);
	if( !index_filename )
		return;

	gchar* text = 0;
	gsize len = 0;

	if( self->app->load_file(index_filename, &text, &len, 0) ) {
		GMatchInfo* info = 0;
		if( g_regex_match_full(self->re_dt, text, len, 0, (GRegexMatchFlags)0, &info, 0) ) {
			DocModuleNode* node = g_new0(DocModuleNode, 1);
			node->path = g_strdup(path);
			node->index = g_hash_table_new_full(&g_str_hash, &g_str_equal, &g_free, &g_free);

			self->modules = g_list_prepend(self->modules, node);

			do {
				g_hash_table_insert(node->index, g_match_info_fetch(info, 1), g_match_info_fetch(info, 2));
			} while( g_match_info_next(info, 0) );

			g_match_info_free(info);
		}
	}
	g_free(index_filename);
}

struct _FindTag {
	GtkDocHelper*	self;
	const gchar*	key;

	const gchar*	res_path;
	const gchar*	res_pos;
};

void __find_and_open_in_brower(DocModuleNode* node, _FindTag* tag) {
	if( !tag->res_path ) {
		gchar* value = (gchar*)g_hash_table_lookup(node->index, tag->key);
		if( value ) {
			tag->res_path = node->path;
			tag->res_pos = value;
		}
	}
}

void find_and_open_in_brower(GtkDocHelper* self, const gchar* key) {
	_FindTag tag = { self, key, 0, 0 };
	g_list_foreach(self->modules, (GFunc)&__find_and_open_in_brower, &tag);

	if( tag.res_path ) {
		gchar* url = g_build_filename("file:///", tag.res_path, tag.res_pos, NULL);
		open_url(url);
		g_free(url);
	}
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	GtkDocHelper* self = g_new0(GtkDocHelper, 1);

	self->app = app;
	self->re_dt = g_regex_new("<dt>(.*), <a class=\"indexterm\" href=\"(.*)\">", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

	//test
	parse_doc_module(self, "d:/gtk/share/gtk-doc/html/gtk", "ix01.html");
	parse_doc_module(self, "d:/gtk/share/gtk-doc/html/glib", "ix01.html");
	parse_doc_module(self, "d:/gtk/share/gtk-doc/html/gdk", "ix01.html");

	find_and_open_in_brower(self, "gtk_widget_show");
	return self;
}

PUSS_EXPORT void  puss_extend_destroy(void* ext) {
	if( ext ) {
		GtkDocHelper* self = (GtkDocHelper*)ext;
		g_regex_unref(self->re_dt);

		g_list_foreach(self->modules, (GFunc)&doc_module_node_free, 0);
		g_list_free(self->modules);

		g_free(ext);
	}
}

