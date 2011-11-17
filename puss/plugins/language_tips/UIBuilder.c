// UIBuilder.c
// 

#include "LanguageTips.h"

#include <assert.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

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
		filepath = g_build_filename(plugin_path, "language_tips", name, NULL);
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

static void parse_editor_font_option(const Option* option, const gchar* old, LanguageTips* self) {
	PangoFontDescription* desc = pango_font_description_from_string(option->value);
	if( desc ) {
		gtk_widget_modify_font(GTK_WIDGET(self->preview_view), desc);
		gtk_widget_modify_font(GTK_WIDGET(self->tips_decl_view), desc);

		pango_font_description_free(desc);
	}
}

static void parse_editor_style_option(const Option* option, const gchar* old, LanguageTips* self) {
	GtkSourceStyleSchemeManager* ssm;
	GtkSourceStyleScheme* style;
	GtkSourceBuffer* buf;

	if( !option->value || option->value[0]=='\0' )
		return;

	ssm = gtk_source_style_scheme_manager_get_default();
	style = gtk_source_style_scheme_manager_get_scheme(ssm, option->value);
	if( style ) {
		buf = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(self->preview_view));
		if( buf )
			gtk_source_buffer_set_style_scheme(buf, style);
	}
}

static void parse_include_path_option(const Option* option, const gchar* old, LanguageTips* self) {
	cpp_guide_include_paths_set(self->cpp_guide, option->value);
}

static const gchar* TARGET_OPTION_KEY = "target_option";
static const gchar* TEXT_VIEW_KEY = "text_view";
static const gchar* FILE_BUTTON_KEY = "file_button";
static const gchar* BUILDER_KEY = "builder";

static void cb_add_button_changed(GtkButton* w, LanguageTips* self) {
	GtkTextIter iter;
	gchar* uri;
	gchar* path;
	// const Option* option;
	GtkTextView* view;
	GtkFileChooserButton* btn;
	GtkTextBuffer* buf;

	// option = (const Option*)g_object_get_data(G_OBJECT(w), TARGET_OPTION_KEY);
	view = (GtkTextView*)g_object_get_data(G_OBJECT(w), TEXT_VIEW_KEY);
	btn = (GtkFileChooserButton*)g_object_get_data(G_OBJECT(w), FILE_BUTTON_KEY);

	uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(btn));
	path = g_filename_from_uri(uri, NULL, NULL);
	g_free(uri);

	buf = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_end_iter(buf, &iter);
	gtk_text_buffer_place_cursor(buf, &iter);
	if( gtk_text_iter_backward_char(&iter) && gtk_text_iter_get_char(&iter)!='\n' )
		gtk_text_buffer_insert_at_cursor(buf, "\n", -1);
	gtk_text_buffer_insert_at_cursor(buf, path, -1);

	g_free(path);
}

static void cb_apply_button_changed(GtkButton* w, LanguageTips* self) {
	GtkTextIter ps, pe;
	gchar* text;
	const Option* option;
	GtkTextView* view;
	GtkTextBuffer* buf;

	option = (const Option*)g_object_get_data(G_OBJECT(w), TARGET_OPTION_KEY);
	view = (GtkTextView*)g_object_get_data(G_OBJECT(w), TEXT_VIEW_KEY);
	buf = gtk_text_view_get_buffer(view);

	gtk_text_buffer_get_start_iter(buf, &ps);
	gtk_text_buffer_get_end_iter(buf, &pe);
	text = gtk_text_buffer_get_text(buf, &ps, &pe, TRUE);

	self->app->option_set(option, text);
	g_free(text);
}

static void cb_normal_apply_button_changed(GtkButton* w, LanguageTips* self) {
	GtkBuilder* builder;
	const Option* option;
	GtkEntry* entry;

	builder = (GtkBuilder*)g_object_get_data(G_OBJECT(w), BUILDER_KEY);

	option = self->app->option_find("language_tips", "hint_max_num");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "hint_max_num_entry"));
	self->app->option_set(option, gtk_entry_get_text(entry));

	option = self->app->option_find("language_tips", "hint_max_time");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "hint_max_time_entry"));
	self->app->option_set(option, gtk_entry_get_text(entry));

	option = self->app->option_find("language_tips", "jump_max_num");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "jump_max_num_entry"));
	self->app->option_set(option, gtk_entry_get_text(entry));

	option = self->app->option_find("language_tips", "jump_max_time");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "jump_max_time_entry"));
	self->app->option_set(option, gtk_entry_get_text(entry));
}

