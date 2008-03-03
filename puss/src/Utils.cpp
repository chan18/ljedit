// Utils.cpp
//

#include "Utils.h"

// Cut and paste from gtkwindow.c
void puss_send_focus_change(GtkWidget *widget, gboolean in) {
	GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

	g_object_ref (widget);
   
	if (in)
		GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	else
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	fevent->focus_change.type = GDK_FOCUS_CHANGE;
	fevent->focus_change.window = GDK_WINDOW (g_object_ref(widget->window));
	fevent->focus_change.in = in;
  
	gtk_widget_event (widget, fevent);
  
	g_object_notify (G_OBJECT (widget), "has-focus");

	g_object_unref (widget);
	gdk_event_free (fevent);
}

void puss_active_panel_page(GtkNotebook* panel, gint page_num) {
	GtkWidget* w = gtk_notebook_get_nth_page(panel, page_num);
	if( w ) {
		gtk_notebook_set_current_page(panel, page_num);
		puss_send_focus_change(w, FALSE);
		puss_send_focus_change(w, TRUE);
	}
}

