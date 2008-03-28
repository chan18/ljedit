// module.cpp
//

#include "IPuss.h"

#include <glib/gi18n.h>


typedef struct 
{
	GtkAction action;
} ToolMenuAction;

typedef struct 
{
	GtkActionClass parent_class;
} ToolMenuActionClass;

G_DEFINE_TYPE(ToolMenuAction, tool_menu_action, GTK_TYPE_ACTION)

static void tool_menu_action_class_init(ToolMenuActionClass *klass)
{
	GTK_ACTION_CLASS(klass)->toolbar_item_type = GTK_TYPE_MENU_TOOL_BUTTON;
}

static void tool_menu_action_init(ToolMenuAction *action)
{
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	GtkAction* language_selector_action = GTK_ACTION(
		g_object_new( tool_menu_action_get_type()
			, "name", "view_language_selector"
			, "label", _("Select Source Language")
			, "stock-id", GTK_STOCK_SELECT_COLOR
			, NULL ) );

	GtkActionGroup* action_group = gtk_action_group_new("PussLanguageSelector");
	gtk_action_group_add_action(action_group, language_selector_action);

	GtkUIManager* ui_mgr = GTK_UI_MANAGER(gtk_builder_get_object(app->get_ui_builder(), "main_ui_manager"));
	gtk_ui_manager_insert_action_group(ui_mgr, action_group, -1);
	guint merge_id = gtk_ui_manager_new_merge_id(ui_mgr);
	gtk_ui_manager_add_ui(ui_mgr, merge_id, "/main_toolbar/view_place", "view_language_selector", "view_language_selector", GTK_UI_MANAGER_AUTO, TRUE);

	g_object_unref(action_group);
	//g_print("* PluginEngine extends create\n");

	// init plugin engine

	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	//g_print("* PluginEngine extends destroy\n");

	// !!! do not use gtk widgets here
	// !!! gtk_main already exit
	// !!! use gtk_quit_add() register destroy callback

	// free resource
}

