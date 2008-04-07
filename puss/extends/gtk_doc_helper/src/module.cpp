// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>

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

	gchar*		browser;

	GList*		modules;	// DocModuleNode list
};

#ifdef G_OS_WIN32
	#include <windows.h>

	/*
	gchar* auto_search_browser() {
		HKEY hkRoot;
		HKEY hSubKey;
		CHAR ValueName[256];
		DWORD dwType;
		DWORD cbValueName = 256;
		DWORD cbDataValue = 256;
		BYTE  DataValue[256];

		if( RegOpenKeyA(HKEY_CLASSES_ROOT, NULL, &hkRoot)==ERROR_SUCCESS ) {
			if( RegOpenKeyExA(hkRoot, "htmlfile\\shell\\open\\command", 0, KEY_ALL_ACCESS, &hSubKey)==ERROR_SUCCESS ) {
				RegEnumValueA(hSubKey, 0, ValueName, &cbValueName, NULL, &dwType, DataValue, &cbDataValue);
				RegCloseKey(hSubKey);
			}
			RegCloseKey(hkRoot);
		}

		return g_strdup((gchar*)DataValue);
	}
	*/

	void open_url(GtkDocHelper* self, const gchar* link) {
		//if( !self->browser || self->browser[0]=='\0' )
		//	self->browser = auto_search_browser();

		gchar* cmd = g_strjoin(" ", self->browser, link, NULL);
		WinExec(cmd, SW_SHOW);
		g_free(cmd);

		//ShellExecuteA(HWND_DESKTOP, "open", self->browser, link, NULL, SW_SHOWNORMAL);
	}

#else
	#include <stdlib.h>

	void open_url(GtkDocHelper* self, const gchar* link) {
		if( !self->browser || self->browser[0]=='\0' )
			self->browser = g_strdup("/usr/bin/firefox");

		gchar* cmd = g_strjoin(" ", self->browser, link, NULL);
		system(cmd);
		g_free(cmd);
	}

#endif

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
	if( !self->modules ) {
		GtkWidget* dlg = gtk_message_dialog_new( puss_get_main_window(self->app)
			, GTK_DIALOG_MODAL
			, GTK_MESSAGE_INFO
			, GTK_BUTTONS_OK
			, _("Not find gtk-doc html files.") );
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), _("please setup\n   option : (gtk_doc_helper.gtk_doc_paths)\n   in [menu]->[tools]->[options]\n  and retry!"));
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		return;
	}

	_FindTag tag = { self, key, 0, 0 };
	g_list_foreach(self->modules, (GFunc)&__find_and_open_in_brower, &tag);

	if( tag.res_path ) {
		gchar* url = g_build_filename("file:///", tag.res_path, tag.res_pos, NULL);
		open_url(self, url);
		g_free(url);

	} else {
		GtkWidget* dlg = gtk_message_dialog_new( puss_get_main_window(self->app)
			, GTK_DIALOG_MODAL
			, GTK_MESSAGE_INFO
			, GTK_BUTTONS_OK
			, _("Not find symbol(%s) in gtk-doc.")
			, key );
		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
	}
}

void parse_web_browser_option(const Option* option, GtkDocHelper* self) {
	g_free(self->browser);
	self->browser = g_strdup(option->value);
}

void parse_gtk_doc_path_option(const Option* option, GtkDocHelper* self) {
	gchar** items = g_strsplit_set(option->value, ",; \t\r\n", 0);
	for( gchar** p=items; *p; ++p ) {
		if( *p[0]=='\0' )
			continue;

		parse_doc_module(self, *p, "ix01.html");
	}
	g_strfreev(items);
}

void search_gtk_doc_symbol_active(GtkAction* action, GtkDocHelper* self) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	GtkTextBuffer* buf = self->app->doc_get_buffer_from_page_num(page_num);
	if( !buf )
		return;

	// get current word
	GtkTextIter ps;
	gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
	if( gtk_text_iter_is_end(&ps) )
		return;

	gunichar ch = gtk_text_iter_get_char(&ps);
	if( !g_unichar_isalnum(ch) && ch!='_' )
		return;

	GtkTextIter pe = ps;

	// find key start position
	while( gtk_text_iter_backward_char(&ps) ) {
		gunichar ch = gtk_text_iter_get_char(&ps);
		if( !g_unichar_isalnum(ch) && ch!='_' ) {
			gtk_text_iter_forward_char(&ps);
			break;
		}
	}

	// find key end position
	while( gtk_text_iter_forward_char(&pe) ) {
		gunichar ch = gtk_text_iter_get_char(&pe);
		if( !g_unichar_isalnum(ch) && ch!='_' )
			break;
    }

	if( gtk_text_iter_equal(&ps, &pe) )
		return;

	gchar* keyword = gtk_text_iter_get_text(&ps, &pe);
	if( keyword ) {
		find_and_open_in_brower(self, keyword);
		g_free(keyword);
	}
}

