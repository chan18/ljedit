// ToolMenuAction.c
//

#include "IPussControls.h"

typedef struct { GtkAction action; } ToolMenuAction;

typedef struct { GtkActionClass parent_class; } ToolMenuActionClass;

G_DEFINE_TYPE(ToolMenuAction, tool_menu_action, GTK_TYPE_ACTION)

static void tool_menu_action_class_init(ToolMenuActionClass* klass)
	{ GTK_ACTION_CLASS(klass)->toolbar_item_type = GTK_TYPE_MENU_TOOL_BUTTON; }

static void tool_menu_action_class_finalize(ToolMenuActionClass* klass)
	{}

static void tool_menu_action_init(ToolMenuAction *action) {}

IToolMenuAction tool_menu_action_interface = {
	tool_menu_action_get_type
};

