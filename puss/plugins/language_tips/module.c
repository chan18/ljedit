// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libintl.h>

#include "IPuss.h"

#include "cpp/parser.h"

#define TEXT_DOMAIN "language_tips"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct {
	Puss* app;

	CppParser		cpp_parser;

	GAsyncQueue*	parse_queue;
	GThread*		parse_thread;

	GtkBuilder*		builder;

	// outline window
	GtkWidget*		outline_panel;
	GtkTreeView*	outline_view;
	GtkTreeStore*	outline_store;
	CppFile*		outline_file;
	gint			outline_pos;

	// preview window
	GtkWidget*		preview_panel;
	GtkLabel*		preview_filename_label;
	GtkButton*		preview_number_button;
	GtkTextView*	preview_view;

	// tips window
	GtkWidget*		tips_include_window;
	GtkTreeView*	tips_include_view;
	GtkTreeModel*	tips_include_model;
	//StringSet*		tips_include_files;

	GtkWidget*		tips_list_window;
	GtkTreeView*	tips_list_view;
	GtkTreeModel*	tips_list_model;

	GtkWidget*		tips_decl_window;
	GtkTextView*	tips_decl_view;
	GtkTextBuffer*	tips_decl_buffer;

	// update timer
	guint			update_timer;

} LanguageTips;

static LanguageTips* g_self = 0;

static gchar* PARSE_THREAD_EXIT_SIGN = "";

static gpointer tips_parse_thread(gpointer args) {
	gchar* filename = 0;
	GAsyncQueue* queue = (GAsyncQueue*)args;
	if( !queue )
		return 0;

	while( (filename = (gchar*)g_async_queue_pop(queue)) != PARSE_THREAD_EXIT_SIGN ) {
		// parse file
		cpp_parser_parse(&(g_self->cpp_parser), filename, -1);

		g_free(filename);
	}

	g_async_queue_unref(queue);

	return 0;
}

typedef struct {
	GtkTreeStore*	store;
	GtkTreeIter*	iter;
} AddItemTag;

static void outline_add_elem(CppElem* elem, AddItemTag* parent) {
	GtkTreeIter iter;

	if( elem->type==CPP_ET_INCLUDE || elem->type==CPP_ET_UNDEF )
		return;

	gtk_tree_store_append(parent->store, &iter, parent->iter);
	gtk_tree_store_set( parent->store, &iter
		//, 0, icons_->get_icon_from_elem(*elem)
		, 1, elem->name->buf
		, 2, elem
		, -1 );

	if( cpp_elem_has_subscope(elem) ) {
		AddItemTag tag = {parent->store, &iter};
		g_list_foreach( cpp_elem_get_subscope(elem), (GFunc)outline_add_elem, &tag );
	}
}

static void outline_set_file(LanguageTips* self, CppFile* file, gint line) {
	if( file != self->outline_file ) {
		gtk_tree_view_set_model(self->outline_view, 0);
		gtk_tree_store_clear(self->outline_store);

		if( self->outline_file ) {
			cpp_file_unref(self->outline_file);
			self->outline_file = 0;
		}

		if( file ) {
			AddItemTag tag = {self->outline_store, 0};

			self->outline_file = cpp_file_ref(file);
			g_list_foreach( file->root_scope.v_ncscope.scope, (GFunc)outline_add_elem, &tag );
		}

		gtk_tree_view_set_model(self->outline_view, GTK_TREE_MODEL(self->outline_store));
	}

	if( file && self->outline_pos != line ) {
		self->outline_pos = line;
		gtk_tree_selection_unselect_all( gtk_tree_view_get_selection(self->outline_view) );

		// locate_line( (size_t)line + 1, 0 );
	}
}

