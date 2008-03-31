// module.cpp
//

#include <glib/gi18n.h>

#include "IPuss.h"


PUSS_EXPORT void* puss_extend_create(Puss* app) {
	// init UI
	GtkBuilder* builder = gtk_builder_new();
	if( !builder )
		return FALSE;

	gchar* filepath = g_build_filename(app->get_module_path(), "extends", "search_tools_res", "search_tools_ui.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(search_tools) : build preview page ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	GError* err = 0;
	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(search_tools): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	GtkWidget* panel = GTK_WIDGET(gtk_builder_get_object(builder, "search_tools_panel"));
	g_assert( panel );

	gtk_widget_show_all(panel);
	gtk_notebook_append_page(puss_get_bottom_panel(app), panel, gtk_label_new(_("Search")));

	//gtk_builder_connect_signals(builder, self);
	g_object_unref(G_OBJECT(builder));

	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
	//g_print("* PluginEngine extends destroy\n");

	// !!! do not use gtk widgets here
	// !!! gtk_main already exit
	// !!! use gtk_quit_add() register destroy callback

	// free resource
}

