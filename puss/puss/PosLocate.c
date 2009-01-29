// PosLocate.c
// 

#include "PosLocate.h"

#include <stdlib.h>

#include "Puss.h"
#include "DocManager.h"

#define POS_NODE_MAX 256

typedef struct _PosNode PosNode;

struct _PosNode {
	PosNode*		prev;
	PosNode*		next;

	GtkWidget*		page;
	gint			line;
	gint			offset;
};

typedef struct _PosList PosList;

struct _PosList {
	PosNode*		head;
	PosNode*		tail;
	PosNode*		current;

	PosNode*		free_list;

	PosNode			pool[POS_NODE_MAX];
};

static PosList* puss_pos_list = 0;

gboolean __debug_pos_list_check() {
	PosNode* p;
	size_t count = 0;
	for( p = puss_pos_list->head; p && count <= POS_NODE_MAX; p = p->next )
		++count;
	for( p = puss_pos_list->free_list; p && count <= POS_NODE_MAX; p = p->next )
		++count;
	return count==POS_NODE_MAX;
}

gboolean __debug_pos_list_check_node(PosNode* node) {
	PosNode* p = puss_pos_list->head;
	for(; p && p!=node; p = p->next);
	return p==node;
}

gboolean puss_pos_locate_create() {
	PosNode* pool;
	gint i;
	puss_pos_list = g_new0(PosList, 1);

	pool = puss_pos_list->pool;
	for( i=0; i<POS_NODE_MAX-1; ++i )
		pool[i].next = &pool[i+1];

	puss_pos_list->free_list = &pool[0];

	g_assert(__debug_pos_list_check());

	return TRUE;
}

void puss_pos_locate_destroy() {
	g_free(puss_pos_list);
	puss_pos_list = 0;
}

void page_delete_notify(PosNode* node, GObject* where_the_object_was) {
	PosList* list = puss_pos_list;

	g_assert(node && G_OBJECT(node->page)==where_the_object_was);

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

void puss_pos_locate_add(gint page_num, gint line, gint offset) {
	GtkWidget* page;
	PosList* list;
	PosNode* node = 0;
	if( page_num < 0 || line < 0 )
		return;

	page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);
	if( !page )
		return;

	list = puss_pos_list;
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

	node = puss_pos_list->free_list;
	if( node ) {
		puss_pos_list->free_list = node->next;

	} else {
		g_assert( list->head && list->head->next );

		node = list->head;
		list->head = node->next;
		list->head->prev = 0;

		g_object_weak_unref(G_OBJECT(node->page), (GWeakNotify)&page_delete_notify, node);
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
	GtkTextIter iter;
	gint line;
	gint offset;
	gint page_num;
	GtkWidget* page;
	GtkTextBuffer* buf;

	page_num = gtk_notebook_get_current_page(puss_app->doc_panel);
	if( page_num < 0 )
		return;

	page = gtk_notebook_get_nth_page(puss_app->doc_panel, page_num);
	buf = puss_doc_get_buffer_from_page(page);
	if( !buf )
		return;

	gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
	line = gtk_text_iter_get_line(&iter);
	offset = gtk_text_iter_get_line_offset(&iter);

	puss_pos_locate_add(page_num, line, offset);
}

void puss_pos_locate_current() {
	gint page_num;
	if( !puss_pos_list->current )
		return;

	page_num = gtk_notebook_page_num(puss_app->doc_panel, puss_pos_list->current->page);
	if( page_num < 0 )
		return;

	puss_doc_locate(page_num, puss_pos_list->current->line, puss_pos_list->current->offset, FALSE);
}

void puss_pos_locate_forward() {
	if( puss_pos_list->current && puss_pos_list->current->next )
		puss_pos_list->current = puss_pos_list->current->next;

	puss_pos_locate_current();
}

void puss_pos_locate_back() {
	if( puss_pos_list->current ) {
		if( !puss_pos_list->current->prev )
			return;
		puss_pos_list->current = puss_pos_list->current->prev;

	} else {
		if( !puss_pos_list->tail )
			return;
		puss_pos_list->current = puss_pos_list->tail;
	}

	puss_pos_locate_current();
}

