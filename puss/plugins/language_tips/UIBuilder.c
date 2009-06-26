// UIBuilder.c
// 

#include "LanguageTips.h"

#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

const gchar* ICONS_FILES[] = { 0
	, 0
	, "keyword.png"
	, 0
	, "macro.png"
	, 0
	, "var.png"
	, "fun.png"
	, "enumitem.png"
	, "enum.png"
	, "class.png"
	, 0
	, "namespace.png"
	, "typedef.png"
};

static GdkPixbuf* tips_icon_load(const gchar* plugin_path, const gchar* name) {
	gchar* filepath;
	GdkPixbuf* icon = 0;

	if( name ) {
		filepath = g_build_filename(plugin_path, "language_tips_res", name, NULL);
		icon = gdk_pixbuf_new_from_file(filepath, NULL);
		if( !icon )
			g_critical(_("* ERROR : language_tips load icon(%s) failed!\n"), filepath);
		g_free(filepath);
	}

	return icon;
}

static void tips_icon_free(GdkPixbuf* icon) {
	if( icon )
		g_object_unref(G_OBJECT(icon));
}

static GtkTextBuffer* set_cpp_lang_to_source_view(GtkTextView* source_view) {
	GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
	GtkSourceLanguage* lang = gtk_source_language_manager_get_language(lm, "cpp");

	GtkSourceBuffer* source_buffer = gtk_source_buffer_new_with_language(lang);
	GtkTextBuffer* retval = GTK_TEXT_BUFFER(source_buffer);
	gtk_text_view_set_buffer(source_view, retval);
	gtk_source_buffer_set_max_undo_levels(source_buffer, 0);
	g_object_unref(G_OBJECT(source_buffer));

	return retval;
}

void ui_create(LanguageTips* self) {
	gchar* filepath;
	const gchar* plugins_path;
	GtkBuilder* builder;
	GError* err = 0;
	gint i;

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
	self->preview_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "preview_view"));
	g_assert( self->preview_panel && self->preview_filename_label && self->preview_number_button && self->preview_view );
	set_cpp_lang_to_source_view(self->preview_view);
	gtk_widget_show_all(self->preview_panel);
	self->app->panel_append(self->preview_panel, gtk_label_new(_("Preview")), "dev_preview", PUSS_PANEL_POS_BOTTOM);

	self->tips_include_window = GTK_WIDGET(gtk_builder_get_object(builder, "include_window"));
	self->tips_include_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "include_view"));
	self->tips_include_model = GTK_TREE_MODEL(g_object_ref(gtk_builder_get_object(builder, "include_store")));
	g_assert( self->tips_include_window && self->tips_include_view && self->tips_include_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "include_panel")));

	self->tips_list_window = GTK_WIDGET(gtk_builder_get_object(builder, "list_window"));
	self->tips_list_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "list_view"));
	self->tips_list_model = GTK_TREE_MODEL(g_object_ref(gtk_builder_get_object(builder, "list_store")));
	g_assert( self->tips_list_window && self->tips_list_view && self->tips_list_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "list_panel")));

	self->tips_decl_window = GTK_WIDGET(gtk_builder_get_object(builder, "decl_window"));
	self->tips_decl_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "decl_view"));
	set_cpp_lang_to_source_view(self->tips_decl_view);
	self->tips_decl_buffer = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(self->tips_decl_view));
	g_assert( self->tips_decl_window && self->tips_decl_view && self->tips_decl_buffer );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "decl_panel")));

	gtk_builder_connect_signals(builder, self);

	// icons
	plugins_path = self->app->get_plugins_path();
	self->icons = g_new0(GdkPixbuf*, CPP_ET__LAST);
	if( self->icons ) {
		for( i=CPP_ET__FIRST; i<CPP_ET__LAST; ++i )
			self->icons[i] = tips_icon_load(plugins_path, ICONS_FILES[i]);
	}
}

void ui_destroy(LanguageTips* self) {
	gint i;

	if( !self->builder )
		return;

	self->app->panel_remove(self->outline_panel);
	self->app->panel_remove(self->preview_panel);

	g_object_unref(G_OBJECT(self->outline_store));
	g_object_unref(G_OBJECT(self->tips_include_model));
	g_object_unref(G_OBJECT(self->tips_list_model));
	g_object_unref(G_OBJECT(self->builder));

	if( self->icons ) {
		for( i=CPP_ET__FIRST; i<CPP_ET__LAST; ++i )
			tips_icon_free(self->icons[i]);
		g_free(self->icons);
	}
}

