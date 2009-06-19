// searcher.c
// 

#include "searcher.h"

struct _SNode {
	GList*		elems;
	GHashTable*	sub;
};

static SNode* snode_new() {
	return g_slice_new0(SNode);
}

static void snode_free(SNode* snode) {
	if( snode ) {
		if( snode->elems )
			g_list_free(snode->elems);
		if( snode->sub )
			g_hash_table_destroy(snode->sub);
		g_slice_free(SNode, snode);
	}
}

void cpp_stree_init(CppSTree* self) {
	self->root = snode_new();

	g_static_rw_lock_init( &(self->lock) );
}

void cpp_stree_final(CppSTree* self) {
	snode_free(self->root);
	self->root = 0;

	g_static_rw_lock_free( &(self->lock) );
}

static SNode* find_sub_node(SNode* parent, const gchar* key) {
	return parent->sub ? g_hash_table_lookup(parent->sub, key) : 0;
}

static SNode* make_sub_node(SNode* parent, const gchar* key) {
	gchar* skey = 0;
	SNode* snode = 0;

	if( !parent->sub )
		parent->sub = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)snode_free);

	if( parent->sub ) {
		snode = g_hash_table_lookup(parent->sub, key);
		if( !snode ) {
			skey = g_strdup(key);
			snode = snode_new();
			if( skey && snode ) {
				g_hash_table_insert(parent->sub, skey, snode);
			} else {
				g_free(skey);
				snode_free(snode);
			}
		}
	}

	return snode;
}

static SNode* snode_locate(SNode* parent, TinyStr* nskey, gboolean make_if_not_exist) {
	SNode* snode;
	gchar* ps;
	gchar* pe;
	gchar ch;

	if( !nskey )
		return parent;

	snode = parent;
	ps = nskey->buf;

	while( *ps && snode ) {
		pe = ps + 1;
		while(*pe && *pe!='.' )
			++pe;

		ch = *pe;
		*pe = '\0';
		snode = make_if_not_exist
			? make_sub_node(parent, ps)
			: find_sub_node(parent, ps);
		*pe = ch; 
		ps = pe;
	}

	return snode;
}

static SNode* cpp_snode_sub_insert(SNode* parent, TinyStr* nskey, CppElem* elem) {
	SNode* snode = 0;
	SNode* scope_snode;

	scope_snode = snode_locate(parent, nskey, TRUE);
	if( scope_snode ) {
		snode = make_sub_node(scope_snode, elem->name->buf);
		snode->elems = g_list_append(snode->elems, elem);
	}
	return snode;
}

static SNode* cpp_snode_sub_remove(SNode* parent, TinyStr* nskey, CppElem* elem) {
	SNode* snode;
	SNode* scope_snode;

	scope_snode = snode_locate(parent, nskey, FALSE);
	if( scope_snode ) {
		snode = find_sub_node(scope_snode, elem->name->buf);
		if( snode )
			snode->elems = g_list_remove(snode->elems, elem);
	}

	return snode;
}

static void cpp_snode_insert(SNode* parent, CppElem* elem);
static void cpp_snode_remove(SNode* node, CppElem* elem);

static void cpp_snode_insert_list(SNode* parent, GList* elems) {
	GList* p;

	for( p=elems; p; p=p->next )
		cpp_snode_insert(parent, (CppElem*)(p->data));
}

static void cpp_snode_remove_list(SNode* parent, GList* elems) {
	GList* p;

	for( p=elems; p; p=p->next )
		cpp_snode_remove(parent, (CppElem*)(p->data));
}

