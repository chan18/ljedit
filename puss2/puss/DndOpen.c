// DndOpen.c
//

#include "Puss.h"
#include "DocManager.h"

static void drag_data_received( GtkWidget* widget
		, GdkDragContext* drag_context
		, gint x
		, gint y
		, GtkSelectionData* data
		, guint info
		, guint time )
{
	gchar** p;
	gchar** uris;
	gchar* filepath;

	if( !data || gtk_selection_data_get_format(data)!=8 )
		return;

	uris = g_uri_list_extract_uris((const gchar*)(gtk_selection_data_get_data(data)));
	if( !uris )
		return;

	for( p=uris; *p; ++p ) {
		filepath = g_filename_from_uri(*p, NULL, NULL);
		if( filepath ) {
			puss_doc_open(filepath, -1, -1, TRUE);
			g_free(filepath);
		}
	}
}

GtkTargetEntry uri_target = { "text/uri-list", 0, 0 };

static void page_added( GtkNotebook* notebook
		, GtkWidget* child
		, guint page_num )
{
	GtkTextView* view = puss_doc_get_view_from_page_num(page_num);
	if( view ) {
		gtk_drag_dest_add_uri_targets(GTK_WIDGET(view));
		g_signal_connect(view, "drag-data-received", G_CALLBACK(&drag_data_received), 0);
	}
}

void puss_dnd_open_support() {
	gtk_drag_dest_set(GTK_WIDGET(puss_app->main_window), GTK_DEST_DEFAULT_ALL, &uri_target, 1, GDK_ACTION_COPY);
	g_signal_connect(puss_app->main_window, "drag-data-received", G_CALLBACK(&drag_data_received), 0);

	gtk_drag_dest_set(GTK_WIDGET(puss_app->doc_panel), GTK_DEST_DEFAULT_ALL, &uri_target, 1, GDK_ACTION_COPY);
	g_signal_connect(puss_app->doc_panel, "drag-data-received", G_CALLBACK(&drag_data_received), 0);
	g_signal_connect(puss_app->doc_panel, "page-added", G_CALLBACK(&page_added), 0);
}