static GtkWidget* create_setup_normal_ui(LanguageTips* self) {
	const Option* option;
	GtkBuilder* builder;
	GtkWidget* panel;
	GError* err = 0;
	GtkEntry* entry;
	GtkButton* button;
	gchar* setup_filename;

	setup_filename = g_build_filename(self->app->get_plugins_path(), "language_tips", "setup_normal.ui", NULL);
	if( !setup_filename )
		return 0;

	// create UI
	builder = gtk_builder_new();
	if( !builder )
		return 0;
	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	gtk_builder_add_from_file(builder, setup_filename, &err);
	if( err ) {
		g_printerr("ERROR(language_tips): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return 0;
	}

	panel = GTK_WIDGET(g_object_ref(gtk_builder_get_object(builder, "main_panel")));

	option = self->app->option_find("language_tips", "hint_max_num");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "hint_max_num_entry"));
	gtk_entry_set_text(entry, option->value);

	option = self->app->option_find("language_tips", "hint_max_time");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "hint_max_time_entry"));
	gtk_entry_set_text(entry, option->value);

	option = self->app->option_find("language_tips", "jump_max_num");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "jump_max_num_entry"));
	gtk_entry_set_text(entry, option->value);

	option = self->app->option_find("language_tips", "jump_max_time");
	entry = GTK_ENTRY(gtk_builder_get_object(builder, "jump_max_time_entry"));
	gtk_entry_set_text(entry, option->value);

	button = GTK_BUTTON(gtk_builder_get_object(builder, "apply_button"));
	g_object_set_data_full(G_OBJECT(button), BUILDER_KEY, g_object_ref(builder), g_object_unref);
	g_signal_connect(button, "clicked", G_CALLBACK(cb_normal_apply_button_changed), self);

	g_object_unref(G_OBJECT(builder));

	return panel;
}

static GtkWidget* create_setup_path_ui(LanguageTips* self) {
	GtkBuilder* builder;
	GtkWidget* panel;
	GtkWidget* w;
	GtkTextView* view;
	GtkTextBuffer* buf;
	GError* err = 0;
	const Option* option;
	gchar* setup_filename;

	setup_filename = g_build_filename(self->app->get_plugins_path(), "language_tips", "setup_path.ui", NULL);
	if( !setup_filename )
		return 0;

	// create UI
	builder = gtk_builder_new();
	if( !builder )
		return 0;
	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	gtk_builder_add_from_file(builder, setup_filename, &err);
	if( err ) {
		g_printerr("ERROR(language_tips): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return 0;
	}

	panel = GTK_WIDGET(g_object_ref(gtk_builder_get_object(builder, "main_panel")));

	{
		option = self->app->option_find("language_tips_cpp", "include_path");
		view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "path_text_view"));

		w = GTK_WIDGET(gtk_builder_get_object(builder, "add_button"));
		g_object_set_data(G_OBJECT(w), TARGET_OPTION_KEY, (gpointer)option);
		g_object_set_data(G_OBJECT(w), TEXT_VIEW_KEY, view);
		g_object_set_data(G_OBJECT(w), FILE_BUTTON_KEY, gtk_builder_get_object(builder, "path_choose_button"));
		g_signal_connect(w, "clicked", G_CALLBACK(cb_add_button_changed), self);

		w = GTK_WIDGET(gtk_builder_get_object(builder, "apply_button"));
		g_object_set_data(G_OBJECT(w), TARGET_OPTION_KEY, (gpointer)option);
		g_object_set_data(G_OBJECT(w), TEXT_VIEW_KEY, view);
		g_signal_connect(w, "clicked", G_CALLBACK(cb_apply_button_changed), self);

		buf = gtk_text_view_get_buffer(view);
		gtk_text_buffer_set_text(buf, option->value, -1);
	}

	g_object_unref(G_OBJECT(builder));

	return panel;
}

static void option_monitor_init(LanguageTips* self) {
	const Option* option;

	option = self->app->option_find("puss", "editor.font");
	if( option ) {
		parse_editor_font_option(option, 0, self);
		self->option_font_change_handler = self->app->option_monitor_reg(option, (OptionChanged)parse_editor_font_option, self, 0);
	}

	option = self->app->option_find("puss", "editor.style");
	if( option ) {
		parse_editor_style_option(option, 0, self);
		self->option_style_change_handler = self->app->option_monitor_reg(option, (OptionChanged)parse_editor_style_option, self, 0);
	}

	option = self->app->option_reg("language_tips_cpp", "include_path", "/usr/include\n/usr/include/c++/4.0\n$pkg-config --cflags gtk+-2.0\n");
	self->option_path_change_handler = self->app->option_monitor_reg(option, (OptionChanged)parse_include_path_option, self, 0);
	parse_include_path_option(option, 0, self);

	self->app->option_setup_reg("language_tips", _("language tips"), (CreateSetupWidget)create_setup_path_ui, self, 0);
	self->app->option_setup_reg("language_tips._", _("tips limit"), (CreateSetupWidget)create_setup_normal_ui, self, 0);
	self->app->option_setup_reg("language_tips.path", _("c++ paths"), (CreateSetupWidget)create_setup_path_ui, self, 0);
}