void create_ui(GtkDocHelper* self) {
	GtkAction* search_gtk_doc_symbol_action = gtk_action_new("search_gtk_doc_symbol_action", _("search gtk symbol"), _("search current symbol in gtk-doc html files and show gtk-doc helper."), GTK_STOCK_FIND);
	g_signal_connect(search_gtk_doc_symbol_action, "activate", G_CALLBACK(&search_gtk_doc_symbol_active), self);

	GtkActionGroup* main_action_group = GTK_ACTION_GROUP(gtk_builder_get_object(self->app->get_ui_builder(), "main_action_group"));
	gtk_action_group_add_action_with_accel(main_action_group, search_gtk_doc_symbol_action, "F1");

	const gchar* ui_info =
		"<ui>\n"
		"  <menubar name='main_menubar'>\n"
		"     <menu action='tool_menu'>\n"
		"      <placeholder name='tool_menu_extend_place'>\n"
		"        <menuitem action='search_gtk_doc_symbol_action'/>\n"
		"      </placeholder>\n"
		"    </menu>\n"
		"  </menubar>\n"
		"\n"
		"  <toolbar name='main_toolbar'>\n"
		"    <placeholder name='main_toolbar_tool_place'>\n"
		"      <toolitem action='search_gtk_doc_symbol_action'/>\n"
		"    </placeholder>\n"
        "  </toolbar>\n"
		"</ui>\n"
		;

	GtkUIManager* ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(self->app->get_ui_builder(), "main_ui_manager"));

	GError* err = 0;
	gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, &err);

	if( err ) {
		g_printerr("ERROR(gtk_doc_helper) : %s", err->message);
		g_error_free(err);
	}

	gtk_ui_manager_ensure_update(ui_mgr);
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	GtkDocHelper* self = g_new0(GtkDocHelper, 1);

	self->app = app;
	self->re_dt = g_regex_new("<dt>(.*), <a class=\"indexterm\" href=\"(.*)\">", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

	{
		const Option* option = app->option_manager_option_reg( "gtk_doc_helper"
			, "web_browser"
#ifdef G_OS_WIN32
			, "C:\\Program Files\\Mozilla Firefox\\firefox.exe"
#else
			, "/usr/bin/firefox"
#endif
			, 0
			, 0
			, 0 );

		app->option_manager_monitor_reg(option, (OptionChanged)&parse_web_browser_option, self);
		parse_web_browser_option(option, self);
	}

	{
		const Option* option = app->option_manager_option_reg( "gtk_doc_helper"
			, "gtk_doc_paths"
#ifdef G_OS_WIN32
			, "c:/gtk/share/gtk-doc/html/glib\n"
			  "c:/gtk/share/gtk-doc/html/gobject\n"
			  "c:/gtk/share/gtk-doc/html/gdk\n"
			  "c:/gtk/share/gtk-doc/html/gtk\n"
#else
			, "/usr/share/gtk-doc/html/glib\n"
			  "/usr/share/gtk-doc/html/gobject\n"
			  "/usr/share/gtk-doc/html/gdk\n"
			  "/usr/share/gtk-doc/html/gtk\n"
#endif
			, 0
			, (gpointer)"text"
			, 0 );

		app->option_manager_monitor_reg(option, (OptionChanged)&parse_gtk_doc_path_option, self);
		parse_gtk_doc_path_option(option, self);
	}

	// UI
	create_ui(self);

	return self;
}

PUSS_EXPORT void  puss_extend_destroy(void* ext) {
	if( ext ) {
		GtkDocHelper* self = (GtkDocHelper*)ext;
		g_regex_unref(self->re_dt);
		g_free(self->browser);

		g_list_foreach(self->modules, (GFunc)&doc_module_node_free, 0);
		g_list_free(self->modules);

		g_free(self);
	}
}

