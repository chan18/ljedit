// module.cpp
//

#include "IPuss.h"
#include "IPussControls.h"

#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcebuffer.h>

#include <libintl.h>

#define TEXT_DOMAIN "plugin_language_selector"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct {
	Puss* app;

	GTypeModule*	type_module;

	GtkActionGroup* action_group;
	guint			merge_id;

	GList*			favory_language_list;
	guint			favory_merge_id;
} LanguageSelector;

static LanguageSelector* g_self = 0;

static void add_fill_favory_language(GtkSourceLanguage* lang);

static void pls_lang_active(GtkAction* action, GtkSourceLanguage* lang) {
	gint page_num;
	GtkTextBuffer* buf;

	page_num = gtk_notebook_get_current_page(puss_get_doc_panel(g_self->app));
	if( page_num < 0 )
		return;

	buf = g_self->app->doc_get_buffer_from_page_num(page_num);
	if( !buf || !GTK_IS_SOURCE_BUFFER(buf) )
		return;

	gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buf), lang);

	add_fill_favory_language(lang);
}

static void fill_languages(gchar* lang, GString* gstr) {
	g_string_append_printf(gstr, "            <menuitem action='pls_lang_%s'/>\n", lang);
}

static gboolean fill_language_section(gchar* key, GList* value, GString* gstr) {
	g_string_append_printf(gstr, "          <menu action='pls_sec_%s'>\n", key);
	g_list_foreach(value, (GFunc)&fill_languages, gstr);
	g_string_append_printf(gstr, "          </menu>\n");

	return FALSE;
}

static void fill_favory_language_menu() {
	GError* err = 0;
	GtkUIManager* ui_mgr;
	gchar* ui_info;
	GString* gstr;

	if( !g_self->favory_language_list )
		return;

	ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(g_self->app->get_ui_builder(), "main_ui_manager"));

	if( g_self->favory_merge_id ) {
		gtk_ui_manager_remove_ui(ui_mgr, g_self->favory_merge_id);
		gtk_ui_manager_ensure_update(ui_mgr);
	}

	// favory selector menu
	// 
	gstr = g_string_new(0);
	g_list_foreach(g_self->favory_language_list, (GFunc)&fill_languages, gstr);

	ui_info = g_strdup_printf(
		"<ui>"
		"  <menubar name='main_menubar'>"
		"     <menu action='view_menu'>\n"
		"      <placeholder name='view_menu_extend_place'>"
		"        <menu action='language_selector_open'>"
		"          <placeholder name='pls_favory_menu_place'>"
		"            <separator/>"
		"            %s"
		"          </placeholder>"
		"        </menu>"
		"      </placeholder>"
		"    </menu>"
		"  </menubar>"
		""
		"  <toolbar name='main_toolbar'>"
		"    <placeholder name='main_toolbar_view_place'>"
		"      <toolitem action='language_selector_toolmenu_open'>"
		"        <menu action='language_selector_toolmenu_open'>"
		"          <separator/>"
		"          <placeholder name='pls_favory_toolmenu_place'>"
		"            <separator/>"
		"            %s"
		"          </placeholder>"
		"        </menu>"
		"      </toolitem>"
		"    </placeholder>"
        "  </toolbar>"
		"</ui>"
		, gstr->str
		, gstr->str
	);

	g_self->favory_merge_id = gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, &err);
	if( err ) {
		g_printerr("%s", err->message);
		g_error_free(err);
	}
	g_free(ui_info);
	g_string_free(gstr, TRUE);

	gtk_ui_manager_ensure_update(ui_mgr);
}

