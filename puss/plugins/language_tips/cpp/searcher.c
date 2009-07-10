// searcher.c
// 

#include "searcher.h"

// c++ keywords & macro keywords
// 
static const char* CPP_KEYWORDS[] = {
	  "_Bool"
	, "_Complex"
	, "_Imaginary"
	, "asm"
	, "auto"
	, "bool"
	, "break"
	, "case"
	, "catch"
	, "char"
	, "class"
	, "const"
	, "const_cast"
	, "continue"
	, "default"
	, "define"			// macro
	, "delete"
	, "do"
	, "double"
	, "dynamic_cast"
	, "else"
	, "endif"			// macro
	, "enum"
	, "explicit"
	, "export"
	, "extern"
	, "false"
	, "float"
	, "for"
	, "friend"
	, "goto"
	, "if"
	, "ifdef"			// macro
	, "ifndef"			// macro
	, "include"			// macro
	, "inline"
	, "int"
	, "long"
	, "mutable"
	, "namespace"
	, "new"
	, "operator"
	, "private"
	, "protected"
	, "public"
	, "register"
	, "reinterpret_cast"
	, "return"
	, "short"
	, "signed"
	, "sizeof"
	, "static"
	, "static_cast"
	, "struct"
	, "switch"
	, "template"
	, "this"
	, "throw"
	, "true"
	, "try"
	, "typedef"
	, "typeid"
	, "typename"
	, "union"
	, "unsigned"
	, "using"
	, "virtual"
	, "void"
	, "volatile"
	, "wchar_t"
	, "while"
};

struct _SNode {
	GList*		elems;
	GHashTable*	sub;
};

static void __cpp_elem_dump(CppElem* elem) {
	g_print("elem : name=%s\n", elem->name->buf);
}

static void __snode_dump(SNode* snode) {
	g_print("==snode : elems======================\n");
	g_list_foreach(snode->elems, __cpp_elem_dump, 0);
	g_print("--snode : subs-----------------------\n");
	if( snode->sub ) {
		GHashTableIter iter;
		TinyStr* sub_key = 0;
		SNode* sub_node = 0;
		g_hash_table_iter_init(&iter, snode->sub);
		while( g_hash_table_iter_next(&iter, &sub_key, &sub_node) )
			g_print("subnode : name=%s\n", sub_key->buf);
	}
}

static inline SNode* snode_new() {
	SNode* res;
	//res = g_slice_new0(SNode);
	cpp_init_new(res, SNode, 1);

	return res;
}

static void snode_free(SNode* snode) {
	if( snode ) {
		if( snode->elems )
			g_list_free(snode->elems);
		if( snode->sub )
			g_hash_table_destroy(snode->sub);
		//g_slice_free(SNode, snode);
		cpp_free(snode);
	}
}

