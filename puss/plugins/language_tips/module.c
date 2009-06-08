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
} DevUI;

typedef struct {
	Puss* app;

	CppParser		cpp_parser;

	GAsyncQueue*	parse_queue;
	GThread*		parse_thread;

	DevUI			ui;
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

static void create_ui(LanguageTips* self) {
	gchar* filepath;
	GtkBuilder* builder;
	GError* err = 0;
	DevUI* ui = &(self->ui);

	builder = gtk_builder_new();
	if( !builder )
		return;

	ui->builder = builder;

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

	ui->outline_panel = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
	ui->outline_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "outline_treeview"));
	ui->outline_store = GTK_TREE_STORE(g_object_ref(gtk_builder_get_object(builder, "outline_store")));
	g_assert( ui->outline_panel && ui->outline_view && ui->outline_store );

	gtk_widget_show_all(ui->outline_panel);
	self->app->panel_append(ui->outline_panel, gtk_label_new(_("Outline")), "dev_outline", PUSS_PANEL_POS_RIGHT);

	ui->preview_panel = GTK_WIDGET(gtk_builder_get_object(builder, "preview_panel"));
	ui->preview_filename_label = GTK_LABEL(gtk_builder_get_object(builder, "filename_label"));
	ui->preview_number_button = GTK_BUTTON(gtk_builder_get_object(builder, "number_button"));
	ui->preview_view           = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "preview_view"));
	g_assert( ui->preview_panel && ui->preview_filename_label && ui->preview_number_button && ui->preview_view );

	gtk_widget_show_all(ui->preview_panel);
	self->app->panel_append(ui->preview_panel, gtk_label_new(_("Preview")), "dev_preview", PUSS_PANEL_POS_BOTTOM);

	ui->tips_include_window = GTK_WIDGET(gtk_builder_get_object(builder, "include_window"));
	ui->tips_include_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "include_view"));
	ui->tips_include_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "include_store"));
	g_assert( ui->tips_include_window && ui->tips_include_view && ui->tips_include_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "include_panel")));

	ui->tips_list_window = GTK_WIDGET(gtk_builder_get_object(builder, "list_window"));
	ui->tips_list_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "list_view"));
	ui->tips_list_model = GTK_TREE_MODEL(gtk_builder_get_object(builder, "list_store"));
	g_assert( ui->tips_list_window && ui->tips_list_view && ui->tips_list_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "list_panel")));

	ui->tips_decl_window = GTK_WIDGET(gtk_builder_get_object(builder, "decl_window"));
	ui->tips_decl_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "decl_view"));
	ui->tips_decl_buffer = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(ui->tips_decl_view));
	g_assert( ui->tips_decl_window && ui->tips_decl_view && ui->tips_decl_buffer );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "decl_panel")));

	gtk_builder_connect_signals(builder, self);
}

static void destroy_ui(LanguageTips* self) {
	DevUI* ui = &(self->ui);

	if( !ui->builder )
		return;

	self->app->panel_remove(ui->outline_panel);
	self->app->panel_remove(ui->preview_panel);

	g_object_unref(G_OBJECT(ui->builder));
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	g_self = g_new0(LanguageTips, 1);
	g_self->app = app;

	cpp_parser_init( &(g_self->cpp_parser), TRUE );

	g_self->parse_queue = g_async_queue_new_full(g_free);
	g_self->parse_thread = g_thread_create(tips_parse_thread, g_self->parse_queue, TRUE, 0);
	if( g_self->parse_thread )
		g_async_queue_ref(g_self->parse_queue);

	create_ui(g_self);

	return g_self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	if( !g_self )
		return;

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


