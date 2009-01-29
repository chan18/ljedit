// PanelOrder.c

#include "PanelOrder.h"

#include "Puss.h"
#include "OptionManager.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#define PUSS_PANEL_SORT_ID "__PUSS_PANEL_SORT_ID__"

typedef struct _PageNode PageNode;

struct _PageNode {
	const gchar*	id;
	GtkWidget*		page;
	GtkWidget*		tab;
	GtkNotebook*	parent;

	PageNode*		next;
};

typedef struct _PosNode PosNode;

struct _PosNode {
	PosNode*	prev;
	PosNode*	next;
	GtkWidget*	panel;
	gchar		id[0];
};

static PosNode* puss_left_panel_order_list = 0;
static PosNode* puss_right_panel_order_list = 0;
static PosNode* puss_bottom_panel_order_list = 0;

static PosNode* panel_pos_new(const gchar* id) {
	size_t len = strlen(id);
	PosNode* pos = g_malloc0(sizeof(PosNode) + (gsize)len + 1);
	memcpy(pos->id, id, len + 1);
	return pos;
}

static PosNode* option_order_list_parse(const gchar* option_value) {
	PosNode* ret = 0;
	PosNode* ps = 0;
	PosNode* pe = 0;

	if( option_value ) {
		gchar** p;
		gchar** items = g_strsplit_set(option_value, ",; \t\r\n", 0);
		for( p=items; *p; ++p ) {
			if( *p[0]=='\0' )
				continue;
			pe = panel_pos_new(*p);
			if( ps )
				ps->next = pe;
			pe->prev = ps;
			ps = pe;
			if( !ret )
				ret = ps;
		}
		g_strfreev(items);
	}

	return ret;
}

#define MAX_ORDER_LEN 1024

static gchar* option_order_list_build(PosNode* order_list) {
	guint i;
	PosNode* ps;
	gchar* arr[MAX_ORDER_LEN+1];
	gchar** pd;

	ps=order_list;
	pd = arr;

	for( i=0; i<MAX_ORDER_LEN && ps; ++i ) {
		*pd = ps->id;
		++pd;
		ps = ps->next;
	}
	*pd = 0;

	return g_strjoinv(" ", arr);
}

static void reorder_panel(GtkNotebook* notebook, guint page_num, PosNode* pos, PosNode** order_list) {
	PosNode* p;
	guint i;

	p = 0;
	for( i=page_num; !p && i>0; --i )
		p = (PosNode*)g_object_get_data( G_OBJECT(gtk_notebook_get_nth_page(notebook, i-1)), PUSS_PANEL_SORT_ID );

	if( p ) {
		pos->prev = p;
		pos->next = p->next;
		if( p->next )
			p->next->prev = pos;
		p->next = pos;
	} else {
		pos->prev = 0;
		pos->next = *order_list;
		if( pos->next )
			pos->next->prev = pos;
		*order_list = pos;
	}
}

static void cb_panel_reordered(GtkNotebook* notebook, GtkWidget* child, guint page_num, PosNode** order_list) {
	PosNode* pos;

	pos = (PosNode*)g_object_get_data(G_OBJECT(child), PUSS_PANEL_SORT_ID);
	if( !pos )
		return;

	if( pos->prev )
		pos->prev->next = pos->next;
	else
		*order_list = pos->next;

	if( pos->next )
		pos->next->prev = pos->prev;

	reorder_panel(notebook, page_num, pos, order_list);
}

static void cb_panel_added(GtkNotebook* notebook, GtkWidget* child, guint page_num, PosNode** order_list) {
	PosNode* pos = (PosNode*)g_object_get_data(G_OBJECT(child), PUSS_PANEL_SORT_ID);
	if( !pos )
		return;

	pos->panel = child;
	if( pos->prev ) {
		pos->prev->next = pos->next;
	} else {
		if( pos==puss_left_panel_order_list )
			puss_left_panel_order_list = pos->next;
		else if( pos==puss_right_panel_order_list )
			puss_right_panel_order_list = pos->next;
		else if( pos==puss_bottom_panel_order_list )
			puss_bottom_panel_order_list = pos->next;
	}

	if( pos->next )
		pos->next->prev = pos->prev;

	reorder_panel(notebook, page_num, pos, order_list);
}

static void cb_panel_removed(GtkNotebook* notebook, GtkWidget* child, guint page_num, PosNode** order_list) {
	PosNode* pos = (PosNode*)g_object_get_data(G_OBJECT(child), PUSS_PANEL_SORT_ID);
	if( !pos )
		return;
	pos->panel = 0;
}

void puss_panel_order_load() {
	const Option* option;

	option = puss_option_manager_option_reg("puss", "left_panels_order", "");
	puss_left_panel_order_list = option_order_list_parse(option->value);
	g_signal_connect(puss_app->left_panel, "page-reordered", G_CALLBACK(cb_panel_reordered), &puss_left_panel_order_list);
	g_signal_connect(puss_app->left_panel, "page-added", G_CALLBACK(cb_panel_added), &puss_left_panel_order_list);
	g_signal_connect(puss_app->left_panel, "page-removed", G_CALLBACK(cb_panel_removed), &puss_left_panel_order_list);

	option = puss_option_manager_option_reg("puss", "right_panels_order", "");
	puss_right_panel_order_list = option_order_list_parse(option->value);
	g_signal_connect(puss_app->right_panel, "page-reordered", G_CALLBACK(cb_panel_reordered), &puss_right_panel_order_list);
	g_signal_connect(puss_app->right_panel, "page-added", G_CALLBACK(cb_panel_added), &puss_right_panel_order_list);
	g_signal_connect(puss_app->right_panel, "page-removed", G_CALLBACK(cb_panel_removed), &puss_right_panel_order_list);

	option = puss_option_manager_option_reg("puss", "bottom_panels_order", "");
	puss_bottom_panel_order_list = option_order_list_parse(option->value);
	g_signal_connect(puss_app->bottom_panel, "page-reordered", G_CALLBACK(cb_panel_reordered), &puss_bottom_panel_order_list);
	g_signal_connect(puss_app->bottom_panel, "page-added", G_CALLBACK(cb_panel_added), &puss_bottom_panel_order_list);
	g_signal_connect(puss_app->bottom_panel, "page-removed", G_CALLBACK(cb_panel_removed), &puss_bottom_panel_order_list);
}

