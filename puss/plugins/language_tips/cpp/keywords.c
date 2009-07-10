// keywords.c
// 

#include "keywords.h"


typedef struct {
	gchar*	key;
	gint	len;
	gint	type;
} KeywordItem;

static KeywordItem keyword_items[] = {
	  { "_Bool",      0, KW__BOOL }
	, { "_Complex",   0, KW__COMPLEX }
	, { "_Imaginary", 0, KW__IMAGINARY }
	, { "asm",        0, KW_ASM }
	, { "auto",       0, KW_AUTO }
	, { "bool",       0, KW_BOOL }
	, { "break",      0, KW_BREAK }
	, { "case",       0, KW_CASE }
	, { "catch",      0, KW_CATCH }
	, { "char",       0, KW_CHAR }
	, { "class",      0, KW_CLASS }
	, { "const",      0, KW_CONST }
	, { "const_cast", 0, KW_CONST_CAST }
	, { "continue",   0, KW_CONTINUE }
	, { "default",    0, KW_DEFAULT }
	, { "delete",     0, KW_DELETE }
	, { "do",         0, KW_DO }
	, { "double",     0, KW_DOUBLE }
	, { "dynamic_cast", 0, KW_DYNAMIC_CAST }
	, { "else",       0, KW_ELSE }
	, { "enum",       0, KW_ENUM }
	, { "explicit",   0, KW_EXPLICIT }
	, { "export",     0, KW_EXPORT }
	, { "extern",     0, KW_EXTERN }
	, { "false",      0, KW_FALSE }
	, { "float",      0, KW_FLOAT }
	, { "for",        0, KW_FOR }
	, { "friend",     0, KW_FRIEND }
	, { "goto",       0, KW_GOTO }
	, { "if",         0, KW_IF }
	, { "inline",     0, KW_INLINE }
	, { "int",        0, KW_INT }
	, { "long",       0, KW_LONG }
	, { "mutable",    0, KW_MUTABLE }
	, { "namespace",  0, KW_NAMESPACE }
	, { "new",        0, KW_NEW }
	, { "operator",   0, KW_OPERATOR }
	, { "private",    0, KW_PRIVATE }
	, { "protected",  0, KW_PROTECTED }
	, { "public",     0, KW_PUBLIC }
	, { "register",   0, KW_REGISTER }
	, { "reinterpret_cast", 0, KW_REINTERPRET_CAST }
	, { "restrict",   0, KW_RESTRICT }
	, { "return",     0, KW_RETURN }
	, { "short",      0, KW_SHORT }
	, { "signed",     0, KW_SIGNED }
	, { "sizeof",     0, KW_SIZEOF }
	, { "static",     0, KW_STATIC }
	, { "static_cast", 0, KW_STATIC_CAST }
	, { "struct",     0, KW_STRUCT }
	, { "switch",     0, KW_SWITCH }
	, { "template",   0, KW_TEMPLATE }
	, { "this",       0, KW_THIS }
	, { "throw",      0, KW_THROW }
	, { "true",       0, KW_TRUE }
	, { "try",        0, KW_TRY }
	, { "typedef",    0, KW_TYPEDEF }
	, { "typeid",     0, KW_TYPEID }
	, { "typename ",  0, KW_TYPENAME } 
	, { "union",      0, KW_UNION }
	, { "unsigned",   0, KW_UNSIGNED }
	, { "using",      0, KW_USING }
	, { "virtual",    0, KW_VIRTUAL }
	, { "void",       0, KW_VOID }
	, { "volatile",   0, KW_VOLATILE }
	, { "wchar_t",    0, KW_WCHAR_T }
	, { "while",      0, KW_WHILE }
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
	for( i=0; i<sizeof(keyword_items) / sizeof(KeywordItem); ++i ) {
		keyword_items[i].len = strlen(keyword_items[i].key);
		g_hash_table_insert(keywords, keyword_items + i, GINT_TO_POINTER(keyword_items[i].type));
	}
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