static inline SNode* find_sub_node(SNode* parent, const TinyStr* key) {
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
				snode = 0;
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

static SNode* snode_sub_insert(SNode* parent, const TinyStr* nskey, CppElem* elem) {
	SNode* snode = 0;
	SNode* scope_snode;

	scope_snode = snode_locate(parent, nskey, TRUE);
	if( scope_snode ) {
		snode = make_sub_node(scope_snode, elem->name);
		snode->elems = g_list_append(snode->elems, elem);
	}
	return snode;
}

static SNode* snode_sub_remove(SNode* parent, TinyStr* nskey, CppElem* elem) {
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

static void snode_insert(SNode* parent, CppElem* elem);
static void snode_remove(SNode* node, CppElem* elem);

static void snode_insert_list(SNode* parent, GList* elems) {
	GList* p;

	for( p=elems; p; p=p->next )
		snode_insert(parent, (CppElem*)(p->data));
}

static void snode_remove_list(SNode* parent, GList* elems) {
	GList* p;

	for( p=elems; p; p=p->next )
		snode_remove(parent, (CppElem*)(p->data));
}

static void snode_insert(SNode* parent, CppElem* elem) {
	switch( elem->type ) {
	case CPP_ET_NCSCOPE:
		snode_insert_list(parent, elem->v_ncscope.scope);
		break;
		
	case CPP_ET_VAR:
		snode_sub_insert(parent, elem->v_var.nskey, elem);
		break;
		
	case CPP_ET_FUN:
		snode_sub_insert(parent, elem->v_fun.nskey, elem);
		break;
		
	case CPP_ET_KEYWORD:
	case CPP_ET_MACRO:
	case CPP_ET_TYPEDEF:
	case CPP_ET_ENUMITEM:
		snode_sub_insert(parent, 0, elem);
		break;
		
	case CPP_ET_ENUM:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = snode_sub_insert(parent, elem->v_enum.nskey, elem);
				if( snode )
					snode_insert_list(snode, elem->v_ncscope.scope);
			}
			snode_insert_list(parent, elem->v_ncscope.scope);
		}
		break;
		
	case CPP_ET_CLASS:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = snode_sub_insert(parent, elem->v_class.nskey, elem);
				if( snode )
					snode_insert_list(snode, elem->v_ncscope.scope);
					
			} else if( elem->v_class.class_type==CPP_CLASS_TYPE_UNION ) {
				snode_insert_list(parent, elem->v_ncscope.scope);
			}
		}
		break;
		
	case CPP_ET_USING:
		{
			if( elem->v_using.isns )
				; // scope.usings.push_back( (Using*)elem );
			else
				snode_sub_insert(parent, 0, elem);
		}
		break;
		
	case CPP_ET_NAMESPACE:
		{
			if( elem->name->buf[0]!='@' )
				parent = snode_sub_insert(parent, 0, elem);

			snode_insert_list(parent, elem->v_ncscope.scope);
		}
		break;
	}
}

static void snode_remove(SNode* parent, CppElem* elem) {
	switch( elem->type ) {
	case CPP_ET_NCSCOPE:
		snode_remove_list(parent, elem->v_ncscope.scope);
		break;
		
	case CPP_ET_VAR:
		snode_sub_remove(parent, elem->v_var.nskey, elem);
		break;
		
	case CPP_ET_FUN:
		snode_sub_remove(parent, elem->v_fun.nskey, elem);
		break;
		
	case CPP_ET_KEYWORD:
	case CPP_ET_MACRO:
	case CPP_ET_TYPEDEF:
	case CPP_ET_ENUMITEM:
		snode_sub_remove(parent, 0, elem);
		break;
		
	case CPP_ET_ENUM:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = snode_sub_remove(parent, elem->v_enum.nskey, elem);
				if( snode )
					snode_remove_list(snode, elem->v_ncscope.scope);
			}
			snode_remove_list(parent, elem->v_ncscope.scope);
		}
		break;
		
	case CPP_ET_CLASS:
		{
			if( elem->name->buf[0]!='@' ) {
				SNode* snode = snode_sub_remove(parent, elem->v_class.nskey, elem);
				if( snode )
					snode_remove_list(snode, elem->v_ncscope.scope);
					
			} else if( elem->v_class.class_type==CPP_CLASS_TYPE_UNION ) {
				snode_remove_list(parent, elem->v_ncscope.scope);
			}
		}
		break;
		
	case CPP_ET_USING:
		{
			if( elem->v_using.isns )
				; // scope.usings.push_back( (Using*)elem );
			else
				snode_sub_remove(parent, 0, elem);
		}
		break;
		
	case CPP_ET_NAMESPACE:
		{
			if( elem->name->buf[0]!='@' )
				parent = snode_sub_remove(parent, 0, elem);

			snode_remove_list(parent, elem->v_ncscope.scope);
		}
		break;
	}
}