static void cpp_snode_insert(SNode* parent, CppElem* elem) {
	switch( elem->type ) {
	case CPP_ET_NCSCOPE:
		cpp_snode_insert_list(parent, elem->v_ncscope.scope);
		break;
		
	case CPP_ET_VAR:
		cpp_snode_sub_insert(parent, elem->v_var.nskey, elem);
		break;
		
	case CPP_ET_FUN:
		cpp_snode_sub_insert(parent, elem->v_fun.nskey, elem);
		break;
		
	case CPP_ET_MACRO:
	case CPP_ET_TYPEDEF:
	case CPP_ET_ENUMITEM:
		cpp_snode_sub_insert(parent, 0, elem);
		break;
		
	case CPP_ET_ENUM:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = cpp_snode_sub_insert(parent, elem->v_enum.nskey, elem);
				if( snode )
					cpp_snode_insert_list(snode, elem->v_ncscope.scope);
			}
			cpp_snode_insert_list(parent, elem->v_ncscope.scope);
		}
		break;
		
	case CPP_ET_CLASS:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = cpp_snode_sub_insert(parent, elem->v_enum.nskey, elem);
				if( snode )
					cpp_snode_insert_list(snode, elem->v_ncscope.scope);
					
			} else if( elem->v_class.class_type==CPP_CLASS_TYPE_UNION ) {
				cpp_snode_insert_list(parent, elem->v_ncscope.scope);
			}
		}
		break;
		
	case CPP_ET_USING:
		{
			if( elem->v_using.isns )
				; // scope.usings.push_back( (Using*)elem );
			else
				cpp_snode_sub_insert(parent, 0, elem);
		}
		break;
		
	case CPP_ET_NAMESPACE:
		{
			if( elem->name->buf[0]!='@' )
				parent = cpp_snode_sub_insert(parent, 0, elem);

			cpp_snode_insert_list(parent, elem->v_ncscope.scope);
		}
		break;
	}
}

static void cpp_snode_remove(SNode* parent, CppElem* elem) {
	switch( elem->type ) {
	case CPP_ET_NCSCOPE:
		cpp_snode_remove_list(parent, elem->v_ncscope.scope);
		break;
		
	case CPP_ET_VAR:
		cpp_snode_sub_remove(parent, elem->v_var.nskey, elem);
		break;
		
	case CPP_ET_FUN:
		cpp_snode_sub_remove(parent, elem->v_fun.nskey, elem);
		break;
		
	case CPP_ET_MACRO:
	case CPP_ET_TYPEDEF:
	case CPP_ET_ENUMITEM:
		cpp_snode_sub_remove(parent, 0, elem);
		break;
		
	case CPP_ET_ENUM:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = cpp_snode_sub_remove(parent, elem->v_enum.nskey, elem);
				if( snode )
					cpp_snode_remove_list(snode, elem->v_ncscope.scope);
			}
			cpp_snode_remove_list(parent, elem->v_ncscope.scope);
		}
		break;
		
	case CPP_ET_CLASS:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = cpp_snode_sub_remove(parent, elem->v_enum.nskey, elem);
				if( snode )
					cpp_snode_remove_list(snode, elem->v_ncscope.scope);
					
			} else if( elem->v_class.class_type==CPP_CLASS_TYPE_UNION ) {
				cpp_snode_remove_list(parent, elem->v_ncscope.scope);
			}
		}
		break;
		
	case CPP_ET_USING:
		{
			if( elem->v_using.isns )
				; // scope.usings.push_back( (Using*)elem );
			else
				cpp_snode_sub_remove(parent, 0, elem);
		}
		break;
		
	case CPP_ET_NAMESPACE:
		{
			if( elem->name->buf[0]!='@' )
				parent = cpp_snode_sub_remove(parent, 0, elem);

			cpp_snode_remove_list(parent, elem->v_ncscope.scope);
		}
		break;
	}
}

void cpp_stree_insert(CppSTree* self, CppFile* file) {
	g_static_rw_lock_writer_lock( &(self->lock) );
	cpp_snode_insert( self->root, &(file->root_scope) );
	g_static_rw_lock_writer_unlock( &(self->lock) );
}

void cpp_stree_remove(CppSTree* self, CppFile* file) {
	g_static_rw_lock_writer_lock( &(self->lock) );
	cpp_snode_remove( self->root, &(file->root_scope) );
	g_static_rw_lock_writer_unlock( &(self->lock) );
}


void cpp_search( CppSTree* stree
	, const gchar* key
	, CppMatched cb
	, gpointer cb_tag
	, CppFile* file
	, gint line )
{
	
}

