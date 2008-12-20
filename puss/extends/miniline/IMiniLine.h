// MiniLine.h
// 

#ifndef PUSS_EXT_INC_IMINILINE_H
#define PUSS_EXT_INC_IMINILINE_H

#include "IPuss.h"

typedef struct _MiniLine MiniLine;

typedef struct {
	gpointer	tag;

	gboolean 	(*cb_active)( MiniLine* self, gpointer tag );
	gboolean 	(*cb_key_press)( MiniLine* self, GdkEventKey* event, gpointer tag );
	void     	(*cb_changed)( MiniLine* self, gpointer tag );
} MiniLineCallback;

struct _MiniLine {
	Puss*		app;

	GtkWidget*	window;
	GtkImage*	image;
	GtkEntry*	entry;

	void		(*active)( MiniLine* self, MiniLineCallback* cb );
	void		(*deactive)( MiniLine* self );
};

#endif//PUSS_EXT_INC_IMINILINE_H

