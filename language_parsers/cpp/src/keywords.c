// keywords.c
// 

#include "keywords.h"


typedef struct {
	gchar*	key;
	gint	len;
	gint	type;
} KeywordItem;

static KeywordItem keyword_items[] = {
	  { "asm",        3, KW_ASM }
	, { "auto",       4, KW_AUTO }
	, { "bool",       4, KW_BOOL }
	, { "break",      5, KW_BREAK }
	, { "case",       4, KW_CASE }
	, { "catch",      5, KW_CATCH }
	, { "char",       4, KW_CHAR }
	, { "class",      5, KW_CLASS }
	, { "const",      5, KW_CONST }
	, { "const_cast", 10, KW_CONST_CAST }
	, { "continue",   8, KW_CONTINUE }
	, { "default",    7, KW_DEFAULT }
	, { "delete",     6, KW_DELETE }
	, { "do",         2, KW_DO }
	, { "double",     6, KW_DOUBLE }
	, { "dynamic_cast", 12, KW_DYNAMIC_CAST }
	, { "else",       4, KW_ELSE }
	, { "enum",       4, KW_ENUM }
	, { "explicit",   8, KW_EXPLICIT }
	, { "export",     6, KW_EXPORT }
	, { "extern",     6, KW_EXTERN }
	, { "false",      5, KW_FALSE }
	, { "float",      5, KW_FLOAT }
	, { "for",        3, KW_FOR }
	, { "friend",     6, KW_FRIEND }
	, { "goto",       4, KW_GOTO }
	, { "if",         2, KW_IF }
	, { "inline",     6, KW_INLINE }
	, { "int",        3, KW_INT }
	, { "long",       4, KW_LONG }
	, { "mutable",    7, KW_MUTABLE }
	, { "namespace",  9, KW_NAMESPACE }
	, { "new",        3, KW_NEW }
	, { "operator",   8, KW_OPERATOR }
	, { "private",    7, KW_PRIVATE }
	, { "protected",  9, KW_PROTECTED }
	, { "public",     6, KW_PUBLIC }
	, { "register",   8, KW_REGISTER }
	, { "reinterpret_cast", 16, KW_REINTERPRET_CAST }
	, { "return",     6, KW_RETURN }
	, { "short",      5, KW_SHORT }
	, { "signed",     6, KW_SIGNED }
	, { "sizeof",     6, KW_SIZEOF }
	, { "static",     6, KW_STATIC }
	, { "static_cast", 11, KW_STATIC_CAST }
	, { "struct",     6, KW_STRUCT }
	, { "switch",     6, KW_SWITCH }
	, { "template",   8, KW_TEMPLATE }
	, { "this",       4, KW_THIS }
	, { "throw",      5, KW_THROW }
	, { "true",       4, KW_TRUE }
	, { "try",        3, KW_TRY }
	, { "typedef",    7, KW_TYPEDEF }
	, { "typeid",     6, KW_TYPEID }
	, { "typename ",  8, KW_TYPENAME } 
	, { "union",      5, KW_UNION }
	, { "unsigned",   8, KW_UNSIGNED }
	, { "using",      5, KW_USING }
	, { "virtual",    7, KW_VIRTUAL }
	, { "void",       4, KW_VOID }
	, { "volatile",   8, KW_VOLATILE }
	, { "wchar_t",    7, KW_WCHAR_T }
	, { "while",      5, KW_WHILE }
};

typedef struct {
	gchar*	buf;
	gint	len;
} Key;

// can use for MLStr, MLToken, KeywordItem
// 
static guint key_hash(Key* key) {
	guint h = 0;
	gchar* ps = key->buf;
	gchar* pe = ps + key->len;
	for( ; ps < pe; ++ps )
		h = (h << 5) - h + *ps;

	return h;
}

static gboolean key_equal(Key* l, Key* r) {
	if( l->len==r->len ) {
		gchar* pe = l->buf + l->len;
		gchar* pl = l->buf;
		gchar* pr = r->buf;

		for( ; pl < pe; ++pl, ++pr)
			if( *pl!=*pr )
				return FALSE;

		return TRUE;
	}

	return FALSE;
}

gpointer cpp_keywords_table_new() {
	GHashTable* keywords = g_hash_table_new((GHashFunc)key_hash, (GEqualFunc)key_equal);
	gint i;
	for( i=0; i<sizeof(keyword_items) / sizeof(KeywordItem); ++i )
		g_hash_table_insert(keywords, keyword_items + i, GINT_TO_POINTER(keyword_items[i].type));

	return keywords;
}

void cpp_keywords_table_free(gpointer keywords_table) {
	g_hash_table_destroy((GHashTable*)keywords_table);
}

void cpp_keywords_check(MLToken* token, gpointer keywords_table) {
	gpointer value;

	g_assert( token && token->type==TK_ID && keywords_table );
	g_assert( ((Key*)token)->buf == token->buf );
	g_assert( ((Key*)token)->len == token->len );

	value = g_hash_table_lookup((GHashTable*)keywords_table, (Key*)token);
	if( value )
		token->type = GPOINTER_TO_INT(value);
}

