// DocManager.cpp
// 

#include "DocManager.h"

#include <assert.h>

struct DocPos {
	gint	page;
	gint	line;
	gint	offset;
};

gboolean doc_scroll_to_pos(DocPos* pos) {
	g_return_val_if_fail(!pos, FALSE);

	g_print("scroll_to : %d:%d", pos->page, pos->line);
    return FALSE;
}

void doc_locate_page_line(Puss* app, int page, int line, int offset) {
	gpointer pos = g_malloc(sizeof(DocPos));
	if( pos ) {
		DocPos* dp = (DocPos*)pos;
		dp->page = page;
		dp->line = line;
		dp->offset = offset;
		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, (GSourceFunc)&doc_scroll_to_pos, pos, &g_free);
	}
}

void puss_doc_new( Puss* app ) {
	g_message("new");
}

gboolean puss_doc_open( Puss* app, const gchar* filepath, int line, int line_offset ) {
	
}