void stree_init(CppSTree* self) {
	gint i;
	CppElem* elem;

	self->root = snode_new();

	self->keywords_file.ref_count = 1;
	self->keywords_file.filename = tiny_str_new("@keywords@", 10);
	self->keywords_file.root_scope.type = CPP_ET_NCSCOPE;
	self->keywords_file.root_scope.file = &self->keywords_file;

	for( i=0; i<sizeof(CPP_KEYWORDS)/sizeof(gpointer); ++i ) {
		elem = cpp_elem_new();
		elem->type = CPP_ET_KEYWORD;
		elem->file = &(self->keywords_file);
		elem->name = tiny_str_new(CPP_KEYWORDS[i], strlen(CPP_KEYWORDS[i]));
		elem->decl = tiny_str_copy(elem->name);
		cpp_scope_insert(&(self->keywords_file.root_scope), elem);
	}

	stree_insert(self, &(self->keywords_file));

	g_static_rw_lock_init( &(self->lock) );
}

void stree_final(CppSTree* self) {
	snode_free(self->root);
	self->root = 0;

	cpp_file_clear(&(self->keywords_file));

	g_static_rw_lock_free( &(self->lock) );
}

void stree_insert(CppSTree* self, CppFile* file) {
	g_static_rw_lock_writer_lock( &(self->lock) );
	snode_insert( self->root, &(file->root_scope) );
	g_static_rw_lock_writer_unlock( &(self->lock) );
}

void stree_remove(CppSTree* self, CppFile* file) {
	g_static_rw_lock_writer_lock( &(self->lock) );
	snode_remove( self->root, &(file->root_scope) );
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

	//skey = (SKey*)g_slice_alloc( sizeof(SKey) + len );
	skey = cpp_malloc( sizeof(SKey) + len );
	skey->type = type;
	skey->value.len_hi = (gchar)(len >> 8);
	skey->value.len_lo = (gchar)(len & 0xff);
	if( buf )
		memcpy(skey->value.buf, buf, len);
	skey->value.buf[len] = '\0';

	return skey;
}

static SKey* skey_tiny_str_new(gchar type, const TinyStr* str) {
	gsize str_len;
	SKey* skey;

	str_len = tiny_str_len(str);
	//skey= (SKey*)g_slice_alloc( sizeof(SKey) + str_len );
	skey = cpp_malloc( sizeof(SKey) + str_len );

	skey->type = type;
	memcpy( &(skey->value), str, sizeof(TinyStr) + str_len );
	return skey;
}

static inline void skey_free(SKey* skey) {
	if( skey ) {
		//g_slice_free1( skey_mem_size(skey), skey );
		cpp_free(skey);
	}
}

//#define skey_copy(skey) (SKey*)g_slice_copy(skey_mem_size(skey), skey)
static inline SKey* skey_copy(SKey* src) {
	SKey* dst;
	dst = (SKey*)cpp_malloc( skey_mem_size(src) );
	memcpy(dst, src, skey_mem_size(src));

	return dst;
}

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

