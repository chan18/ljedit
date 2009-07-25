// module.c
// 

#include "IPuss.h"
#include "vdriver.h"

#include <libintl.h>

#define TEXT_DOMAIN "gdbmi"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct {
	Puss*			app;
	MIVDriver*		drv;

	GtkActionGroup*	action_group;
} GDBMIPlugin;

static void gdbmi_menu_setup( GtkAction* action, GDBMIPlugin* self ) {
}

static void gdbmi_menu_run( GtkAction* action, GDBMIPlugin* self ) {
	switch( mi_vdriver_target_status(self->drv) ) {
	case MI_TARGET_ST_NONE:
	case MI_TARGET_ST_EXIT:
	case MI_TARGET_ST_ERROR:
		mi_vdriver_command_run(self->drv);
		break;
	case MI_TARGET_ST_DONE:
	case MI_TARGET_ST_STOPPED:
		mi_vdriver_command_continue(self->drv);
		break;
	case MI_TARGET_ST_RUNNING:
		mi_vdriver_command_pause(self->drv);
		break;
	case MI_TARGET_ST_CONNECTED:
		break;
	}
}

static void gdbmi_menu_stop( GtkAction* action, GDBMIPlugin* self ) {
	mi_vdriver_command_stop(self->drv);
}

static GtkActionEntry gdbmi_actions[] = {
	  { "gdbmi_setup", 0, "setup", 0, 0, (GCallback)gdbmi_menu_setup }
	, { "gdbmi_run", 0, "run/pause", "F5", 0, (GCallback)gdbmi_menu_run }
	, { "gdbmi_stop", 0, "stop", 0, 0, (GCallback)gdbmi_menu_stop }
};

void ui_create(GDBMIPlugin* self) {
	gchar* filepath;
	GtkBuilder* builder;
	GError* err = 0;

	filepath = g_build_filename(self->app->get_plugins_path(), "gdbmi.ui", NULL);
	if( !filepath ) {
		g_printerr("ERROR(gdbmi) : build ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return;
	}

	self->action_group = gtk_action_group_new("gdbmi_action_group");
	gtk_action_group_add_actions(self->action_group, gdbmi_actions, sizeof(gdbmi_actions)/sizeof(GtkActionEntry), self);
	gtk_ui_manager_insert_action_group(puss_get_ui_manager(self->app), self->action_group, 0);

	gtk_ui_manager_add_ui_from_file(puss_get_ui_manager(self->app), filepath, 0);
	//self->toolbar = GTK_TOOLBAR(gtk_builder_get_object(builder, "gdbmi_toolbar"));
	//self->app->panel_append(self->toolbar, gtk_label_new("debug"), "gdbmi", PUSS_PANEL_POS_BOTTOM);
}

void ui_destroy(GDBMIPlugin* self) {
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	GDBMIPlugin* self;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(GDBMIPlugin, 1);
	self->app = app;
	self->drv = mi_vdriver_new();

	ui_create(self);
	
	return self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	GDBMIPlugin* self = (GDBMIPlugin*)ext;
	if( !self )
		return;

	if( self->drv )
		mi_vdriver_free(self->drv);

	ui_destroy(self);

	g_free(self);
}

