// PosLocate.cpp
// 

#include "PosLocate.h"

#include <stdlib.h>

#include "Puss.h"
#include "DocManager.h"

#define POS_NODE_MAX 128

struct PosNode;
struct PosList;

struct PosNode {
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

	PosNode*		free_list;

	PosNode			pool[POS_NODE_MAX];
};

gboolean __debug_pos_list_check() {
	PosList* list = puss_app->pos_list;
	size_t count = 0;
	PosNode* p;
	for( p = list->head; p && count <= POS_NODE_MAX; p = p->next )
		++count;
	for( p = list->free_list; p && count <= POS_NODE_MAX; p = p->next )
		++count;
	return count==POS_NODE_MAX;
}

gboolean __debug_pos_list_check_node(PosNode* node) {
	PosList* list = puss_app->pos_list;
	PosNode* p = list->head;
	for(; p && p!=node; p = p->next);
	return p==node;
}

gboolean puss_pos_locate_create() {
	puss_app->pos_list = g_new0(PosList, 1);
	if( !puss_app->pos_list ) {
		g_printerr("ERROR(pos locate) : pos list create failed!\n");
		return FALSE;
	}

	PosNode* pool = puss_app->pos_list->pool;
	for( int i=0; i<POS_NODE_MAX-1; ++i )
		pool[i].next = &pool[i+1];

	puss_app->pos_list->free_list = &pool[0];

	g_assert(__debug_pos_list_check());

	return TRUE;
}

void puss_pos_locate_destroy() {
	g_free(puss_app->pos_list);
	puss_app->pos_list = 0;
}

void page_delete_notify(PosNode* node, GObject* where_the_object_was) {
	PosList* list = puss_app->pos_list;

	g_assert(__debug_pos_list_check_node(node));

	if( node->next )
		node->next->prev = node->prev;
	else
		list->tail = node->prev;

	if( node->prev )
		node->prev->next = node->next;
	else
		list->head = node->next;

	if( list->current==node ) {
		if( node->next )
			list->current = node->next;
		else
			list->current = list->tail;
	}

	node->next = list->free_list;
	list->free_list = node;

	g_assert(__debug_pos_list_check());
}

void puss_pos_locate_add(int page_num, int line, int offset) {
	if( page_num < 0 || line < 0 )
		return;

	GtkWidget* page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);
	if( !page )
		return;

	PosNode* node = 0;
	PosList* list = puss_app->pos_list;
	if( list->current ) {
		// if in same as current pos (modify offset only)
		if( list->current->page==page && list->current->line==line ) {
			list->current->offset = offset;
			return;
		}

		// remove all forward from current
		g_assert( list->head && list->tail );

		for( node = list->current->next; node; node = node->next )
			g_object_weak_unref(G_OBJECT(node->page), (GWeakNotify)&page_delete_notify, node);

		list->tail->next = list->free_list;
		list->free_list = list->current->next;
		list->tail = list->current;
		list->tail->next = 0;
	}

	node = puss_app->pos_list->free_list;
	if( node ) {
		puss_app->pos_list->free_list = node->next;

	} else {
		g_assert( list->head && list->head->next );

		node = list->head;
		list->head = node->next;
		list->head->prev = 0;
	}

	node->page = page;
	node->line = line;
	node->offset = offset;

	g_object_weak_ref(G_OBJECT(page), (GWeakNotify)&page_delete_notify, node);

	node->next = 0;
	node->prev = list->tail;

	if( !list->head )
		list->head = node;

	if( !list->tail ) {
		list->tail = node;

	} else {
		list->tail->next = node;
		list->tail = node;
	}

	list->current = list->tail;

	g_assert(__debug_pos_list_check());
}

void puss_pos_locate_add_current_pos() {
	gint page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	if( page_num < 0 )
		return;

	GtkWidget* page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);
	GtkTextBuffer* buf = puss_doc_get_buffer_from_page(page);
	if( !buf )
		return;

	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	gint line = gtk_text_iter_get_line(&iter);
	gint offset = gtk_text_iter_get_line_offset(&iter);

	puss_pos_locate_add(page_num, line, offset);
}

void puss_pos_locate_current() {
	PosList* list = puss_app->pos_list;
	if( !list->current )
		return;

	gint page_num = gtk_notebook_page_num(puss_app->doc_panel, list->current->page);
	if( page_num < 0 )
		return;

	puss_doc_locate(page_num, list->current->line, list->current->offset, FALSE);
}

void puss_pos_locate_forward() {
	PosList* list = puss_app->pos_list;

	if( list->current && list->current->next )
		list->current = list->current->next;

	puss_pos_locate_current();
}

void puss_pos_locate_back() {
	PosList* list = puss_app->pos_list;

	if( list->current ) {
		if( !list->current->prev )
			return;
		list->current = list->current->prev;

	} else {
		if( !list->tail )
			return;
		list->current = list->tail;
	}

	puss_pos_locate_current();
}

