// CmdLine.h
// 

#ifndef PUSS_INC_CMDLINE_H
#define PUSS_INC_CMDLINE_H

#include <gtk/gtk.h>

struct Puss;

struct CmdLineCallback {
	gboolean (*cb_active)( Puss* app, void* tag );
	gboolean (*cb_key_press)( Puss* app, GdkEventKey* event );
	void     (*cb_changed)( Puss* app );
};

GtkLabel*	puss_cmd_line_get_label( GtkWidget* cmd_line );
GtkEntry*	puss_cmd_line_get_entry( GtkWidget* cmd_line );

void		puss_cmd_line_create( Puss* app );
void		puss_cmd_line_destroy( Puss* app );

void		puss_cmd_line_active( Puss* app, CmdLineCallback* cb, gint x, gint y, void* tag );
void		puss_cmd_line_deactive( Puss* app );

#endif//PUSS_INC_CMDLINE_H

