// GlobalOptions.c

#include "GlobalOptions.h"

#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#include "Puss.h"
#include "OptionManager.h"
#include "DocManager.h"
#include "Utils.h"

static void parse_puss_theme_option(const Option* option, const gchar* old, gpointer tag) {
	gchar* str;
	if( !option->value || option->value[0]=='\0' )
		return;

	str = g_strdup_printf("gtk-theme-name = \"%s\"", option->value);
	gtk_rc_parse_string(str);
	g_free(str);
}

static void parse_puss_editor_style_option(const Option* option, const gchar* old, gpointer tag) {
	GtkSourceStyleSchemeManager* ssm;
	GtkSourceStyleScheme* style;
	GtkSourceBuffer* buf;
	gint num;
	gint i;

	if( !option->value || option->value[0]=='\0' )
		return;

	ssm = gtk_source_style_scheme_manager_get_default();
	style = gtk_source_style_scheme_manager_get_scheme(ssm, option->value);
	if( style ) {
		num = gtk_notebook_get_n_pages(puss_app->doc_panel);
		for( i=0; i<num; ++i ) {
			buf = GTK_SOURCE_BUFFER(puss_doc_get_buffer_from_page_num(i));
			if( buf )
				gtk_source_buffer_set_style_scheme(buf, style);
		}
	}
}

static void parse_source_editor_font_option(const Option* option, const gchar* old, gpointer tag) {
	gint i;
	gint num;
	GtkTextView* view;
	PangoFontDescription* desc = pango_font_description_from_string(option->value);
	if( desc ) {
		num = gtk_notebook_get_n_pages(puss_app->doc_panel);
		for( i=0; i<num; ++i ) {
			view = puss_doc_get_view_from_page_num(i);
			if( view )
				gtk_widget_modify_font(GTK_WIDGET(view), desc);
		}

		pango_font_description_free(desc);
	}
}

static void append_global_paths() {
	// append style path PUSS/styles
	{
		GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
		gchar* style_path = g_build_filename(puss_app->module_path, "styles", NULL);
		gtk_source_style_scheme_manager_append_search_path(ssm, style_path);
		g_free(style_path);
	}
}

void puss_reg_global_options() {
	const Option* option;

#ifdef G_OS_WIN32
	option = puss_option_manager_option_reg("puss", "theme", "MS-Windows");
#else
	option = puss_option_manager_option_reg("puss", "theme", 0);
#endif

	puss_option_manager_monitor_reg(option, &parse_puss_theme_option, 0, 0);
	parse_puss_theme_option(option, 0, 0);

	option = puss_option_manager_option_reg("puss", "editor.style", "puss");
	puss_option_manager_monitor_reg(option, &parse_puss_editor_style_option, 0, 0);

	option = puss_option_manager_option_reg("puss", "editor.font", "");
	puss_option_manager_monitor_reg(option, &parse_source_editor_font_option, 0, 0);

	append_global_paths();
}

static void cb_combo_box_option_changed(GtkComboBox* w, const Option* option) {
	gchar* text = gtk_combo_box_get_active_text(w);
	puss_option_manager_option_set(option, text);
	g_free(text);
}

static void cb_font_button_changed(GtkFontButton* w, const Option* option) {
	const gchar* text = gtk_font_button_get_font_name(w);
	puss_option_manager_option_set(option, text);
}

static void cb_apply_button_changed(GtkButton* w, GtkEntry* entry) {
	const Option* option = puss_option_manager_option_find("puss", "fileloader.charset_list");
	const gchar* text = gtk_entry_get_text(entry);
	puss_option_manager_option_set(option, text);
}

GtkWidget* puss_create_global_options_setup_widget(gpointer tag) {
	gchar* filepath;
	GtkBuilder* builder;
	GtkWidget* panel;
	GtkWidget* w;
	GError* err = 0;
	const Option* option;

	// create UI
	builder = gtk_builder_new();
	if( !builder )
		return 0;
	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);

	filepath = g_build_filename(puss_app->module_path, "res", "puss_setup_widget.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(puss) : build setup dialog filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return 0;
	}

	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(puss): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return 0;
	}

	panel = GTK_WIDGET(g_object_ref(gtk_builder_get_object(builder, "main_panel")));


	{
		gchar* path;
		GDir*  dir;
		const gchar* fname;
		gchar* rcfile;
		gint i;
		gint index = -1;

		option = puss_option_manager_option_find("puss", "theme");
		w = GTK_WIDGET(gtk_builder_get_object(builder, "theme_combo"));

		path = gtk_rc_get_theme_dir();
		dir = g_dir_open(path, 0, 0);
		if( dir ) {
			i = 0;
			for(;;) {
				fname = g_dir_read_name(dir);
				if( !fname )
					break;

				rcfile = g_build_filename(path, fname, "gtk-2.0", "gtkrc", NULL);
				if( g_file_test(rcfile, G_FILE_TEST_EXISTS) ) {
					gtk_combo_box_append_text(GTK_COMBO_BOX(w), fname);

					if( index < 0 && option->value ) {
						if( g_str_equal(fname, option->value) )
							index = i;
						++i;
					}
				}
				g_free(rcfile);
			}
			g_dir_close(dir);
		}
		g_free(path);

		if( index >= 0 )
			gtk_combo_box_set_active(GTK_COMBO_BOX(w), index);

		g_signal_connect(w, "changed", G_CALLBACK(cb_combo_box_option_changed), (gpointer)option);
	}

	{
		const gchar* const * ids;
		const gchar* const * p;
		gint i;
		gint index = -1;

		GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();

		option = puss_option_manager_option_find("puss", "editor.style");
		w = GTK_WIDGET(gtk_builder_get_object(builder, "style_combo"));

		if( ssm ) {
			gtk_source_style_scheme_manager_force_rescan(ssm);

			ids = gtk_source_style_scheme_manager_get_scheme_ids(ssm);
			i = 0;
			for( p=ids; *p; ++p ) {
				gtk_combo_box_append_text(GTK_COMBO_BOX(w), *p);

				if( index < 0 && option->value ) {
					if( g_str_equal(*p, option->value) )
						index = i;
					++i;
				}
			}
			
			if( index >= 0 )
				gtk_combo_box_set_active(GTK_COMBO_BOX(w), index);
		}

		g_signal_connect(w, "changed", G_CALLBACK(cb_combo_box_option_changed), (gpointer)option);
	}

	{
		option = puss_option_manager_option_find("puss", "editor.font");
		w = GTK_WIDGET(gtk_builder_get_object(builder, "font_button"));

		if( option->value && option->value[0] )
			gtk_font_button_set_font_name(GTK_FONT_BUTTON(w), option->value);

		g_signal_connect(w, "font-set", G_CALLBACK(cb_font_button_changed), (gpointer)option);
	}

	{
		GtkEntry* entry;
		option = puss_option_manager_option_find("puss", "fileloader.charset_list");
		entry = GTK_ENTRY(gtk_builder_get_object(builder, "charset_entry"));
		w = GTK_WIDGET(gtk_builder_get_object(builder, "charset_apply_button"));

		gtk_entry_set_text(entry, option->value);
		g_signal_connect(w, "clicked", G_CALLBACK(cb_apply_button_changed), (gpointer)entry);
	}

	g_object_unref(G_OBJECT(builder));

	return panel;
}

