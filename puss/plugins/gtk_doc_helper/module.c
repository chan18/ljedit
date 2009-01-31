// module.cpp
//

#include "IPuss.h"

#include <libintl.h>

#define TEXT_DOMAIN "plugin_gtk_doc_helper"

#define _(str) dgettext(TEXT_DOMAIN, str)


typedef struct {
	gchar*		path;
	GHashTable*	index;
} DocModuleNode;

static void doc_module_node_free(DocModuleNode* node, gpointer nouse) {
	g_free(node->path);
	g_hash_table_destroy(node->index);
}

typedef struct {
	Puss*		app;
	GRegex*		re_dt;

	gchar*		browser;

	guint		merge_id;
	GtkAction*	search_gtk_doc_symbol_action;
	gulong		search_gtk_doc_symbol_handler;
	gpointer	option_monitor_web_browser;
	gpointer	option_monitor_gtk_doc_path;

	GList*		modules;	// DocModuleNode list
} GtkDocHelper;

#ifdef G_OS_WIN32
	#include <windows.h>

	/*
	static gchar* auto_search_browser() {
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

	static void open_url(GtkDocHelper* self, const gchar* link) {
		//if( !self->browser || self->browser[0]=='\0' )
		//	self->browser = auto_search_browser();

		gchar* cmd = g_strjoin(" ", self->browser, link, NULL);
		WinExec(cmd, SW_SHOW);
		g_free(cmd);

		//ShellExecuteA(HWND_DESKTOP, "open", self->browser, link, NULL, SW_SHOWNORMAL);
	}

#else
	#include <stdlib.h>

	static void open_url(GtkDocHelper* self, const gchar* link) {
		if( !self->browser || self->browser[0]=='\0' )
			self->browser = g_strdup("/usr/bin/firefox");

		gchar* cmd = g_strdup_printf("%s %s &", self->browser, link);
		//gchar* cmd = g_strjoin(" ", self->browser, link, NULL);
		system(cmd);
		g_free(cmd);
	}

#endif

static void parse_doc_module(GtkDocHelper* self, const gchar* gtk_doc_path, const char* path, const gchar* index_html_file) {
	gchar* text = 0;
	gsize len = 0;
	gchar* index_filename = g_build_filename(gtk_doc_path, path, index_html_file, NULL);
	if( !index_filename )
		return;

	if( self->app->load_file(index_filename, &text, &len, 0) ) {
		GMatchInfo* info = 0;
		if( g_regex_match_full(self->re_dt, text, len, 0, (GRegexMatchFlags)0, &info, 0) ) {
			DocModuleNode* node = g_new0(DocModuleNode, 1);
			node->path = g_strdup(gtk_doc_path);
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

static void parse_gtk_doc_module(GtkDocHelper* self, const gchar* gtk_doc_path) {
	const gchar* filename;
	GDir* dir = g_dir_open(gtk_doc_path, 0, NULL);
	if( dir ) {
		for(;;) {
			filename = g_dir_read_name(dir);
			if( !filename )
				break;

			parse_doc_module(self, gtk_doc_path, filename, "index.sgml");
		}

		g_dir_close(dir);
	}

}

typedef struct {
	GtkDocHelper*	self;
	const gchar*	key;

	const gchar*	res_path;
	const gchar*	res_pos;
} _FindTag;

static void __find_and_open_in_brower(DocModuleNode* node, _FindTag* tag) {
	if( !tag->res_path ) {
		gchar* value = (gchar*)g_hash_table_lookup(node->index, tag->key);
		if( value ) {
			tag->res_path = node->path;
			tag->res_pos = value;
		}
	}
}

static void find_and_open_in_brower(GtkDocHelper* self, const gchar* key) {
	_FindTag tag = {0, 0, 0, 0};
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

	tag.self = self;
	tag.key = key;

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

static void parse_web_browser_option(const Option* option, const gchar* old, GtkDocHelper* self) {
	g_free(self->browser);
	self->browser = g_strdup(option->value);
}

static void parse_gtk_doc_path_option(const Option* option, const gchar* old, GtkDocHelper* self) {
	gchar** p;
	gchar** items = g_strsplit_set(option->value, ",; \t\r\n", 0);
	for( p=items; *p; ++p ) {
		if( *p[0]=='\0' )
			continue;

		parse_gtk_doc_module(self, *p);
	}
	g_strfreev(items);
}

static void search_gtk_doc_symbol_active(GtkAction* action, GtkDocHelper* self) {
	gunichar ch;
	gchar* keyword;
	GtkTextIter ps, pe;
	GtkTextBuffer* buf;
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(self->app));
	if( page_num < 0 )
		return;

	buf = self->app->doc_get_buffer_from_page_num(page_num);
	if( !buf )
		return;

	// get current word
	gtk_text_buffer_get_iter_at_mark(buf, &ps, gtk_text_buffer_get_insert(buf));
	if( gtk_text_iter_is_end(&ps) )
		return;

	ch = gtk_text_iter_get_char(&ps);
	if( !g_unichar_isalnum(ch) && ch!='_' && ch!='-' )
		return;

	pe = ps;

	// find key start position
	while( gtk_text_iter_backward_char(&ps) ) {
		ch = gtk_text_iter_get_char(&ps);
		if( !g_unichar_isalnum(ch) && ch!='_' && ch!='-' ) {
			gtk_text_iter_forward_char(&ps);
			break;
		}
	}

	// find key end position
	while( gtk_text_iter_forward_char(&pe) ) {
		ch = gtk_text_iter_get_char(&pe);
		if( !g_unichar_isalnum(ch) && ch!='_' && ch!='-' )
			break;
    }

	if( gtk_text_iter_equal(&ps, &pe) )
		return;

	keyword = gtk_text_iter_get_text(&ps, &pe);
	if( keyword ) {
		find_and_open_in_brower(self, keyword);
		g_free(keyword);
	}
}

static const gchar* ui_info =
	"<ui>"
	"  <menubar name='main_menubar'>"
	"     <menu action='tool_menu'>"
	"      <placeholder name='tool_menu_plugin_place'>"
	"        <menuitem action='search_gtk_doc_symbol_action'/>"
	"      </placeholder>"
	"    </menu>"
	"  </menubar>"
	""
	"  <toolbar name='main_toolbar'>"
	"    <placeholder name='main_toolbar_tool_place'>"
	"      <toolitem action='search_gtk_doc_symbol_action'/>"
	"    </placeholder>"
    "  </toolbar>"
	"</ui>"
	;

static void create_ui(GtkDocHelper* self) {
	GtkActionGroup* main_action_group;
	GtkUIManager* ui_mgr;
	GError* err;

	self->search_gtk_doc_symbol_action = gtk_action_new("search_gtk_doc_symbol_action", _("search gtk symbol"), _("search current symbol in gtk-doc html files and show gtk-doc helper."), GTK_STOCK_FIND);
	self->search_gtk_doc_symbol_handler = g_signal_connect(self->search_gtk_doc_symbol_action, "activate", G_CALLBACK(&search_gtk_doc_symbol_active), self);

	main_action_group = GTK_ACTION_GROUP(gtk_builder_get_object(self->app->get_ui_builder(), "main_action_group"));
	gtk_action_group_add_action_with_accel(main_action_group, self->search_gtk_doc_symbol_action, "F1");

	ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(self->app->get_ui_builder(), "main_ui_manager"));

	err = 0;
	self->merge_id = gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, &err);

	if( err ) {
		g_printerr("ERROR(gtk_doc_helper) : %s", err->message);
		g_error_free(err);
	}

	gtk_ui_manager_ensure_update(ui_mgr);
}

static void destroy_ui(GtkDocHelper* self) {
	GtkActionGroup* main_action_group;
	GtkUIManager* ui_mgr;

	g_signal_handler_disconnect(self->search_gtk_doc_symbol_action, self->search_gtk_doc_symbol_handler);

	main_action_group = GTK_ACTION_GROUP(gtk_builder_get_object(self->app->get_ui_builder(), "main_action_group"));
	gtk_action_group_remove_action(main_action_group, self->search_gtk_doc_symbol_action);

	ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(self->app->get_ui_builder(), "main_ui_manager"));

	gtk_ui_manager_remove_ui(ui_mgr, self->merge_id);

	gtk_ui_manager_ensure_update(ui_mgr);
}

static const gchar* setup_ui_info =
	"<interface>"
	"  <object class='GtkTable' id='main_panel'>"
	"	<property name='n_rows'>4</property>"
	"	<property name='n_columns'>2</property>"
	"	<property name='column_spacing'>5</property>"
	"	<property name='row_spacing'>20</property>"
	"	<child>"
	"	  <object class='GtkFileChooserButton' id='web_browser_file_button'/>"
	"	  <packing>"
	"		<property name='left_attach'>1</property>"
	"		<property name='right_attach'>2</property>"
	"		<property name='top_attach'>0</property>"
	"		<property name='bottom_attach'>1</property>"
	"		<property name='y_options'>GTK_FILL</property>"
	"	  </packing>"
	"	</child>"
	"	<child>"
	"	  <object class='GtkLabel' id='web_browser_label'>"
	"		<property name='label' translatable='yes'>web browser</property>"
	"	  </object>"
	"	  <packing>"
	"		<property name='top_attach'>0</property>"
	"		<property name='bottom_attach'>1</property>"
	"		<property name='y_options'>GTK_FILL</property>"
	"	  </packing>"
	"	</child>"
	"	<child>"
	"	  <object class='GtkFileChooserButton' id='gtk_doc_path_button'>"
	"       <property name='action'>GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER</property>"
	"     </object>"
	"	  <packing>"
	"		<property name='left_attach'>1</property>"
	"		<property name='right_attach'>2</property>"
	"		<property name='top_attach'>1</property>"
	"		<property name='bottom_attach'>2</property>"
	"		<property name='y_options'>GTK_FILL</property>"
	"	  </packing>"
	"	</child>"
	"	<child>"
	"	  <object class='GtkLabel' id='gtk_doc_path_label'>"
	"		<property name='label' translatable='yes'>gtk-doc path</property>"
	"	  </object>"
	"	  <packing>"
	"		<property name='top_attach'>1</property>"
	"		<property name='bottom_attach'>2</property>"
	"		<property name='y_options'>GTK_FILL</property>"
	"	  </packing>"
	"	</child>"
	"	<child>"
	"	  <object class='GtkLabel' id='gtk_doc_path_label'>"
	"		<property name='label' translatable='yes'>e.g. (/usr/share/gtk-doc/html)</property>"
	"	  </object>"
	"	  <packing>"
	"		<property name='left_attach'>1</property>"
	"		<property name='right_attach'>2</property>"
	"		<property name='top_attach'>2</property>"
	"		<property name='bottom_attach'>3</property>"
	"		<property name='y_options'>GTK_FILL</property>"
	"	  </packing>"
	"	</child>"
	"	<child>"
	"	  <placeholder/>"
	"	</child>"
	"	<child>"
	"	  <placeholder/>"
	"	</child>"
	"  </object>"
	"</interface>"
	;

static const gchar* TARGET_OPTION_KEY = "target_option";

static void cb_file_set(GtkFileChooserButton *widget, GtkDocHelper* self) {
	const Option* option = g_object_get_data(G_OBJECT(widget), TARGET_OPTION_KEY);
	gchar* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(widget));
	gchar* filename = g_filename_from_uri(uri, NULL, NULL);
	self->app->option_set(option, filename);
	g_free(filename);
	g_free(uri);
}

static GtkWidget* create_setup_ui(GtkDocHelper* self) {
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

	gtk_builder_add_from_string(builder, setup_ui_info, -1, &err);
	if( err ) {
		g_printerr("ERROR(gtk_doc_helper): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return 0;
	}

	panel = GTK_WIDGET(g_object_ref(gtk_builder_get_object(builder, "main_panel")));

	{
		option = self->app->option_find("gtk_doc_helper", "web_browser");
		w = GTK_WIDGET(gtk_builder_get_object(builder, "web_browser_file_button"));

		gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(w), option->value);

		g_object_set_data(G_OBJECT(w), TARGET_OPTION_KEY, (gpointer)option);
		g_signal_connect(w, "file-set", G_CALLBACK(cb_file_set), self);
	}

	{
		option = self->app->option_find("gtk_doc_helper", "gtk_doc_path");
		w = GTK_WIDGET(gtk_builder_get_object(builder, "gtk_doc_path_button"));

		gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(w), option->value);

		g_object_set_data(G_OBJECT(w), TARGET_OPTION_KEY, (gpointer)option);
		g_signal_connect(w, "file-set", G_CALLBACK(cb_file_set), self);
	}

	g_object_unref(G_OBJECT(builder));

	return panel;
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	GtkDocHelper* self;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(GtkDocHelper, 1);

	self->app = app;
	self->re_dt = g_regex_new("<ANCHOR id=\"(.*)\" href=\"(.*)\">", (GRegexCompileFlags)0, (GRegexMatchFlags)0, 0);

	{
		const Option* option = app->option_reg( "gtk_doc_helper"
			, "web_browser"
#ifdef G_OS_WIN32
			, "C:\\Program Files\\Mozilla Firefox\\firefox.exe"
#else
			, "/usr/bin/firefox"
#endif
			);

		self->option_monitor_web_browser = app->option_monitor_reg(option, &parse_web_browser_option, self, 0);
		parse_web_browser_option(option, 0, self);
	}

	{
		const Option* option = app->option_reg( "gtk_doc_helper"
			, "gtk_doc_path"
#ifdef G_OS_WIN32
			, "c:/gtk/share/gtk-doc/html"
#else
			, "/usr/share/gtk-doc/html"
#endif
			);

		self->option_monitor_gtk_doc_path = app->option_monitor_reg(option, &parse_gtk_doc_path_option, self, 0);
		parse_gtk_doc_path_option(option, 0, self);
	}

	// UI
	create_ui(self);

	app->option_setup_reg("gtk_doc_helper", _("gtk-doc helper"), create_setup_ui, self, 0);

	return self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* ext) {
	if( ext ) {
		GtkDocHelper* self = (GtkDocHelper*)ext;
		destroy_ui(self);

		self->app->option_setup_unreg("gtk_doc_helper");

		self->app->option_monitor_unreg(self->option_monitor_web_browser);
		self->app->option_monitor_unreg(self->option_monitor_gtk_doc_path);

		g_regex_unref(self->re_dt);
		g_free(self->browser);

		g_list_foreach(self->modules, (GFunc)&doc_module_node_free, 0);
		g_list_free(self->modules);

		g_free(self);
	}
}

