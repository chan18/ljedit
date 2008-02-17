// MiniLine.h
// 

#ifndef PUSS_INC_CMD_WINDOW_H
#define PUSS_INC_CMD_WINDOW_H

#include <gtk/gtk.h>

struct MiniLine {
	GtkWindow*	window;

	GtkLabel*	label;
	GtkEntry*	entry;
};

struct Puss;

struct CmdLineCallback {
	gboolean (*cb_active)( Puss* app, void* tag );
	gboolean (*cb_key_press)( Puss* app, GdkEventKey* event );
	void     (*cb_changed)( Puss* app );
};

void	puss_mini_line_create( Puss* app );
void	puss_mini_line_active( Puss* app, CmdLineCallback* cb, gint x, gint y, void* tag );
void	puss_mini_line_deactive( Puss* app );

#endif//PUSS_INC_CMD_WINDOW_H

