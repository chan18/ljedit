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
	guint			ui_meger_id;

	MITargetSetup*	current_setup;
} GDBMIPlugin;

static void gdbmi_menu_setup( GtkAction* action, GDBMIPlugin* self ) {
	gchar* filepath;
	GtkBuilder* builder;
	GtkWidget* dlg;
	GError* err = 0;
	GtkEntry* gdb_entry;
	GtkEntry* exec_entry;
	GtkEntry* args_entry;
	GtkEntry* working_dir_entry;
	//GtkEntry* env_entry;
	gint res;

	filepath = g_build_filename(self->app->get_plugins_path(), "gdbmi_target_setup.ui", NULL);
	if( !filepath ) {
		g_printerr("ERROR(gdbmi) : build target setup ui filepath failed!\n");
		return;
	}

	builder = gtk_builder_new();
	gtk_builder_set_translation_domain(builder, TEXT_DOMAIN);
	gtk_builder_add_from_file(builder, filepath, &err);
	if( err ) {
		g_printerr("ERROR(gdbmi) : load target set ui error %s\n", err->message, 0);
		g_error_free(err);

	} else {
		dlg = GTK_WIDGET( gtk_builder_get_object(builder, "target_setup_dialog") );
		gdb_entry = GTK_ENTRY( gtk_builder_get_object(builder, "gdb_entry") );
		exec_entry = GTK_ENTRY( gtk_builder_get_object(builder, "exec_entry") );
		args_entry = GTK_ENTRY( gtk_builder_get_object(builder, "args_entry") );
		working_dir_entry = GTK_ENTRY( gtk_builder_get_object(builder, "working_dir_entry") );
		//env_entry = GTK_ENTRY( gtk_builder_get_object(builder, "env_entry") );

		if( self->current_setup ) {
			gtk_entry_set_text(gdb_entry, self->current_setup->gdb);
			gtk_entry_set_text(exec_entry, self->current_setup->exec);
			gtk_entry_set_text(args_entry, self->current_setup->args);
			gtk_entry_set_text(working_dir_entry, self->current_setup->working_dir);
			//gtk_entry_set_text(env_entry, g_strjoinv(";", self->current_setup->env));

		} else {
			gtk_entry_set_text(gdb_entry, g_find_program_in_path("gdb"));
		}

		if( dlg ) {
			gtk_widget_show_all( GTK_DIALOG(dlg)->vbox );
			gtk_widget_grab_focus( GTK_WIDGET(exec_entry) );
			res = gtk_dialog_run(GTK_DIALOG(dlg));
			if( res==0 ) {
				mi_target_setup_free(self->current_setup);
				self->current_setup = mi_target_setup_new();
				self->current_setup->gdb = g_strdup( gtk_entry_get_text(gdb_entry) );
				self->current_setup->exec = g_strdup( gtk_entry_get_text(exec_entry) );
				self->current_setup->args = g_strdup( gtk_entry_get_text(args_entry) );
				self->current_setup->working_dir = g_strdup( gtk_entry_get_text(working_dir_entry) );

				// TODO : use
				//self->current_setup->env = g_listenv() +g_strsplit( gtk_entry_get_text(env_entry), ";", 0 );

				mi_vdriver_set_target(self->drv, self->current_setup);
			}
			gtk_widget_destroy(dlg);
		}
	}

	g_object_unref(builder);
}

