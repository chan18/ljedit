// ds.h
// 
#ifndef PUSS_CPP_DS_H
#define PUSS_CPP_DS_H

#include <glib.h>
#include <memory.h>

#define CPP_VIEW_NORMAL		'N'
#define CPP_VIEW_PUBLIC		'P'
#define CPP_VIEW_PROTECTED	'O'
#define CPP_VIEW_PRIVATE	'R'


#define CPP_ET_NCSCOPE		'S'
#define CPP_ET_KEYWORD		'K'
#define CPP_ET_UNDEF		'U'
#define CPP_ET_MACRO		'M'
#define CPP_ET_INCLUDE		'I'
#define CPP_ET_VAR			'v'
#define CPP_ET_FUN			'f'
#define CPP_ET_ENUMITEM		'i'
#define CPP_ET_ENUM			'e'
#define CPP_ET_CLASS		'c'
#define CPP_ET_USING		'u'
#define CPP_ET_NAMESPACE	'n'
#define CPP_ET_TYPEDEF		't'

typedef struct _CppElem    CppElem;
typedef struct _CppFile    CppFile;

typedef struct {
	gshort	len;
	gchar	buf[1];
} TinyStr;

TinyStr* tiny_str_new(const gchar* buf, gshort len);
void     tiny_str_free(TinyStr* str);
#define  tiny_str_copy(str) tiny_str_new((str)->buf, (str)->len)
gboolean tiny_str_equal(const TinyStr* a, const TinyStr* b);

typedef struct {
	gpointer	nouse;
} CppKeyword;

typedef struct {
	gpointer	nouse;
} CppMacroDefine;

typedef struct {
	gpointer	_nouse;
} CppMacroUndef;

typedef struct {
	gboolean	sys_header;
	TinyStr*	filename;
	TinyStr*	include_file;
} CppMacroInclude;

typedef struct {
	TinyStr*	typekey;
	TinyStr*	nskey;
} CppVar;

typedef struct {
	TinyStr*	type;
	TinyStr*	name;
	TinyStr*	value;
} TemplateArg;

typedef struct {
	gsize		argc;
	TemplateArg	argv[0];
} Template;

typedef struct {
	GList*		scope;
} CppNCScope;

typedef struct {
	CppNCScope	subscope;

	TinyStr*	typekey;
	TinyStr*	nskey;
	gboolean	fun_ptr;
	Template*	fun_template;
} CppFun;

typedef struct {
	CppNCScope	subscope;
} CppNamespace;

typedef struct {
	CppNCScope	subscope;

	gchar		class_type;
	TinyStr*	nskey;
	gsize		inhers_count;
	TinyStr**	inhers;
} CppClass;

typedef struct {
	TinyStr*	value;
} CppEnumItem;

typedef struct {
	CppNCScope	subscope;

	TinyStr*	nskey;
} CppEnum;

typedef struct {
	TinyStr*	nskey;
	gboolean	isns;
} CppUsing;

typedef struct {
	TinyStr*	typekey;
} CppTypedef;

struct _CppElem {
	CppFile*	file;
	gchar		type;
	gchar		view;
	TinyStr*	name;
	gint		sline;
	gint		eline;
	gint		offset;
	TinyStr*	decl;

	union {
		CppKeyword		v_keyword;
		CppMacroDefine	v_define;
		CppMacroUndef	v_undef;
		CppMacroInclude	v_include;
		CppVar			v_var;
		CppFun			v_fun;
		CppNCScope		v_ncscope;
		CppNamespace	v_namespace;
		CppClass		v_class;
		CppEnumItem		v_enum_item;
		CppEnum			v_enum;
		CppUsing		v_using;
		CppTypedef		v_typedef;
	};
};

CppElem* cpp_elem_new();
void cpp_elem_free(CppElem* elem);
void cpp_elem_clear(CppElem* elem);

#define cpp_elem_has_subscope(e)		\
		(  (e)->type==CPP_ET_NCSCOPE	\
		|| (e)->type==CPP_ET_NAMESPACE	\
		|| (e)->type==CPP_ET_CLASS		\
		|| (e)->type==CPP_ET_ENUM		\
		|| (e)->type==CPP_ET_FUN )

#define cpp_elem_get_subscope(e) ((e)->v_ncscope.scope)

void cpp_scope_insert(CppElem* parent, CppElem* elem);

struct _CppFile {
	int			ref_count;
	TinyStr*	filename;
	time_t		datetime;
	CppElem		root_scope;
};

void     cpp_file_clear(CppFile* file);

CppFile* cpp_file_ref(CppFile* file);
void     cpp_file_unref(CppFile* file);

#endif//PUSS_CPP_DS_H
