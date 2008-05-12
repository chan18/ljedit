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


#define CPP_ET_NCSCOPE		'S'	// namespace or class
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
	gsize	len;
	gchar	buf[1];
} TinyStr;

TinyStr* tiny_str_new(gchar* buf, gsize len);
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
	TinyStr*	typekey;
	TinyStr*	nskey;
	gboolean	fun_ptr;
	Template*	fun_template;
	GList*		impl;
} CppFun;

#define CPP_NC_SCOPE \

typedef struct {
	GList*		impl;
} CppNCScope;

typedef struct {
	CppNCScope	scope;
} CppNamespace;

typedef struct {
	CppNCScope	scope;

	gchar		class_type;
	TinyStr*	nskey;
	gsize		inhers_count;
	TinyStr**	inhers;
} CppClass;

typedef struct {
	TinyStr*	value;
} CppEnumItem;

typedef struct {
	CppNCScope	scope;

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
		CppKeyword		keyword;
		CppMacroDefine	macro_define;
		CppMacroUndef	macro_undef;
		CppMacroInclude	macro_include;
		CppVar			cpp_var;
		CppFun			cpp_fun;
		CppNamespace	cpp_namespace;
		CppClass		cpp_class;
		CppEnumItem		cpp_enum_item;
		CppEnum			cpp_enum;
		CppUsing		cpp_using;
		CppTypedef		cpp_typedef;
	};
};

struct _CppFile {
	TinyStr*	filename;
	time_t		datetime;
	GList*		elems;
	int			ref_count;
};

#endif//PUSS_CPP_DS_H

