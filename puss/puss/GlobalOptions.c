// GlobalOptions.c

#include "GlobalOptions.h"

#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#include "Puss.h"
#include "OptionManager.h"
#include "DocManager.h"
#include "Utils.h"

void parse_puss_theme_option(const Option* option, const gchar* old, gpointer tag) {
	gchar* str;
	if( !option->value || option->value[0]=='\0' )
		return;

	str = g_strdup_printf("gtk-theme-name = \"%s\"", option->value);
	gtk_rc_parse_string(str);
	g_free(str);
}

void reg_gtk_theme_option() {
	gchar* path;
	GDir*  dir;
	gchar* tag;
	const gchar* fname;
	gchar* rcfile;
	gchar* tmp;
	const Option* option;

	path = gtk_rc_get_theme_dir();
	dir = g_dir_open(path, 0, 0);
	tag = g_strdup("enum:");
	if( dir ) {
		for(;;) {
			fname = g_dir_read_name(dir);
			if( !fname )
				break;

			rcfile = g_build_filename(path, fname, "gtk-2.0", "gtkrc", NULL);
			if( g_file_test(rcfile, G_FILE_TEST_EXISTS) ) {
				tmp = g_strconcat(tag, fname, " ", NULL);
				g_free(tag);
				tag = tmp;
			}
			g_free(rcfile);
		}
		g_dir_close(dir);
	}
	g_free(path);

#ifdef G_OS_WIN32
	option = puss_option_manager_option_reg("puss", "theme", "MS-Windows");
#else
	option = puss_option_manager_option_reg("puss", "theme", 0);
#endif

	puss_option_manager_monitor_reg(option, &parse_puss_theme_option, 0, 0);
	parse_puss_theme_option(option, 0, 0);
}

void parse_puss_editor_style_option(const Option* option, const gchar* old, gpointer tag) {
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

void reg_source_editor_style_option() {
	//gchar* style_path;
	//gchar* styles;
	//gchar* tag;
	//const gchar* const * ids;
	const Option* option;

	GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
	if( ssm ) {
		/*
		style_path = g_build_filename(puss_app->module_path, "styles", NULL);
		gtk_source_style_scheme_manager_append_search_path(ssm, style_path);
		g_free(style_path);

		ids = gtk_source_style_scheme_manager_get_scheme_ids(ssm);
		styles = g_strjoinv(" ", (gchar**)ids);
		tag = g_strdup_printf("enum:%s", styles);
		g_free(styles);
		*/

		option = puss_option_manager_option_reg("puss", "editor.style", "puss");
		puss_option_manager_monitor_reg(option, &parse_puss_editor_style_option, 0, 0);
	}
}

void parse_source_editor_font_option(const Option* option, const gchar* old, gpointer tag) {
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

void reg_source_editor_font_option() {
	const Option* option = puss_option_manager_option_reg("puss", "editor.font", "");
	puss_option_manager_monitor_reg(option, &parse_source_editor_font_option, 0, 0);
}

void puss_reg_global_options() {
	reg_gtk_theme_option();
	reg_source_editor_style_option();
	reg_source_editor_font_option();
}

