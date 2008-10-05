// MiniLine.h
// 

#ifndef PUSS_INC_MINILINE_H
#define PUSS_INC_MINILINE_H

#include <gtk/gtk.h>

typedef struct _MiniLineCallback MiniLineCallback;

typedef struct _MiniLine {
	GtkWidget*			window;
	GtkImage*			image;
	GtkEntry*			entry;

	gulong				signal_id_changed;
	gulong				signal_id_key_press;

	MiniLineCallback*	cb;
} MiniLine;

gboolean	puss_mini_line_create();
void		puss_mini_line_destroy();

void		puss_mini_line_active(MiniLineCallback* cb);
void		puss_mini_line_deactive();

#endif//PUSS_INC_MINILINE_H

