// module.cpp
//

#include "IPuss.h"

void drag_data_received( GtkWidget* widget
		, GdkDragContext* drag_context
		, gint x
		, gint y
		, GtkSelectionData* data
		, guint info
		, guint time
		, Puss* app )
{
	if( !data || data->format!=8 )
		return;

	gchar** uris = g_uri_list_extract_uris((const gchar*)(data->data));
	if( !uris )
		return;

	for( gchar** p=uris; *p; ++p ) {
		gchar* filepath = g_filename_from_uri(*p, NULL, NULL);
		if( filepath ) {
			app->api->doc_open(app, filepath, 0, 0);
			g_free(filepath);
		}
	}
}

GtkTargetEntry uri_target = { "text/uri-list", 0, 0 };

void page_added( GtkNotebook* notebook
		, GtkWidget* child
		, guint page_num
		, Puss* app )
{
	GtkTextView* view = app->api->doc_get_view_from_page_num(app, page_num);
	if( view ) {
		gtk_drag_dest_set(GTK_WIDGET(view), GTK_DEST_DEFAULT_ALL, &uri_target, 1, GDK_ACTION_COPY);
		g_signal_connect(view, "drag-data-received", G_CALLBACK(&drag_data_received), app);
	}
}

PUSS_EXPORT void* puss_extend_create(Puss* app) {
	GtkWidget* main_window = GTK_WIDGET(puss_get_main_window(app));
	gtk_drag_dest_set(main_window, GTK_DEST_DEFAULT_ALL, &uri_target, 1, GDK_ACTION_COPY);
	g_signal_connect(main_window, "drag-data-received", G_CALLBACK(&drag_data_received), app);

	GtkWidget* doc_panel = GTK_WIDGET(puss_get_doc_panel(app));
	gtk_drag_dest_set(doc_panel, GTK_DEST_DEFAULT_ALL, &uri_target, 1, GDK_ACTION_COPY);
	g_signal_connect(doc_panel, "drag-data-received", G_CALLBACK(&drag_data_received), app);
	g_signal_connect(doc_panel, "page-added", G_CALLBACK(&page_added), app);

	return 0;
}

PUSS_EXPORT void  puss_extend_destroy(void* self) {
}

