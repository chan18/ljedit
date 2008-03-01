// PosLocate.cpp
// 

#include "PosLocate.h"

#include "IPuss.h"

struct PosNode;
struct PosList;

struct PosNode {
	PosList*		owner;
	PosNode*		prev;
	PosNode*		next;

	GtkWidget*		page;
	gint			line;
	gint			offset;
};

struct PosList {
	PosNode*		head;
	PosNode*		tail;
	PosNode*		current;

	gsize			count;
};

void pos_list_destroy(PosList* pos_list) {
	if( pos_list ) {
		PosNode* t = 0;
		PosNode* p = pos_list->head;
		while( p ) {
			t = p;
			p = p->next;
			g_free(t);
		}

		g_free(pos_list);
	}
}

GQuark quark_pos_list = g_quark_from_static_string("puss_pos_locate_list");

PosList* pos_locate_get_list( Puss* app ) {
	GObject* owner = G_OBJECT(app->main_window->doc_panel);

	PosList* pos_list = (PosList*)g_object_get_qdata(owner, quark_pos_list);
	if( !pos_list ) {
		pos_list = g_try_new0(PosList, 1);
		if( pos_list )
			g_object_set_qdata_full(owner, quark_pos_list, pos_list, (GDestroyNotify)&pos_list_destroy);
	}

	return pos_list;
}

void page_delete_notify(PosNode* node, GObject* where_the_object_was) {
	// TODO : 
	g_free(node);
	return;
}

void puss_pos_locate_add( Puss* app, gint page_num, gint line, gint offset) {
	if( page_num < 0 || line <= 0 )
		return;

	GtkWidget* page = gtk_notebook_get_nth_page(app->main_window->doc_panel, page_num);
	if( !page )
		return;

	PosList* pos_list = pos_locate_get_list(app);
	if( !pos_list )
		return;

	// TODO : 
	// node from pool or list.head
	PosNode* node = g_try_new0(PosNode, 1);
	if( node ) {
		node->page = page;
		node->line = line;
		node->offset = offset;

		// node->prev = pos_list->current;
		// remove after current
		// 
		++pos_list->count;

		g_object_weak_ref(G_OBJECT(page), (GWeakNotify)&page_delete_notify, node);
	}
}

void puss_pos_locate_forward( Puss* app ) {
	// TODO : 
	PosList* pos_list = pos_locate_get_list(app);
	if( pos_list ) {
	}
}

void puss_pos_locate_back( Puss* app ) {
	// TODO : 
	PosList* pos_list = pos_locate_get_list(app);
	if( pos_list ) {
	}
}