gboolean iter_skip_pair(SearchIterEnv* env, gpointer it, gchar sch, gchar ech) {
	gchar ch = '\0';
	gint layer = 1;
	while( layer > 0 ) {
		ch = iter_prev(env, it);
		switch( ch ) {
		case '\0':
		case ';':
		case '{':
		case '}':
			return FALSE;
		case ':':
			if( iter_prev_char(env, it)!=':' )
				return FALSE;
			break;
		default:
			if( ch==ech )
				++layer;
			else if( ch==sch )
				--layer;
			break;
		}
	}

	return TRUE;
}

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
GList* spath_find(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith) {
	gchar ch;
	gchar type;
	gboolean loop_sign;
	GList* spath = 0;
	GString* buf = g_string_sized_new(512);

	type = '*';

	if( find_startswith ) {
		ch = iter_next(env, ps);
		ch = iter_prev(env, ps);

		switch( ch ) {
		case '\0':
			goto find_error;

		case '.':
			if( iter_prev_char(env, ps)=='.' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			type = 'v';
			break;

		case '>':
			if( iter_prev(env, ps)!='-' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			type = 'v';
			break;

		case '(':
		case '<':
			break;

		case ':':
			if( iter_prev(env, ps)!=':' )
				goto find_error;
			spath = g_list_prepend(spath, skey_new('S', "", 0));
			type = '?';
			break;

		default:
			while( ch=='_' || g_ascii_isalnum(ch) ) {
				g_string_append_c(buf, ch);
				ch = iter_prev(env, ps);
			}

			if( buf->len==0 )
				goto find_error;

			spath = g_list_prepend(spath, skey_new('S', g_strreverse(buf->str), buf->len));
			g_string_assign(buf, "");
			iter_next(env, ps);
		}
	}

	loop_sign = TRUE;
	while( loop_sign && ((ch=iter_prev(env, ps)) != '\0') ) {
		switch( ch ) {
		case '.':
			if( buf->len ) {
				spath = g_list_prepend(spath, skey_new(type, g_strreverse(buf->str), buf->len));
				g_string_assign(buf, "");
			}

			type = 'v';
			break;

		case ':':
			if( iter_prev_char(env, ps)==':' ) {
				iter_prev(env, ps);
				if( buf->len > 0 ) {
					spath = g_list_prepend(spath, skey_new(type, g_strreverse(buf->str), buf->len));
					g_string_assign(buf, "");
				}
				type = '?';
			} else {
				loop_sign = FALSE;
			}
			break;

		case ']':
			if( buf->len > 0 ) {
				spath = g_list_prepend(spath, skey_new(type, g_strreverse(buf->str), buf->len));
				g_string_assign(buf, "");
				loop_sign = FALSE;
				break;
			}

			if( !iter_skip_pair(env, ps, '[', ']') ) {
				loop_sign = FALSE;
				break;
			}

			type = 'v';
			break;
			
		case ')':
			if( buf->len > 0 ) {
				spath = g_list_prepend(spath, skey_new(type, g_strreverse(buf->str), buf->len));
				g_string_assign(buf, "");
				loop_sign = FALSE;
				break;
			}

			if( !iter_skip_pair(env, ps, '(', ')') ) {
				loop_sign = FALSE;
				break;
			}

			type = 'f';
			break;
			
		case '>':
			if( iter_prev_char(env, ps)=='-' ) {
				iter_prev(env, ps);
				if( buf->len > 0 ) {
					spath = g_list_prepend(spath, skey_new(type, g_strreverse(buf->str), buf->len));
					g_string_assign(buf, "");
				}
				type = 'v';
				break;	
			}

			if( iter_skip_pair(env, ps, '<', '>') ) {
				type = 't';
				break;
			}

			loop_sign = FALSE;
			break;
			
		default:
			if( ch=='_' || g_ascii_isalnum(ch) ) {
				do {
					g_string_append_c(buf, ch);
					ch = iter_prev(env, ps);
				} while( ch=='_' || g_ascii_isalnum(ch) );

				if( ch )
					iter_next(env, ps);

			} else {
				loop_sign = FALSE;
			}
		}
	}

	if( buf->len > 0 ) {
		spath = g_list_prepend(spath, skey_new(type, g_strreverse(buf->str), buf->len));
	}

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

GList* spath_parse(const gchar* text, gboolean find_startswith) {
	gint len = strlen(text);
	ParseKeyIter ps = { text, text+len, text+len };
	ParseKeyIter pe = { text, text+len, text+len };
	SearchIterEnv env = { parse_key_do_prev, parse_key_do_next };

	return spath_find(&env, &ps, &pe, find_startswith);
}

void spath_free(GList* spath) {
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

#ifdef _DEBUG
	static void spath_trace(GList* spath) {
		g_printerr("WALK ");
		while( spath ) {
			SKey* skey = (SKey*)(spath->data);
			g_printerr(" %c:%s", skey->type, skey->value.buf);
			spath = spath->next;
		}
		g_printerr("\n");
	}
#else
	#define spath_trace(spath)
#endif

typedef struct {
	CppSTree*	stree;
	GTree*		worked_nodes;
	GList*		spaths;
	CppMatched	cb;
	gpointer	cb_tag;
	gboolean	run_sign;
	gint		limit_num;
	gint		limit_time;
	GTimeVal	start_time;
} Searcher;

static inline void search_call_cb(Searcher* searcher, CppElem* elem) {
	#ifdef _DEBUG
		g_printerr("\tfind : %s\n", elem->name->buf);
	#endif

	if( (*(searcher->cb))( elem, searcher->cb_tag ) ) {
		if( searcher->limit_num > 0 ) {
			--searcher->limit_num;
			if( searcher->limit_num==0 )
				searcher->run_sign = FALSE;
		}
	}	
}

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
		spath_free(spath);
		spath = 0;
	}

	return spath;
}

static void loop_insert(Searcher* searcher, GList* spath, GList* pos, GList* rep) {
	GList* ps = ((SKey*)(rep->data))->type=='R' ? pos : spath;
	GList* pe = pos->next;
	GList* p;
	GList* new_spath;

	if( spath==ps ) {
		new_spath = spath_replace_new(ps, ps, pe, rep);
		if( new_spath )
			searcher_add_spath(searcher, new_spath);

	} else {
		for( p=spath; p!=ps; p=p->next ) {
			new_spath = spath_replace_new(p, ps, pe, rep);
			if( new_spath )
				searcher_add_spath(searcher, new_spath);
		}
	}
}

static void loop_insert_typekey(Searcher* searcher, GList* spath, GList* pos, TinyStr* typekey) {
	GList* rep;
	if( typekey && tiny_str_len(typekey) > 0 ) {
		rep = parse_typekey_to_spath(typekey);
		if( rep ) {
			loop_insert(searcher, spath, pos, rep);
			spath_free(rep);
		}
	}
}

static void loop_insert_nskey(Searcher* searcher, GList* spath, GList* pos, TinyStr* nskey) {
	GList* rep;
	if( nskey && tiny_str_len(nskey) > 0 ) {
		rep = parse_nskey_to_spath(nskey);
		if( rep ) {
			loop_insert(searcher, spath, pos, rep);
			spath_free(rep);
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
	gboolean next_sign;
	gint i;
	GTimeVal tm;

	if( !node )
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

		if( !node->sub )
			return;

		g_hash_table_iter_init(&it, node->sub);
		while( g_hash_table_iter_next(&it, &key, &value) ) {
			if( tiny_str_len(key) < cur_key_len )
				continue;

			if( g_str_has_prefix(key->buf, cur_key->buf) ) {
				for(ps=value->elems; searcher->run_sign && ps; ps=ps->next ) {
					search_call_cb(searcher, (CppElem*)(ps->data));

					// use limit_num enought
					// 
					// g_get_current_time(&tm);
					// if( searcher->limit_time > 0 ) {
					//	if( searcher->limit_time < abs((tm.tv_sec - searcher->start_time.tv_sec)*1000 + (tm.tv_usec - searcher->start_time.tv_usec)/1000) ) {
					// 		searcher->run_sign = FALSE;
					// 		break;
					//	}
					// }
				}
			}
		}

		return;
	}

	sub_node = (node->sub) ? g_hash_table_lookup(node->sub, cur_key) : 0;
	if( !sub_node ) {
		// debug, dump snode
		// __snode_dump(node);
		return;
	}

	for( ps=sub_node->elems; searcher->run_sign && ps; ps=ps->next ) {
		g_get_current_time(&tm);
		if( searcher->limit_time > 0 ) {
			if( searcher->limit_time < abs((tm.tv_sec - searcher->start_time.tv_sec)*1000 + (tm.tv_usec - searcher->start_time.tv_usec)/1000) ) {
				searcher->run_sign = FALSE;
				break;
			}
		}

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

			search_call_cb(searcher, elem);
			continue;
		}

		next_sign = FALSE;

		switch( elem->type ) {
		case CPP_ET_NCSCOPE:
			if( cur_type=='?' || cur_type=='t' || cur_type=='n' )
				next_sign = TRUE;
			break;
			
		case CPP_ET_CLASS:
			if( cur_type=='?' || cur_type=='t' ) {
				cur->type = 't';
				for( i=0; i<elem->v_class.inhers_count; ++i )
					loop_insert_typekey(searcher, spath, pos, elem->v_class.inhers[i]);
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
			if( pos->next ) {
				if( elem->type==CPP_ET_CLASS && ((SKey*)(pos->data))->type=='L' && pos->next->next )
					pos = pos->next;
				searcher_do_walk(searcher, sub_node, spath, pos->next);
			}
		}
	}
}

static void searcher_walk(Searcher* searcher, GList* spath) {
	spath_trace(spath);

	if( g_list_length(spath) > 8 )
		return;

	searcher->worked_nodes = g_tree_new(g_direct_equal);
	searcher_do_walk(searcher, searcher->stree->root, spath, spath);
	g_tree_destroy(searcher->worked_nodes);
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
					snode_insert_list(impl_root, elem->v_ncscope.scope);
					searcher_do_walk(searcher, impl_root, spath, pos);
					snode_free(impl_root);
				}

				if( elem->v_fun.typekey ) {
					rep = parse_typekey_to_spath(elem->v_fun.typekey);
					if( rep ) {
						new_spath = spath_replace_new(spath, ((SKey*)(rep->data))->type=='R' ? spath : pos, pos, rep);
						spath_free(rep);

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
					spath_free(rep);

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
					spath_free(rep);

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
	if( file && line && spath && ((SKey*)(spath->data))->type!='R' ) {
		searcher->worked_nodes = g_tree_new(g_direct_equal);
		need_walk = searcher_do_locate(searcher, file->root_scope.v_ncscope.scope, line, spath, spath, need_walk);
		g_tree_destroy(searcher->worked_nodes);
	}

	if( need_walk )
		searcher_add_spath(searcher, spath);
}

static void searcher_start(Searcher* searcher, GList* spath, CppFile* file, gint line) {
	GList* p;
	GList* worked;

	searcher->run_sign = TRUE;
	g_get_current_time( &(searcher->start_time) );

	g_static_rw_lock_reader_lock( &(searcher->stree->lock) );

	searcher_locate(searcher, file, line, spath);

	for( p=searcher->spaths; searcher->run_sign && p; p=p->next ) {
		spath = (GList*)(p->data);

		for( worked=searcher->spaths; worked!=p; worked=worked->next )
			if( spath_equal((GList*)(worked->data), spath) )
				break;

		if( worked==p )
			searcher_walk(searcher, spath);
	}

	g_static_rw_lock_reader_unlock( &(searcher->stree->lock) );
}

void searcher_search( CppSTree* stree
	, GList* spath
	, gboolean (*cb)(CppElem* elem, gpointer tag)
	, gpointer cb_tag
	, CppFile* file
	, gint line
	, gint limit_num
	, gint limit_time )
{
	GList* p;
	Searcher s;
	memset(&s, 0, sizeof(Searcher));

	if( !spath )
		return;

	if( limit_num==0 || limit_time==0 )
		return;

	s.stree = stree;
	s.cb = cb;
	s.cb_tag = cb_tag;
	s.limit_num = limit_num;
	s.limit_time = limit_time;

#ifdef _DEBUG
	g_printerr("-----------------------\n");
#endif

	searcher_start(&s, spath, file, line);

	for( p=s.spaths; p; p=p->next ) {
		if( p->data!=(gpointer)spath )
			spath_free(p->data);
	}
}