static void gdbmi_menu_run( GtkAction* action, GDBMIPlugin* self ) {
	if( !mi_vdriver_get_target(self->drv) || !mi_vdriver_get_target(self->drv)->succeed )
		gdbmi_menu_setup(action, self);

	if( !mi_vdriver_get_target(self->drv) )
		return;

	switch( mi_vdriver_get_target_status(self->drv) ) {
	case MI_TARGET_ST_NONE:
	case MI_TARGET_ST_EXIT:
		mi_vdriver_command_run(self->drv);
		break;
	case MI_TARGET_ST_DONE:
	case MI_TARGET_ST_STOPPED:
	case MI_TARGET_ST_ERROR:
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
	  { "gdbmi_setup", GTK_STOCK_PREFERENCES, "setup", 0, 0, (GCallback)gdbmi_menu_setup }
	, { "gdbmi_run", GTK_STOCK_MEDIA_PLAY, "run", "F5", 0, (GCallback)gdbmi_menu_run }
	, { "gdbmi_stop", GTK_STOCK_MEDIA_STOP, "stop", 0, 0, (GCallback)gdbmi_menu_stop }
};

void ui_create(GDBMIPlugin* self) {
	gchar* filepath;
	GError* err = 0;

	filepath = g_build_filename(self->app->get_plugins_path(), "gdbmi.ui", NULL);
	if( !filepath ) {
		g_printerr("ERROR(gdbmi) : build ui filepath failed!\n");
		return;
	}

	self->action_group = gtk_action_group_new("gdbmi_action_group");
	gtk_action_group_add_actions(self->action_group, gdbmi_actions, sizeof(gdbmi_actions)/sizeof(GtkActionEntry), self);
	gtk_ui_manager_insert_action_group(puss_get_ui_manager(self->app), self->action_group, 0);

	self->ui_meger_id = gtk_ui_manager_add_ui_from_file(puss_get_ui_manager(self->app), filepath, 0);
	//self->toolbar = GTK_TOOLBAR(gtk_builder_get_object(builder, "gdbmi_toolbar"));
	//self->app->panel_append(self->toolbar, gtk_label_new("debug"), "gdbmi", PUSS_PANEL_POS_BOTTOM);
}

void ui_destroy(GDBMIPlugin* self) {
	gtk_ui_manager_remove_ui(puss_get_ui_manager(self->app), self->ui_meger_id);
	gtk_ui_manager_remove_action_group(puss_get_ui_manager(self->app), self->action_group);
}

static void cb_target_status_changed(MIVDriver* drv, MITargetStatus status, const MIRecord* record, GDBMIPlugin* self) {
	GtkWidget* dlg;
	GtkAction* action;
	gchar* str;
	g_print("SSSSSSSSSSSSSS : %d\n", status);

	action = gtk_action_group_get_action(self->action_group, "gdbmi_run");
	switch( status ) {
	case MI_TARGET_ST_NONE:
	case MI_TARGET_ST_EXIT:
	case MI_TARGET_ST_CONNECTED:
		gtk_action_set_stock_id(action, GTK_STOCK_MEDIA_PLAY);
		gtk_action_set_label(action, "run");
		break;
	case MI_TARGET_ST_DONE:
	case MI_TARGET_ST_STOPPED:
		gtk_action_set_stock_id(action, GTK_STOCK_MEDIA_PLAY);
		gtk_action_set_label( action, "continue" );
		break;
	case MI_TARGET_ST_RUNNING:
		gtk_action_set_stock_id(action, GTK_STOCK_MEDIA_PAUSE);
		gtk_action_set_label( action, "pause" );
		break;
	case MI_TARGET_ST_ERROR:
		str = mi_results_to_str( ((MIResultRecord*)record)->results );
		dlg = gtk_message_dialog_new_with_markup( puss_get_main_window(self->app)
			, 0
			, GTK_MESSAGE_WARNING
			, GTK_BUTTONS_OK
			, "<b>ERROR</b> : \n%s"
			, str );
		g_free(str);

		gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);

		gtk_action_set_stock_id(action, GTK_STOCK_MEDIA_PLAY);
		gtk_action_set_label( action, "continue" );
		break;
	}

	action = gtk_action_group_get_action(self->action_group, "gdbmi_stop");
	gtk_action_set_visible( action, (status!=MI_TARGET_ST_NONE && status!=MI_TARGET_ST_EXIT) );

	gtk_ui_manager_ensure_update( puss_get_ui_manager(self->app) );
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	GDBMIPlugin* self;

	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	self = g_new0(GDBMIPlugin, 1);
	self->app = app;
	self->drv = mi_vdriver_new();
	ui_create(self);

	mi_vdriver_set_callbacks(self->drv, (MITargetStatusChanged)cb_target_status_changed, self, 0);
	cb_target_status_changed(self->drv, MI_TARGET_ST_NONE, 0, self);

	return self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	GDBMIPlugin* self = (GDBMIPlugin*)ext;
	if( !self )
		return;

	if( self->drv )
		mi_vdriver_free(self->drv);

	ui_destroy(self);

	mi_target_setup_free(self->current_setup);

	g_free(self);
}