static void add_fill_favory_language(GtkSourceLanguage* lang) {
	GList* list;
	const gchar* name = lang ? gtk_source_language_get_name(lang) : 0;
	if( !name )
		return;

	list = g_list_remove_all(g_self->favory_language_list, name);
	g_self->favory_language_list = g_list_prepend(list, (gchar*)name);

	for(;;) {
		list = g_list_nth(g_self->favory_language_list, 16);
		if( !list )
			break;
		g_self->favory_language_list = g_list_delete_link(g_self->favory_language_list, list);
	}

	fill_favory_language_menu();
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	GTree* sections;
	GtkSourceLanguageManager* lm;
	const gchar* const* ids;
	const gchar* const* id;
	GtkSourceLanguage* lang;
	const gchar* name;
	gchar* action_name;
	GtkAction* action;
	const gchar* section;
	GList* section_list;
	GtkUIManager* ui_mgr;
	IToolMenuAction* tool_menu_interface;

	GString* gstr;
	gchar* ui_info;
	GError* err;

	tool_menu_interface = app->extend_query("puss_controls", INTERFACE_TOOL_MENU_ACTION);
	if( !tool_menu_interface )
		return 0;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	g_self = g_new0(LanguageSelector, 1);
	g_self->app = app;
	g_self->action_group = gtk_action_group_new("puss_language_selector_action_group");

	// set select language menu
	sections = g_tree_new_full((GCompareDataFunc)&g_ascii_strcasecmp, 0, 0, (GDestroyNotify)g_list_free);
	lm = gtk_source_language_manager_get_default();
	ids = gtk_source_language_manager_get_language_ids(lm);
	for( id=ids; *id; ++id ) {
		lang = gtk_source_language_manager_get_language(lm, *id);
		if( lang ) {
			name = gtk_source_language_get_name(lang);
			action_name = g_strdup_printf("pls_lang_%s", name);
			action = gtk_action_new(action_name, name, NULL, NULL);
			g_signal_connect(action, "activate", G_CALLBACK(&pls_lang_active), lang);
			gtk_action_group_add_action(g_self->action_group, action);
			g_object_unref(action);
			g_free(action_name);

			section = gtk_source_language_get_section(lang);

			section_list = (GList*)g_tree_lookup(sections, section);
			if( section_list ) {
				g_tree_steal(sections, section);
			} else {
				action_name = g_strdup_printf("pls_sec_%s", section);
				action = GTK_ACTION(gtk_action_new(action_name, section, NULL, NULL));
				gtk_action_group_add_action(g_self->action_group, action);
				g_object_unref(action);
				g_free(action_name);
			}

			section_list = g_list_insert_sorted(section_list, (gchar*)name, (GCompareFunc)&g_ascii_strcasecmp);
			
			g_tree_insert(sections, (gchar*)section, section_list);
		}
	}

	// insert language selector menu-tool-button
	// 
	action = gtk_action_new("language_selector_open", _("Language"), _("select high-light source language, default use c++"), GTK_STOCK_SELECT_COLOR);
	gtk_action_group_add_action(g_self->action_group, action);
	g_object_unref(action);

	action = GTK_ACTION( g_object_new(tool_menu_interface->get_type ()
			, "name", "language_selector_toolmenu_open"
			, "label", _("Language")
			, "tooltip", _("select high-light source language, default use c++")
			, "stock-id", GTK_STOCK_SELECT_COLOR
			, NULL) );
	g_signal_connect(action, "activate", G_CALLBACK(&pls_lang_active), gtk_source_language_manager_get_language(lm, "cpp"));
	gtk_action_group_add_action(g_self->action_group, action);
	g_object_unref(action);

	ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(app->get_ui_builder(), "main_ui_manager"));
	gtk_ui_manager_insert_action_group(ui_mgr, g_self->action_group, 0);

	// main selector menu
	gstr= g_string_new(NULL);
	g_tree_foreach(sections, (GTraverseFunc)&fill_language_section, gstr);
	g_tree_destroy(sections);

	ui_info = g_strdup_printf(
		"<ui>"
		"  <menubar name='main_menubar'>"
		"     <menu action='view_menu'>\n"
		"      <placeholder name='view_menu_extend_place'>"
		"        <menu action='language_selector_open'>"
		"          %s"
		"          <placeholder name='pls_favory_menu_place'>"
		"          </placeholder>"
		"        </menu>"
		"      </placeholder>"
		"    </menu>"
		"  </menubar>"
		""
		"  <toolbar name='main_toolbar'>"
		"    <placeholder name='main_toolbar_view_place'>"
		"      <toolitem action='language_selector_toolmenu_open'>"
		"        <menu action='language_selector_toolmenu_open'>"
		"          %s"
		"          <placeholder name='pls_favory_toolmenu_place'>"
		"          </placeholder>"
		"        </menu>"
		"      </toolitem>"
		"    </placeholder>"
        "  </toolbar>"
		"</ui>"
		, gstr->str
		, gstr->str
	);

	//g_print(ui_info);
	err = 0;
	g_self->merge_id = gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, &err);
	if( err ) {
		g_printerr("%s", err->message);
		g_error_free(err);
	}
	g_free(ui_info);
	g_string_free(gstr, TRUE);

	gtk_ui_manager_ensure_update(ui_mgr);

	fill_favory_language_menu();

	return g_self;
}

PUSS_EXPORT void  puss_plugin_destroy(void* self) {
	GtkUIManager* ui_mgr;

	if( !g_self )
		return;

	ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(g_self->app->get_ui_builder(), "main_ui_manager"));
	gtk_ui_manager_remove_ui(ui_mgr, g_self->favory_merge_id);
	gtk_ui_manager_remove_ui(ui_mgr, g_self->merge_id);
	gtk_ui_manager_remove_action_group(ui_mgr, g_self->action_group);
	gtk_ui_manager_ensure_update(ui_mgr);

	g_object_unref(g_self->action_group);
	g_list_free(g_self->favory_language_list);

	g_free(g_self);
	g_self = 0;
}