void puss_panel_order_save() {
	const Option* option;
	gchar* option_value;

	option = puss_option_manager_option_find("puss", "left_panels_order");
	option_value = option_order_list_build(puss_left_panel_order_list);
	puss_option_manager_option_set(option, option_value);
	g_free(option_value);

	option = puss_option_manager_option_find("puss", "right_panels_order");
	option_value = option_order_list_build(puss_right_panel_order_list);
	puss_option_manager_option_set(option, option_value);
	g_free(option_value);

	option = puss_option_manager_option_find("puss", "bottom_panels_order");
	option_value = option_order_list_build(puss_bottom_panel_order_list);
	puss_option_manager_option_set(option, option_value);
	g_free(option_value);
}

gboolean puss_panel_get_pos(GtkWidget* panel, GtkNotebook** parent, gint* page_num) {
	gint i;
	gint count;
	GtkWidget* page;
	GtkNotebook* nb;
	
	if( panel ) {
		nb = GTK_NOTEBOOK( gtk_widget_get_parent(panel) );
		count = gtk_notebook_get_n_pages(nb);
		for( i=0; i<count; ++i ) {
			page = gtk_notebook_get_nth_page(nb, i);
			if( page==panel ) {
				if( parent )
					*parent = nb;
				if( page_num )
					*page_num = i;
				return TRUE;
			}
		}
	}

	return FALSE;
}

void puss_panel_append(GtkWidget* panel, GtkWidget* tab_label, const gchar* id, PanelPosition default_pos) {
	GtkNotebook* parent = 0;
	PosNode** order_list = 0;
	PosNode* p = 0;
	PosNode* pos = 0;
	gint page_pos = 0;
	gint page_num;

	if( !parent ) {
		for( pos=puss_left_panel_order_list; pos; pos=pos->next) {
			if( g_str_equal(pos->id, id) ) {
				parent = puss_app->left_panel;
				order_list = &puss_left_panel_order_list;
				break;
			}
		}
	}

	if( !parent ) {
		for( pos=puss_right_panel_order_list; pos; pos=pos->next) {
			if( g_str_equal(pos->id, id) ) {
				parent = puss_app->right_panel;
				order_list = &puss_right_panel_order_list;
					break;
			}
		}
	}

	if( !parent ) {
		for( pos=puss_bottom_panel_order_list; pos; pos=pos->next) {
			if( g_str_equal(pos->id, id) ) {
				parent = puss_app->bottom_panel;
				order_list = &puss_bottom_panel_order_list;
					break;
			}
		}
	}

	if( !parent ) {
		switch( default_pos ) {
		case PUSS_PANEL_POS_HIDE:
			// TODO : use hidden notebook
		case PUSS_PANEL_POS_LEFT:
			parent = puss_app->left_panel;
			order_list = &puss_left_panel_order_list;
			break;
		case PUSS_PANEL_POS_RIGHT:
			parent = puss_app->right_panel;
			order_list = &puss_right_panel_order_list;
			break;
		case PUSS_PANEL_POS_BOTTOM:
		default:
			parent = puss_app->bottom_panel;
			order_list = &puss_bottom_panel_order_list;
			break;
		}
		pos = panel_pos_new(id);
		if( *order_list ) {
			for( p=*order_list; p->next; p=p->next );
			p->next = pos;
			pos->prev = p;
		} else {
			*order_list = pos;
		}
	}

	g_object_set_data(G_OBJECT(panel), PUSS_PANEL_SORT_ID, pos);
	pos->panel = panel;

	for( p=pos->prev; p; p=p->prev ) {
		if( p->panel ) {
			page_pos = gtk_notebook_page_num(parent, p->panel) + 1;
			break;
		}
	}

	page_num = gtk_notebook_get_n_pages(parent);
	g_signal_handlers_block_by_func(parent, &cb_panel_added, order_list);
	if( page_pos >= page_num )
		gtk_notebook_append_page(parent, panel, tab_label);
	else
		gtk_notebook_insert_page(parent, panel, tab_label, page_pos);
	g_signal_handlers_unblock_by_func(parent, &cb_panel_added, order_list);

	gtk_notebook_set_tab_reorderable(parent, panel, TRUE);
	gtk_notebook_set_tab_detachable(parent, panel, TRUE);
}

void puss_panel_remove(GtkWidget* panel) {
	gint page_num;
	GtkNotebook* parent;
	PosNode* pos;

	pos = (PosNode*)g_object_get_data(G_OBJECT(panel), PUSS_PANEL_SORT_ID);
	if( pos )
		pos->panel = 0;

	if( puss_panel_get_pos(panel, &parent, &page_num) )
		gtk_notebook_remove_page(parent, page_num);
}

