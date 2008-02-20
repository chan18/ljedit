// MiniLine.h
// 

#ifndef PUSS_INC_MINILINE_H
#define PUSS_INC_MINILINE_H

#include <gtk/gtk.h>

struct MiniLine {
	GtkWindow*	window;

	GtkLabel*	label;
	GtkEntry*	entry;
};

struct Puss;

struct MiniLineCallback {
	gpointer tag;

	gboolean (*cb_active)( Puss* app, gpointer tag );
	gboolean (*cb_key_press)( Puss* app, GdkEventKey* event, gpointer tag );
	void     (*cb_changed)( Puss* app, gpointer tag );
};

void	puss_mini_line_create( Puss* app );
void	puss_mini_line_destroy( Puss* app );
void	puss_mini_line_active( Puss* app, MiniLineCallback* cb );
void	puss_mini_line_deactive( Puss* app );

#endif//PUSS_INC_MINILINE_H

