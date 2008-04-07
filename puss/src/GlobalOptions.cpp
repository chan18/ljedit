// GlobalOptions.cpp

#include "GlobalOptions.h"

#include <glib/gi18n.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#include "Puss.h"
#include "OptionManager.h"
#include "DocManager.h"
#include "Utils.h"

void parse_puss_theme_option(const Option* option, gpointer tag) {
	if( !option->value || option->value[0]=='\0' )
		return;

	gchar* str = g_strdup_printf("gtk-theme-name = \"%s\"", option->value);
	gtk_rc_parse_string(str);
	g_free(str);
}

void reg_gtk_theme_option() {
	gchar* path = gtk_rc_get_theme_dir();
	GDir* dir = g_dir_open(path, 0, 0);

	gchar* tag = g_strdup("enum:");
	if( dir ) {
		for(;;) {
			const gchar* fname = g_dir_read_name(dir);
			if( !fname )
				break;

			gchar* rcfile = g_build_filename(path, fname, "gtk-2.0", "gtkrc", NULL);
			if( g_file_test(rcfile, G_FILE_TEST_EXISTS) ) {
				gchar* tmp = g_strconcat(tag, fname, " ", NULL);
				g_free(tag);
				tag = tmp;
			}
			g_free(rcfile);
		}
		g_dir_close(dir);
	}
	g_free(path);

#ifdef G_OS_WIN32
	const Option* option = puss_option_manager_option_reg("puss", "theme", "MS-Windows", 0, tag, &g_free);
#else
	const Option* option = puss_option_manager_option_reg("puss", "theme", 0, 0, tag, &g_free);
#endif

	puss_option_manager_monitor_reg(option, &parse_puss_theme_option, 0, 0);
	parse_puss_theme_option(option, 0);
}

void parse_puss_editor_style_option(const Option* option, gpointer tag) {
	if( !option->value || option->value[0]=='\0' )
		return;

	GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
	GtkSourceStyleScheme* style = gtk_source_style_scheme_manager_get_scheme(ssm, option->value);
	if( style ) {
		gint num = gtk_notebook_get_n_pages(puss_app->doc_panel);
		for( gint i=0; i<num; ++i ) {
			GtkSourceBuffer* buf = GTK_SOURCE_BUFFER(puss_doc_get_buffer_from_page_num(i));
			if( buf )
				gtk_source_buffer_set_style_scheme(buf, style);
		}
	}
}

void reg_source_editor_style_option() {
	GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
	if( ssm ) {
		gchar* style_path = g_build_filename(puss_app->module_path, "styles", NULL);
		gtk_source_style_scheme_manager_append_search_path(ssm, style_path);
		g_free(style_path);

		const gchar* const * ids = gtk_source_style_scheme_manager_get_scheme_ids(ssm);
		gchar* styles = g_strjoinv(" ", (gchar**)ids);
		gchar* tag = g_strdup_printf("enum:%s", styles);
		g_free(styles);

		const Option* option = puss_option_manager_option_reg("puss", "editor.style", "puss", 0, tag, &g_free);
		puss_option_manager_monitor_reg(option, &parse_puss_editor_style_option, 0, 0);
	}
}

void parse_source_editor_font_option(const Option* option, gpointer tag) {
	PangoFontDescription* desc = pango_font_description_from_string(option->value);
	if( desc ) {
		gint num = gtk_notebook_get_n_pages(puss_app->doc_panel);
		for( gint i=0; i<num; ++i ) {
			GtkTextView* view = puss_doc_get_view_from_page_num(i);
			if( view )
				gtk_widget_modify_font(GTK_WIDGET(view), desc);
		}

		pango_font_description_free(desc);
	}
}

void reg_source_editor_font_option() {
	const Option* option = 0;

	option = puss_option_manager_option_reg("puss", "editor.font", "", 0, (gpointer)"font", 0);
	puss_option_manager_monitor_reg(option, &parse_source_editor_font_option, 0, 0);
}

void puss_reg_global_options() {
	reg_gtk_theme_option();
	reg_source_editor_style_option();
	reg_source_editor_font_option();
}

