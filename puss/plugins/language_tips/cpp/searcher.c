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

static SNode* find_sub_node(SNode* parent, const TinyStr* key) {
	return parent->sub ? g_hash_table_lookup(parent->sub, key) : 0;
}

static SNode* make_sub_node(SNode* parent, const TinyStr* key) {
	TinyStr* skey = 0;
	SNode* snode = 0;

	if( !parent->sub )
		parent->sub = g_hash_table_new_full(tiny_str_hash, tiny_str_equal, tiny_str_free, (GDestroyNotify)snode_free);

	if( parent->sub ) {
		snode = g_hash_table_lookup(parent->sub, key);
		if( !snode ) {
			skey = tiny_str_copy(key);
			snode = snode_new();
			if( skey && snode ) {
				g_hash_table_insert(parent->sub, skey, snode);
			} else {
				tiny_str_free(skey);
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
	gchar len_hi, len_lo, str_end;
	TinyStr* str;

	if( !nskey )
		return parent;

	snode = parent;
	ps = nskey->buf;

	while( *ps && snode ) {
		pe = ps + 1;
		while(*pe && *pe!='.' )
			++pe;

		str = (TinyStr*)(ps-2);
		len_hi = str->len_hi;
		len_lo = str->len_lo;
		str_end = *pe;
		str->len_hi = (gchar)((pe-ps) >> 8);
		str->len_lo = (gchar)((pe-ps) & 0xff);
		*pe = '\0';

		snode = make_if_not_exist
			? make_sub_node(parent, str)
			: find_sub_node(parent, str);

		str->len_hi = len_hi;
		str->len_lo = len_lo;
		*pe = str_end;

		ps = pe;
	}

	return snode;
}

static SNode* cpp_snode_sub_insert(SNode* parent, TinyStr* nskey, CppElem* elem) {
	SNode* snode = 0;
	SNode* scope_snode;

	scope_snode = snode_locate(parent, nskey, TRUE);
	if( scope_snode ) {
		snode = make_sub_node(scope_snode, elem->name);
		snode->elems = g_list_append(snode->elems, elem);
	}
	return snode;
}

static SNode* cpp_snode_sub_remove(SNode* parent, TinyStr* nskey, CppElem* elem) {
	SNode* snode;
	SNode* scope_snode;

	scope_snode = snode_locate(parent, nskey, FALSE);
	if( scope_snode ) {
		snode = find_sub_node(scope_snode, elem->name);
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


typedef struct {
	gchar   type;
	TinyStr value;
} SKey;

#define skey_len(skey) tiny_str_len(&((skey)->value))

#define skey_mem_size(skey) (sizeof(SKey) + skey_len(skey))

static SKey* skey_new(gchar type, const gchar* buf, gsize len) {
	SKey* skey;

	if( len > 0x0000ffff )
		len = 0x0000ffff;

	skey = (SKey*)g_slice_alloc( sizeof(SKey) + len );
	skey->type = type;
	skey->value.len_hi = (gchar)(len >> 8);
	skey->value.len_lo = (gchar)(len & 0xff);
	if( buf )
		memcpy(skey->value.buf, buf, len);
	skey->value.buf[len] = '\0';

	return skey;
}

static SKey* skey_tiny_str_new(gchar type, const TinyStr* str) {
	gsize str_len = tiny_str_len(str);
	SKey* skey = (SKey*)g_slice_alloc( sizeof(SKey) + str_len );
	skey->type = type;
	memcpy( &(skey->value), str, sizeof(TinyStr) + str_len );
	return skey;
}

static inline void skey_free(SKey* skey) {
	if( skey )
		g_slice_free1( skey_mem_size(skey), skey );
}

#define skey_copy(skey) (SKey*)g_slice_copy(skey_mem_size(skey), skey)

#define iter_prev(env, it) env->do_prev(it)
#define iter_next(env, it) env->do_next(it)

inline gchar iter_prev_char(SearchIterEnv* env, gpointer it) {
	gchar ch = env->do_prev(it);
	if( ch )
		env->do_next(it);
	return ch;
}

inline gchar iter_next_char(SearchIterEnv* env, gpointer it) {
	gchar ch = env->do_next(it);
	if( ch )
		env->do_prev(it);
	return ch;
}

void iter_skip_pair(SearchIterEnv* env, gpointer it, gchar sch, gchar ech) {
	gchar ch = '\0';
	gint layer = 1;
	while( layer > 0 ) {
		ch = iter_prev(env, it);
		switch( ch ) {
		case '\0':
		case ';':
		case '{':
		case '}':
			return;
		case ':':
			if( iter_prev_char(env, it)!=':' )
				return;
			break;
		default:
			if( ch==ech )
				++layer;
			else if( ch==sch )
				--layer;
			break;
		}	
	}
}

GList* cpp_spath_find(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith) {
	gchar ch;
	gboolean loop_sign;
	GList* spath = 0;
	GString* buf = g_string_sized_new(512);

	ch = iter_prev(env, ps);

	if( find_startswith ) {
		switch( ch ) {
		case '\0':
			goto find_error;

		case '.':
			if( iter_prev_char(env, ps)=='.' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			ch = iter_prev(env, ps);
			break;

		case '>':
			if( iter_prev(env, ps)!='-' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			ch = iter_prev(env, ps);
			break;

		case '(':
		case '<':
			break;

		case ':':
			if( iter_prev(env, ps)!=':' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			ch = iter_prev(env, ps);
			break;

		default:
			while( ch=='_' || g_ascii_isalnum(ch) ) {
				g_string_append_c(buf, ch);
				ch = iter_prev(env, ps);
			}

			if( buf->len==0 )
				goto find_error;

			spath = g_list_prepend(spath, skey_new('S', buf->str, buf->len));
			g_string_assign(buf, "");
		}
	}

	loop_sign = TRUE;
	while( loop_sign && ((ch=iter_prev(env, ps)) != '\0') ) {
		switch( ch ) {
		case '.':
			if( buf->len==0 )
				return FALSE;
			spath = g_list_prepend(spath, skey_new('S', buf->str, buf->len));
			g_string_append_c(buf, ch);
			break;

		case ':':
			if( buf->len==0 )
				return FALSE;

			if( iter_prev_char(env, ps)==':' ) {
				iter_prev(env, ps);
				spath = g_list_prepend(spath, skey_new('?', buf->str, buf->len));
				g_string_append_c(buf, ch);
			} else {
				loop_sign = FALSE;
			}
			break;

		case ']':
			iter_skip_pair(env, ps, '[', ']');
			spath = g_list_prepend(spath, skey_new('v', buf->str, buf->len));
			g_string_append_c(buf, ch);
			break;
			
		case ')':
			iter_skip_pair(env, ps, '(', ')');
			spath = g_list_prepend(spath, skey_new('f', buf->str, buf->len));
			g_string_append_c(buf, ch);
			break;
			
		case '>':
			if( buf->len==0 ) {
				if( iter_prev_char(env, ps) != '>' ) {
					iter_skip_pair(env, ps, '<', '>');
				} else {
					loop_sign = FALSE;
				}

			} else {
				if( iter_prev_char(env, ps)=='-' ) {
					iter_prev(env, ps);
					spath = g_list_prepend(spath, skey_new('v', buf->str, buf->len));
					g_string_append_c(buf, ch);
					
				} else {
					loop_sign = FALSE;
				}
			}
			break;
			
		default:
			if( ch=='_' || g_ascii_isalnum(ch) ) {
				do {
					g_string_append_c(buf, ch);
					ch = iter_prev(env, ps);
				} while( ch=='_' || g_ascii_isalnum(ch) );
			} else {
				loop_sign = FALSE;
			}
		}
	}

	iter_next(env, ps);
	goto find_finish;

find_error:
	g_list_free(spath);
	spath = 0;

find_finish:
	g_string_free(buf, TRUE);
	return spath;
}

typedef struct {
	const gchar* start;
	const gchar* end;
	gchar* cur;
} ParseKeyIter;

static gchar parse_key_do_prev(ParseKeyIter* pos) {
	return (pos->cur > pos->start) ? *(--(pos->cur)) : '\0';
}

static gchar parse_key_do_next(ParseKeyIter* pos) {
	return (pos->cur < pos->end) ? *(++(pos->cur)) : '\0';
}

GList* cpp_spath_parse(const gchar* text, gboolean find_startswith) {
	gint len = strlen(text);
	ParseKeyIter ps = { text, text+len, text+len };
	ParseKeyIter pe = { text, text+len, text+len };
	SearchIterEnv env = { parse_key_do_prev, parse_key_do_next };

	return cpp_spath_find(&env, &ps, &pe, find_startswith);
}

void cpp_spath_free(GList* spath) {
	g_list_foreach(spath, skey_free, 0);
	g_list_free(spath);
}

static gboolean spath_equal(GList* a, GList* b) {
	SKey* pa;
	SKey* pb;

	if( a==b )
		return TRUE;

	while( a && b ) {
		pa = (SKey*)(a->data);
		pb = (SKey*)(b->data);
		a = a->next;
		b = b->next;

		if( pa!=pb && (pa->type!=pb->type || !tiny_str_equal( &(pa->value), &(pb->value))) )
			break;
	}

	return a==b;
}

struct _Searcher {
	CppSTree*  stree;
	GTree*     worked_nodes;
	GList*     spaths;
	CppMatched cb;
	gpointer   cb_tag;
};

static void searcher_add_spath(Searcher* searcher, GList* spath) {
	searcher->spaths = g_list_append(searcher->spaths, spath);
}

static GList* do_parse_nskey_to_spath(const TinyStr* nskey, gchar mtype, gchar etype) {
	GList* spath = 0;
	gchar* ps = nskey->buf;
	gchar* pe = 0;

	if( *ps=='.' ) {
		spath = g_list_append(spath, skey_new('R',  "", 0));
		++ps;
	}

	while( *ps ) {
		pe = ps;
		while( *pe && *pe!='.' )
			++pe;

		spath = g_list_append(spath, skey_new(mtype,  ps, pe-ps));
		if( *pe )
			++pe;

		ps = pe;
	}

	return spath;
}

#define parse_typekey_to_spath(s)	do_parse_nskey_to_spath((s), '?', 't')
#define parse_nskey_to_spath(s)		do_parse_nskey_to_spath((s), 'n', 'n')

// types:
//    n : namespace
//    ? : namespace or type(class or enum or typedef)
//    t : type
//    f : function return type
//    v : var datatype
//    R : root node
//    L : this
//    S : search startswith
//    
gboolean follow_type_check(gchar* last, gchar cur) {
	switch(*last) {
	case 't':
		switch(cur) {
		case 'n':	return FALSE;
		case '?':	*last = 't';	return TRUE;
		}
		break;
	case 'S':
		return FALSE;
	}

	*last = cur;
	return TRUE;
}

// replace spath :
//		| start ... ps ... pe ... end |
//   block(ps .. pe)
// with rep
//      | start ... rep pe ... end |
// 
static GList* spath_replace_new(GList* start, GList* ps, GList* pe, GList* rep) {
	GList* p;
	GList* spath = 0;
	gchar last = '?';
	gboolean ok = TRUE;
	SKey* skey;

	for( p=start; ok && p!=ps; p=p->next ) {
		skey = (SKey*)(p->data);
		ok = follow_type_check(&last, skey->type);
		if( ok ) {
			skey = skey_copy(skey);
			skey->type = last;
			spath = g_list_append(spath, skey);
		}
	}

	for( p=rep; ok && p; p=p->next ) {
		skey = (SKey*)(p->data);
		ok = follow_type_check(&last, skey->type);
		if( ok ) {
			skey = skey_copy(skey);
			spath = g_list_append(spath, skey);
			skey->type = last;
		}
	}

	for( p=pe; ok && p; p=p->next ) {
		skey = (SKey*)(p->data);
		ok = follow_type_check(&last, skey->type);
		if( ok ) {
			skey = skey_copy(skey);
			spath = g_list_append(spath, skey);
			skey->type = last;
		}
	}

	if( !ok ) {
		cpp_spath_free(spath);
		spath = 0;
	}

	return spath;
}

static void loop_insert(Searcher* searcher, GList* spath, GList* pos, GList* rep) {
	GList* ps = ((SKey*)(rep->data))->type=='R' ? spath : pos;
	GList* pe = pos->next;
	GList* p;
	GList* new_spath;

	for( p=spath; p!=ps; p=p->next ) {
		new_spath = spath_replace_new(p, ps, pe, rep);
		if( new_spath )
			searcher_add_spath(searcher, new_spath);
	}
}

static void loop_insert_typekey(Searcher* searcher, GList* spath, GList* pos, TinyStr* typekey) {
	GList* rep;
	if( typekey && tiny_str_len(typekey) > 0 ) {
		rep = parse_typekey_to_spath(typekey);
		if( rep ) {
			loop_insert(searcher, spath, pos, rep);
			cpp_spath_free(rep);
		}
	}
}

static void loop_insert_nskey(Searcher* searcher, GList* spath, GList* pos, TinyStr* nskey) {
	GList* rep;
	if( nskey && tiny_str_len(nskey) > 0 ) {
		rep = parse_nskey_to_spath(nskey);
		if( rep ) {
			loop_insert(searcher, spath, pos, rep);
			cpp_spath_free(rep);
		}
	}
}

static void searcher_do_walk(Searcher* searcher, SNode* node, GList* spath, GList* pos) {
	SKey* cur;
	gchar cur_type;
	TinyStr* cur_key;
	gint cur_key_len;
	GList* ps;
	GList* pe;
	GHashTableIter it;
	TinyStr* key;
	SNode* value;
	SNode* sub_node;
	CppElem* elem;
	TinyStr** pts;

	if( !node || !node->elems )
		return;

	cur = ((SKey*)(pos->data));
	
	if( cur->type=='L' )
		return;

	if( cur->type=='R' ) {
		if( !pos->next )
			return;
		pos = pos->next;
		cur = ((SKey*)(pos->data));
	}

	if( g_tree_lookup(searcher->worked_nodes, node)==0 ) {
		g_tree_insert(searcher->worked_nodes, node, node);

		if( cur->type=='?' || cur->type=='n' ) {
			/*
			if( !node.usings.empty() ) {
				cpp::Usings::iterator ps = node.usings.begin ();
				cpp::Usings::iterator pe = node.usings.end();
				for( ; ps != pe; ++ps ) {
					cpp::Using& elem = **ps;
					loop_insert_nskey(paths_, path, elem.nskey);
				}
			}
			*/
		}
    }

	cur_type = cur->type;
	cur_key = &(cur->value);
	cur_key_len = tiny_str_len(cur_key);

	if( cur_type=='S' ) {
		// search start with
		if( pos->next )
			return;	// bad logic

		g_hash_table_iter_init(&it, node->sub);
		while( g_hash_table_iter_next(&it, &key, &value) ) {
			if( tiny_str_len(key) < cur_key_len )
				continue;

			if( g_str_has_prefix(key->buf, cur_key->buf) )
				for(ps=value->elems; ps; ps=ps->next )
					(*(searcher->cb))( (CppElem*)(ps->data), searcher->cb_tag );
		}

		return;
	}

	sub_node = (node->sub) ? g_hash_table_lookup(node->sub, cur_key) : 0;
	if( !sub_node )
		return;

	for( ps=sub_node->elems; ps; ps=ps->next ) {
		elem = ((CppElem*)(ps->data));
		if( !pos->next ) {
			if( elem->type==CPP_ET_USING ) {
				switch( cur_type ) {
				case '?':
				case 't':
				case '*':
					loop_insert_typekey(searcher, spath, pos, elem->name);
					break;
				}
				break;
			}

			(*(searcher->cb))( elem, searcher->cb_tag );
			continue;
		}

		gboolean next_sign = FALSE;

		switch( elem->type ) {
		case CPP_ET_NCSCOPE:
			if( cur_type=='?' || cur_type=='t' || cur_type=='n' )
				next_sign = TRUE;
			break;
			
		case CPP_ET_CLASS:
			if( cur_type=='?' || cur_type=='t' ) {
				cur->type = 't'; 
				for( pts=elem->v_class.inhers; *pts; ++pts )
					loop_insert_typekey(searcher, spath, pos, *pts);
				cur->type = cur_type;

				next_sign = TRUE;
			}
			break;
			
		case CPP_ET_ENUM:
			if( cur_type=='?' || cur_type=='t' )
				next_sign = TRUE;
			break;
			
		case CPP_ET_NAMESPACE:
			if( cur_type=='?' || cur_type=='n' )
				next_sign = TRUE;
			break;
			
		case CPP_ET_TYPEDEF:
			if( cur_type=='?' || cur_type=='t' )
				loop_insert_typekey(searcher, spath, pos, elem->v_typedef.typekey);
			break;
			
		case CPP_ET_FUN:
			if( cur_type=='f' )
				loop_insert_typekey(searcher, spath, pos, elem->v_fun.typekey);
			break;
			
		case CPP_ET_VAR:
			if( cur_type=='v' )
				loop_insert_typekey(searcher, spath, pos, elem->v_var.typekey);
			break;
			
		case CPP_ET_USING:
			if( cur_type=='?' || cur_type=='t' )
				loop_insert_nskey(searcher, spath, pos, elem->v_using.nskey);
			break;
		}

		if( next_sign ) {
			pos = pos->next;
			if( pos ) {
				if( elem->type==CPP_ET_CLASS && ((SKey*)(pos->data))->type=='L' && pos->next )
					pos = pos->next;

				searcher_do_walk(searcher, sub_node, spath, pos);
			}
		}
	}
}

static void searcher_walk(Searcher* searcher, GList* spath) {
	if( g_list_length(spath) > 8 )
		return;

	searcher_do_walk(searcher, searcher->stree->root, spath, spath);
}

static gboolean searcher_do_locate(Searcher* searcher, GList* scope, gint line, GList* spath, GList* pos, gboolean need_walk) {
	GList* p;
	CppElem* elem;
	SKey* cur;
	GList* rep;
	GList* new_spath;
	GList* ps;
	GList* pe;

	if( !pos )
		return need_walk;

	cur = (SKey*)(pos->data);

	for( p=scope; p; p=p->next ) {
		elem = (CppElem*)(p->data);
		if( line < elem->sline )
			break;
		if(	line > elem->eline )
			continue;

		switch( elem->type ) {
		case CPP_ET_FUN:
			{
				if( cur->type!='L' ) {
					SNode* impl_root = snode_new();
					cpp_snode_insert_list(impl_root, elem->v_ncscope.scope);
					searcher_do_walk(searcher, impl_root, spath, pos);
					snode_free(impl_root);
				}

				if( elem->v_fun.nskey ) {
					rep = parse_typekey_to_spath(elem->v_fun.typekey);
					if( rep ) {
						new_spath = spath_replace_new(spath, ((SKey*)(rep->data))->type=='R' ? spath : pos, pos, rep);
						cpp_spath_free(rep);

						searcher_add_spath(searcher, new_spath);
					}
				}
			}
			break;

		case CPP_ET_CLASS:
			{
				if( !elem->v_ncscope.scope )
					break;

				rep = g_list_append(0, skey_tiny_str_new('t', elem->name));
				if( rep ) {
					new_spath = spath_replace_new(spath, pos, pos, rep);
					cpp_spath_free(rep);

					rep = new_spath;
					for( ps=spath; ps!=pos; ps=ps->next )
						rep=rep->next;
					rep=rep->next;

					need_walk = searcher_do_locate(searcher, elem->v_ncscope.scope, line, new_spath, rep, need_walk);
					if( need_walk ) {
						if( cur->type=='L' )
							need_walk = FALSE;
						searcher_add_spath(searcher, new_spath);
					}
				}
			}
			break;

		case CPP_ET_NAMESPACE:
			{
				if( !elem->v_ncscope.scope )
					break;

				rep = g_list_append(0, skey_tiny_str_new('n', elem->name));
				if( rep ) {
					new_spath = spath_replace_new(spath, pos, pos, rep);
					cpp_spath_free(rep);

					rep = new_spath;
					for( ps=spath; ps!=pos; ps=ps->next )
						rep=rep->next;
					rep=rep->next;

					need_walk = searcher_do_locate(searcher, elem->v_ncscope.scope, line, new_spath, rep, need_walk);
					if( need_walk )
						searcher_add_spath(searcher, new_spath);
				}
			}
			break;
		}
		break;	// only find first CPP_Element
	}

	return need_walk;
}

static void searcher_locate(Searcher* searcher, CppFile* file, gint line, GList* spath) {
	gboolean need_walk = TRUE;
	if( file && line && spath && ((SKey*)(spath->data))->type!='R' )
		need_walk = searcher_do_locate(searcher, file, line, spath, spath, need_walk);

	if( need_walk )
		searcher_add_spath(searcher, spath);
}

static void searcher_start(Searcher* searcher, GList* spath, CppFile* file, gint line) {
	GList* p;
	GList* worked;

	g_static_rw_lock_reader_lock( &(searcher->stree->lock) );

	searcher_locate(searcher, file, line, spath);

	for( p=searcher->spaths; p; p=p->next ) {
		spath = (GList*)(p->data);

		for( worked=searcher->spaths; worked!=p; worked=worked->next )
			if( spath_equal((GList*)(worked->data), spath) )
				break;

		if( worked==p )
			searcher_walk(searcher, spath);
	}

	g_static_rw_lock_reader_unlock( &(searcher->stree->lock) );
}

void cpp_search( CppSTree* stree
	, GList* spath
	, CppMatched cb
	, gpointer cb_tag
	, CppFile* file
	, gint line )
{
	GList* p;
	Searcher s;

	if( !spath )
		return;

	s.stree = stree;
	s.worked_nodes = g_tree_new(g_direct_equal);
	s.spaths = 0;
	s.cb = cb;
	s.cb_tag = cb_tag;

	searcher_start(&s, spath, file, line);

	for( p=s.spaths; p; p=p->next ) {
		if( p->data!=(gpointer)spath )
			cpp_spath_free(p);
	}

	g_tree_destroy(s.worked_nodes);
}