static void option_monitor_final(LanguageTips* self) {
	self->app->option_monitor_unreg(self->option_font_change_handler);
	self->app->option_monitor_unreg(self->option_style_change_handler);
	self->app->option_monitor_unreg(self->option_path_change_handler);

	self->app->option_setup_unreg("language_tips.path");
	self->app->option_setup_unreg("language_tips._");
	self->app->option_setup_unreg("language_tips");
}

void ui_create(LanguageTips* self) {
	gchar* filepath;
	const gchar* plugins_path;
	GtkBuilder* builder;
	GtkContainer* container;
	GError* err = 0;
	gint i;

	builder = gtk_builder_new();
	if( !builder )
		return;

	self->builder = builder;

	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	filepath = g_build_filename(self->app->get_plugins_path(), "language_tips", "main.ui", NULL);
	if( !filepath ) {
		g_printerr("ERROR(language_tips) : build ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(language_tips): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return;
	}

	self->outline_panel = GTK_WIDGET(gtk_builder_get_object(builder, "outline_panel"));
	self->outline_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "outline_treeview"));
	self->outline_store = GTK_TREE_STORE(g_object_ref(gtk_builder_get_object(builder, "outline_store")));
	assert( self->outline_panel && self->outline_view && self->outline_store );

	gtk_widget_show_all(self->outline_panel);
	self->app->panel_append(self->outline_panel, gtk_label_new(_("Outline")), "dev_outline", PUSS_PANEL_POS_RIGHT);

	self->preview_panel = GTK_WIDGET(gtk_builder_get_object(builder, "preview_panel"));
	self->preview_filename_label = GTK_LABEL(gtk_builder_get_object(builder, "filename_label"));
	self->preview_number_button = GTK_BUTTON(gtk_builder_get_object(builder, "number_button"));
	container = GTK_CONTAINER(gtk_builder_get_object(builder, "preview_container"));
	self->preview_view = GTK_TEXT_VIEW(gtk_source_view_new());
	assert( self->preview_panel && self->preview_filename_label && self->preview_number_button && container && self->preview_view );
	g_object_set(self->preview_view, "editable", FALSE, "tab-width", 4, "show-line-numbers", TRUE, "highlight-current-line", TRUE, NULL);
	gtk_container_add(container, GTK_WIDGET(self->preview_view));
	set_cpp_lang_to_source_view(self->preview_view);
	gtk_widget_show_all(self->preview_panel);
	self->app->panel_append(self->preview_panel, gtk_label_new(_("Preview")), "dev_preview", PUSS_PANEL_POS_BOTTOM);

	self->tips_include_window = GTK_WIDGET(gtk_builder_get_object(builder, "include_window"));
	self->tips_include_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "include_view"));
	self->tips_include_model = GTK_TREE_MODEL(g_object_ref(gtk_builder_get_object(builder, "include_store")));
	assert( self->tips_include_window && self->tips_include_view && self->tips_include_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "include_panel")));

	self->tips_list_window = GTK_WIDGET(gtk_builder_get_object(builder, "list_window"));
	self->tips_list_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "list_view"));
	self->tips_list_model = GTK_TREE_MODEL(g_object_ref(gtk_builder_get_object(builder, "list_store")));
	assert( self->tips_list_window && self->tips_list_view && self->tips_list_model );
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "list_panel")));

	self->tips_decl_window = GTK_WIDGET(gtk_builder_get_object(builder, "decl_window"));
	container = GTK_CONTAINER(gtk_builder_get_object(builder, "decl_scrolled_window"));
	self->tips_decl_view =  GTK_TEXT_VIEW(gtk_source_view_new());
	set_cpp_lang_to_source_view(self->tips_decl_view);
	self->tips_decl_buffer = GTK_TEXT_BUFFER(gtk_text_view_get_buffer(self->tips_decl_view));
	assert( self->tips_decl_window && self->tips_decl_view && container && self->tips_decl_buffer );
	g_object_set(self->tips_decl_view, "editable", FALSE, "tab-width", 4, NULL);
	gtk_container_add(container, GTK_WIDGET(self->tips_decl_view));
	gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, "decl_panel")));

	gtk_builder_connect_signals(builder, self);

	// icons
	plugins_path = self->app->get_plugins_path();
	self->icons = g_new0(GdkPixbuf*, CPP_ET__LAST);
	if( self->icons ) {
		for( i=CPP_ET__FIRST; i<CPP_ET__LAST; ++i )
			self->icons[i] = tips_icon_load(plugins_path, ICONS_FILES[i]);
	}

	option_monitor_init(self);
}

void ui_destroy(LanguageTips* self) {
	gint i;

	if( !self->builder )
		return;

	option_monitor_final(self);

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

