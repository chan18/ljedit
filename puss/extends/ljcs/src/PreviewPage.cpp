// PreviewPage.cpp
// 

#include "PreviewPage.h"
#include "IPuss.h"

#include <sstream>

gboolean PreviewPage::create(Puss* app) {
	GtkBuilder* builder = gtk_builder_new();
	if( !builder )
		return FALSE;

	gchar* filepath = g_build_filename(app->get_module_path(), "extends", "ljcs_res", "preview_page_ui.xml", NULL);
	if( !filepath ) {
		g_printerr("ERROR(ljcs) : build ui filepath failed!\n");
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	GError* err = 0;
	gtk_builder_add_from_file(builder, filepath, &err);
	g_free(filepath);

	if( err ) {
		g_printerr("ERROR(ljcs): %s\n", err->message);
		g_error_free(err);
		g_object_unref(G_OBJECT(builder));
		return FALSE;
	}

	GtkWidget* vbox = GTK_WIDGET(g_object_ref(gtk_builder_get_object(builder, "vbox")));
	g_assert( vbox );
	gtk_widget_show_all(vbox);
	gtk_notebook_append_page(puss_get_bottom_panel(app), vbox, gtk_label_new("Preview"));

	g_object_unref(G_OBJECT(builder));
	return TRUE;
}

void PreviewPage::destroy() {

}

