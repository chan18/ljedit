// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>

#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcebuffer.h>

//------------------------------------------------------------------------------
// Language Menu Tool Button
//  
typedef struct { GtkAction action; } ToolMenuAction;

typedef struct { GtkActionClass parent_class; } ToolMenuActionClass;

G_DEFINE_TYPE(ToolMenuAction, tool_menu_action, GTK_TYPE_ACTION)

static void tool_menu_action_class_init(ToolMenuActionClass *klass)
	{ GTK_ACTION_CLASS(klass)->toolbar_item_type = GTK_TYPE_MENU_TOOL_BUTTON; }

static void tool_menu_action_init(ToolMenuAction *action) {}
//------------------------------------------------------------------------------

static Puss* puss_app = 0;

void pls_lang_active(GtkAction* action, GtkSourceLanguage* lang) {
	gint page_num = gtk_notebook_get_current_page(puss_get_doc_panel(puss_app));
	if( page_num < 0 )
		return;

	GtkTextBuffer* buf = puss_app->doc_get_buffer_from_page_num(page_num);
	if( !buf || !GTK_IS_SOURCE_BUFFER(buf) )
		return;

	gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buf), lang);
}

gint compare_data_func_wrapper(gpointer a, gpointer b, GCompareFunc cmp) { return (*cmp)(a, b); }

void fill_languages(gchar* lang, GString* gstr) {
	g_string_append_printf(gstr, "            <menuitem action='pls_lang_%s'/>\n", lang);
}

gboolean fill_language_section(gchar* key, GList* value, GString* gstr) {
	g_string_append_printf(gstr, "          <menu action='pls_sec_%s'>\n", key);
	g_list_foreach(value, (GFunc)&fill_languages, gstr);
	g_string_append_printf(gstr, "          </menu>\n");

	return FALSE;
}

void destroy_language(gchar* lang, gpointer) {
	g_free(lang);
}

gboolean destroy_language_section(gchar* key, GList* value, gpointer) {
	g_list_foreach(value, (GFunc)&destroy_language, 0);
	g_list_free(value);
	return FALSE;
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	puss_app = app;

	GtkActionGroup* action_group = gtk_action_group_new("puss_language_selector_action_group");

	// set select language menu
	GTree* sections = g_tree_new((GCompareFunc)&g_ascii_strcasecmp);
	GtkSourceLanguageManager* lm = gtk_source_language_manager_get_default();
	const gchar* const* ids = gtk_source_language_manager_get_language_ids(lm);
	for( const gchar* const* id=ids; *id; ++id ) {
		GtkSourceLanguage* lang = gtk_source_language_manager_get_language(lm, *id);
		if( lang ) {
			const gchar* name = gtk_source_language_get_name(lang);
			gchar* action_name = g_strdup_printf("pls_lang_%s", name);
			GtkAction* action = gtk_action_new(action_name, name, NULL, NULL);
			g_signal_connect(action, "activate", G_CALLBACK(&pls_lang_active), lang);

			g_free(action_name);
			gtk_action_group_add_action(action_group, action);

			const gchar* section = gtk_source_language_get_section(lang);

			GList* section_list = (GList*)g_tree_lookup(sections, section);
			if( !section_list ) {
				action_name = g_strdup_printf("pls_sec_%s", section);
				action = GTK_ACTION(gtk_action_new(action_name, section, NULL, NULL));
				g_free(action_name);
				gtk_action_group_add_action(action_group, action);
			}

			section_list = g_list_insert_sorted(section_list, (gchar*)name, (GCompareFunc)&g_ascii_strcasecmp);
			
			g_tree_insert(sections, (gchar*)section, section_list);
		}
	}

	// insert language selector menu-tool-button
	// 
	GtkAction* language_selector_open_action =
		GTK_ACTION( g_object_new(tool_menu_action_get_type ()
			, "name", "language_selector_open"
			, "label", _("Language")
			, "tooltip", _("select c++ language high-light")
			, "stock-id", GTK_STOCK_SELECT_COLOR
			, NULL) );
	g_signal_connect(language_selector_open_action, "activate", G_CALLBACK(&pls_lang_active), gtk_source_language_manager_get_language(lm, "cpp"));

	gtk_action_group_add_action(action_group, language_selector_open_action);

	GtkUIManager* ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(app->get_ui_builder(), "main_ui_manager"));
	gtk_ui_manager_insert_action_group(ui_mgr, action_group, 0);

	GString* gstr= g_string_new(NULL);
	g_tree_foreach(sections, (GTraverseFunc)&fill_language_section, gstr);
	g_tree_foreach(sections, (GTraverseFunc)&destroy_language_section, 0);
	g_tree_destroy(sections);

	gchar* ui_info = g_strdup_printf(
		"<ui>\n"
		"  <menubar name='main_menubar'>\n"
		"     <menu action='view_menu'>\n"
		"      <placeholder name='view_menu_extend_place'>\n"
		"        <menu action='language_selector_open'>\n"
		"          %s\n"
		"        </menu>\n"
		"      </placeholder>\n"
		"    </menu>\n"
		"  </menubar>\n"
		"\n"
		"  <toolbar name='main_toolbar'>\n"
		"    <toolitem action='language_selector_open'>\n"
		"      <menu action='language_selector_open'>\n"
		"        %s\n"
		"      </menu>\n"
		"    </toolitem>\n"
        "  </toolbar>\n"
		"</ui>\n"
		, gstr->str
		, gstr->str
	);

	//g_print(ui_info);
	GError* err = 0;
	gtk_ui_manager_add_ui_from_string(ui_mgr, ui_info, -1, &err);

	if( err ) {
		g_printerr("%s", err->message);
		g_error_free(err);
	}

	g_free(ui_info);
	g_string_free(gstr, TRUE);

	gtk_ui_manager_ensure_update(ui_mgr);
	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	puss_app = 0;
}

