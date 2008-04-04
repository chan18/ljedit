// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

struct ResultNode {
	const char*		filename;
	gint			line;
	gchar*			preview;

	ResultNode*		next;
};

struct FileResultNode {
	gchar*			owner_filename;
	ResultNode*		result_node_list;

	FileResultNode*	next;
};

struct SearchTools {
	Puss*				app;

	FileResultNode*		result_list;
	FileResultNode*		result_list_last;

	GtkEntry*			search_entry;
	GtkTreeView*		result_view;
	GtkListStore*		result_store;

	GtkToggleButton*	search_option_in_current_file;
	GtkToggleButton*	search_option_in_opened_files;
	GtkToggleButton*	search_option_in_current_file_dir;
};

void clear_search_results(SearchTools* self) {
	gtk_list_store_clear(self->result_store);

	while( self->result_list ) {
		FileResultNode* rs = self->result_list;
		self->result_list = rs->next;

		while( rs->result_node_list ) {
			ResultNode* p = rs->result_node_list;
			rs->result_node_list = p->next;

			g_free(p->preview);
			g_free(p);
		}

		g_free(rs->owner_filename);
		g_free(rs);
	}

	self->result_list = 0;
	self->result_list_last = 0;
}

void fill_search_results_list(SearchTools* self) {
	GtkTreeIter iter;
	for(FileResultNode* rs = self->result_list; rs; rs = rs->next) {
		for(ResultNode* p = rs->result_node_list; p; p = p->next) {
			gtk_list_store_append(self->result_store, &iter);
			gtk_list_store_set( self->result_store, &iter
				, 0, p->filename
				, 1, p->line
				, 2, p->preview
				, 3, p
				, -1 );
		}
	}
}

void search_in_file_content( const gchar* filename
							, gchar* content
							, gsize len
							, const gchar* search_text
							, SearchTools* self )
{
	g_assert( content );

	ResultNode* last_node = 0;

	gchar* end = content + len;
	gint line = -1;
	gchar* ps = content;
	gchar* pe = content;
	while( ps < end ) {
		for( pe=ps; pe < end; ++pe )
			if( *pe=='\r' || *pe=='\n' )
				break;
		++line;

		gchar* pos = g_strstr_len(ps, (gsize)(pe-ps), search_text);
		if( pos ) {
			ResultNode* node = g_new0(ResultNode, 1);
			if( !node )
				break;

			if( last_node ) {
				last_node->next = node;

			} else {
				FileResultNode* fnode = g_new0(FileResultNode, 1);
				if( !fnode ) {
					g_free(node);
					break;
				}

				fnode->owner_filename = g_strdup(filename);

				if( self->result_list ) {
					g_assert( self->result_list_last );
					self->result_list_last->next = fnode;
				} else {
					self->result_list = fnode;
				}

				fnode->result_node_list = node;
				self->result_list_last = fnode;
			}
			last_node = node;

			node->filename = self->result_list_last->owner_filename;
			node->line = line;
			node->preview = g_strndup(ps, (gsize)(pe-ps));
		}

		if( (pe < end) && *pe=='\r' )
			++pe;

		if( (pe < end) && *pe=='\n' )
			++pe;

		ps = pe;
	}
}

void search_in_doc(GtkTextBuffer* buf, const gchar* search_text, SearchTools* self) {
	GString* url = self->app->doc_get_url(buf);
	if( !url )
		return;

	GtkTextIter ps, pe;
	gtk_text_buffer_get_start_iter(buf, &ps);
	gtk_text_buffer_get_end_iter(buf, &pe);
	gchar* content = gtk_text_buffer_get_text(buf, &ps, &pe, TRUE);
	gsize len = g_utf8_strlen(content, -1);
	if( content ) {
		search_in_file_content( url->str
			, content
			, len
			, search_text
			, self );
		g_free(content);
	}
}

