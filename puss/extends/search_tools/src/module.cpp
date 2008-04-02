// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

struct SearchTools {
	Puss*			app;

	GtkEntry*		search_entry;
	GtkTreeView*	result_view;
	GtkListStore*	result_store;
};

void search_in_file(const gchar* filename, const gchar* search_text, SearchTools* self) {
	gchar* text = 0;
	gsize len = 0;
	if( !self->app->load_file(filename, &text, &len, 0) )
		return;

	g_assert( text );

	gchar* end = text + len;
	gsize line = 0;
	gchar* ps = text;
	gchar* pe = text;
	while( ps < end ) {
		for( pe=ps; pe < end; ++pe )
			if( *pe=='\r' || *pe=='\n' )
				break;
		++line;

		gchar* pos = g_strstr_len(ps, (gsize)(pe-ps), search_text);
		if( pos ) {
			GtkTreeIter iter;
			gtk_list_store_append(self->result_store, &iter);
			gtk_list_store_set(self->result_store, &iter, 0, g_strdup(filename), 1, line, 2, g_strndup(ps, (gsize)(pe-ps)), -1);	// TODO : not finish!!! memory-lack!! test only
		}

		for( ps=pe; ps < end; ++ps )
			if( *ps!='\r' && *ps!='\n' )
				break;
	}

	g_free(text);
}

SIGNAL_CALLBACK gboolean search_tools_search(GtkEntry* entry, GdkEventKey* event, SearchTools* self) {
	if( event->keyval==GDK_Return )
	{
		const gchar* text = gtk_entry_get_text(entry);

		gtk_list_store_clear(self->result_store);

		gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
		if( page_num < 0 )
			return TRUE;

		GtkTextBuffer* buf = self->app->doc_get_buffer_from_page_num(page_num);
		if( !buf )
			return TRUE;

		GString* url = self->app->doc_get_url(buf);
		if( url )
			search_in_file(url->str, text, self);
	}
	return FALSE;
}


gboolean search_tools_active_search_entry(GtkWidget* widget, GdkEventFocus* event, SearchTools* self) {
	gtk_widget_grab_focus(GTK_WIDGET(self->search_entry));
	return TRUE;
}

void search_tools_build_ui(SearchTools* self) {
	GtkBuilder* builder = gtk_builder_new();
	if( !builder )
		return;

	gchar* filepath = g_build_filename(self->app->get_module_path(), "extends", "search_tools_res", "search_tools_ui.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(search_tools) : build preview page ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	GError* err = 0;
	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(search_tools): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	GtkWidget* panel = GTK_WIDGET(gtk_builder_get_object(builder, "search_tools_panel"));
	self->search_entry = GTK_ENTRY(gtk_builder_get_object(builder, "search_entry"));
	self->result_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "search_result_store"));
	self->result_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "search_result_view"));
	g_assert( panel && self->search_entry && self->result_store && self->result_view );

	gtk_widget_show_all(panel);
	gtk_notebook_append_page(puss_get_bottom_panel(self->app), panel, gtk_label_new(_("Search")));
	g_signal_connect(panel, "focus-in-event",G_CALLBACK(&search_tools_active_search_entry), self);

	gtk_builder_connect_signals(builder, self);
	g_object_unref(G_OBJECT(builder));
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	SearchTools* self = g_new0(SearchTools, 1);
	if( self ) {
		self->app = app;
		search_tools_build_ui(self);
	}

	return self;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	g_free(self);
}