static void create_ui(LanguageTips* self) {
	gchar* filepath;
	GtkBuilder* builder;
	GError* err = 0;

	builder = gtk_builder_new();
	if( !builder )
		return;

	self->builder = builder;

	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	filepath = g_build_filename(self->app->get_plugins_path(), "language_tips.ui", NULL);
	if( !filepath ) {
		g_printerr("ERROR(search_tools) : build ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(search_tools): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	self->outline_panel = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
	self->outline_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "outline_treeview"));
	self->outline_store = GTK_TREE_STORE(g_object_ref(gtk_builder_get_object(builder, "outline_store")));
	g_assert( self->outline_panel && self->outline_view && self->outline_store );

	gtk_widget_show_all(self->outline_panel);
	self->app->panel_append(self->outline_panel, gtk_label_new(_("Outline")), "dev_outline", PUSS_PANEL_POS_RIGHT);

	self->preview_panel = GTK_WIDGET(gtk_builder_get_object(builder, "preview_panel"));
	self->preview_filename_label = GTK_LABEL(gtk_builder_get_object(builder, "filename_label"));
	self->preview_number_button = GTK_BUTTON(gtk_builder_get_object(builder, "number_button"));
	self->preview_view           = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "preview_view"));
	g_assert( self->preview_panel && self->preview_filename_label && self->preview_number_button && self->preview_view );

	gtk_widget_show_all(self->preview_panel);
	self->app->panel_append(self->preview_panel, gtk_label_new(_("Preview")), "dev_preview", PUSS_PANEL_POS_BOTTOM);

	self->tips_include_window = GTK_WIDGET(gtk_builder_get_object(builder, "include_window"));
	self->tips_include_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "include_view"));
	self->tips_include_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "include_store"));
	g_assert( self->tips_include_window && self->tips_include_view && self->tips_include_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "include_panel")));

	self->tips_list_window = GTK_WIDGET(gtk_builder_get_object(builder, "list_window"));
	self->tips_list_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "list_view"));
	self->tips_list_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "list_store"));
	g_assert( self->tips_list_window && self->tips_list_view && self->tips_list_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "list_panel")));

	self->tips_decl_window = GTK_WIDGET(gtk_builder_get_object(builder, "decl_window"));
	self->tips_decl_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "decl_view"));
	self->tips_decl_buffer = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(self->tips_decl_view));
	g_assert( self->tips_decl_window && self->tips_decl_view && self->tips_decl_buffer );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "decl_panel")));

	gtk_builder_connect_signals(builder, self);
}

static void destroy_ui(LanguageTips* self) {
	if( !self->builder )
		return;

	self->app->panel_remove(self->outline_panel);
	self->app->panel_remove(self->preview_panel);

	g_object_unref(G_OBJECT(self->builder));
}

static void outline_update(LanguageTips* self) {
	GtkNotebook* doc_panel;
	gint num;
	GtkTextBuffer* buf;
	GString* url;
	CppFile* file;
	GtkTextIter iter;

	doc_panel = puss_get_doc_panel(self->app);
	num = gtk_notebook_get_current_page(doc_panel);
	if( num < 0 )
		return;

	buf = self->app->doc_get_buffer_from_page_num(num);
	if( !buf )
		return;

	url = self->app->doc_get_url(buf);
	if( !url )
		return;

	file = (CppFile*)g_hash_table_lookup(self->cpp_parser.parsed_files, url->str);
	if( !file )
		return;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	num = gtk_text_iter_get_line(&iter);
	outline_set_file(self, file, num);

	cpp_file_unref(file);
}

static gboolean on_update_timeout(LanguageTips* self) {
	outline_update(self);

	return TRUE;
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	g_self = g_new0(LanguageTips, 1);
	g_self->app = app;

	cpp_parser_init( &(g_self->cpp_parser), TRUE );
	g_self->cpp_parser.load_file = app->load_file;

	g_self->parse_queue = g_async_queue_new_full(g_free);
	g_self->parse_thread = g_thread_create(tips_parse_thread, g_self->parse_queue, TRUE, 0);
	if( g_self->parse_thread )
		g_async_queue_ref(g_self->parse_queue);

	create_ui(g_self);

	g_self->update_timer = g_timeout_add(500, on_update_timeout, g_self);

	return g_self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	if( !g_self )
		return;

	g_source_remove(g_self->update_timer);

	destroy_ui(g_self);

	if( g_self->parse_queue ) {
		g_async_queue_push(g_self->parse_queue, PARSE_THREAD_EXIT_SIGN);
		g_async_queue_unref(g_self->parse_queue);
	}

	if( g_self->parse_thread ) {
		g_thread_join(g_self->parse_thread);
	}

	cpp_parser_final( &(g_self->cpp_parser) );

	g_free(g_self);
	g_self = 0;
}