void search_in_file(const gchar* filename, const gchar* search_text, SearchTools* self) {
	gchar* content = 0;
	gsize len = 0;
	if( !self->app->load_file(filename, &content, &len, 0) )
		return;

	search_in_file_content(filename, content, len, search_text, self);

	g_free(content);
}

void search_in_dir(const gchar* dirname, const gchar* search_text, SearchTools* self) {
	GDir* dir = g_dir_open(dirname, 0, 0);
	if( !dir )
		return;

	for(;;) {
		const gchar* fname = g_dir_read_name(dir);
		if( !fname )
			break;

		gchar* filename = g_build_filename(dirname, fname, NULL);
		if( g_file_test(filename, G_FILE_TEST_IS_DIR) ) {
			search_in_dir(filename, search_text, self);
		} else {
			search_in_file(filename, search_text, self);
		}
		g_free(filename);
	}
	g_dir_close(dir);
}

void search_in_current_file(const gchar* search_text, SearchTools* self) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	GtkTextBuffer* buf = self->app->doc_get_buffer_from_page_num(page_num);
	if( buf )
		search_in_doc(buf, search_text, self);
}

void search_in_opened_files(const gchar* search_text, SearchTools* self) {
	gint count = gtk_notebook_get_n_pages(puss_get_doc_panel(self->app));
	for( gint i=0; i < count; ++i ) {
		GtkTextBuffer* buf = self->app->doc_get_buffer_from_page_num(i);
		if( buf )
			search_in_doc(buf, search_text, self);
	}
}

void search_in_current_file_dir(const gchar* search_text, SearchTools* self) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	GtkTextBuffer* buf = self->app->doc_get_buffer_from_page_num(page_num);
	if( !buf )
		return;

	GString* url = self->app->doc_get_url(buf);
	if( !url )
		return;

	gchar* dirname = g_path_get_dirname(url->str);
	if( dirname ) {
		search_in_dir(dirname, search_text, self);
		g_free(dirname);
	}
}

SIGNAL_CALLBACK gboolean search_tools_search(GtkEntry* entry, GdkEventKey* event, SearchTools* self) {
	if( event->keyval==GDK_Return )
	{
		const gchar* text = gtk_entry_get_text(entry);

		gtk_tree_view_set_model(self->result_view, 0);
		clear_search_results(self);

		if( gtk_toggle_button_get_active(self->search_option_in_current_file) )
			search_in_current_file(text, self);

		else if( gtk_toggle_button_get_active(self->search_option_in_opened_files) )
			search_in_opened_files(text, self);

		else if( gtk_toggle_button_get_active(self->search_option_in_current_file_dir) )
			search_in_current_file_dir(text, self);

		fill_search_results_list(self);
		gtk_tree_view_set_model(self->result_view, GTK_TREE_MODEL(self->result_store));
		return TRUE;
	}
	return FALSE;
}

SIGNAL_CALLBACK void search_results_cb_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* col, SearchTools* self) {
	GtkTreeIter iter;
	if( !gtk_tree_model_get_iter(GTK_TREE_MODEL(self->result_store), &iter, path) )
		return;

	ResultNode* p = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(self->result_store), &iter, 3, &p, -1);
	if( !p )
		return;

	self->app->doc_open(p->filename, p->line, -1, FALSE);
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
	self->search_option_in_current_file = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "search_option_in_current_file"));
	self->search_option_in_opened_files = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "search_option_in_opened_files"));
	self->search_option_in_current_file_dir = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "search_option_in_current_file_dir"));
	g_assert( panel
		&& self->search_entry
		&& self->result_store
		&& self->result_view
		&& self->search_option_in_current_file
		&& self->search_option_in_opened_files
		&& self->search_option_in_current_file_dir );

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

		g_object_ref(self->result_store);
	}

	return self;
}

PUSS_EXPORT void  puss_extend_destroy(void* ext) {
	SearchTools* self = (SearchTools*)ext;
	clear_search_results(self);
	g_object_unref(self->result_store);
	g_free(self);
}

