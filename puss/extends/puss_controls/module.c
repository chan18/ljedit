// module.c
//

#include "IPussControls.h"

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
}

extern IToolMenuAction tool_menu_action_interface;

PUSS_EXPORT gpointer puss_extend_query(void* self, const gchar* interface_name) {
	if( g_str_equal(interface_name, INTERFACE_TOOL_MENU_ACTION) )
		return &tool_menu_action_interface;

	return 0;
}

