// guide.h
// 
#ifndef PUSS_CPP_GUIDE_H
#define PUSS_CPP_GUIDE_H

#ifdef _MSVCR
#pragma warning( disable : 4244 )
#pragma warning( disable : 4311 )
#pragma warning( disable : 4312 )
#endif//_MSVCR

#include <glib.h>
#include <memory.h>

#define CPP_VIEW_NORMAL		'N'
#define CPP_VIEW_PUBLIC		'P'
#define CPP_VIEW_PROTECTED	'O'
#define CPP_VIEW_PRIVATE	'R'


enum CppElemType {
	  CPP_ET__FIRST
	, CPP_ET_NCSCOPE
	, CPP_ET_KEYWORD
	, CPP_ET_UNDEF
	, CPP_ET_MACRO
	, CPP_ET_INCLUDE
	, CPP_ET_VAR
	, CPP_ET_FUN
	, CPP_ET_ENUMITEM
	, CPP_ET_ENUM
	, CPP_ET_CLASS
	, CPP_ET_USING
	, CPP_ET_NAMESPACE
	, CPP_ET_TYPEDEF
	, CPP_ET__LAST
};

typedef struct _CppElem    CppElem;
typedef struct _CppFile    CppFile;

typedef struct {
	gchar	len_hi;
	gchar	len_lo;
	gchar	buf[1];
} TinyStr;

typedef struct {
	gpointer	nouse;
} CppKeyword;

typedef struct {
	gint		argc;
	TinyStr**	argv;
	TinyStr*	value;
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

#define CPP_CLASS_TYPE_UNKNOWN	'?'
#define CPP_CLASS_TYPE_STRUCT	's'
#define CPP_CLASS_TYPE_CLASS	'c'
#define CPP_CLASS_TYPE_UNION	'u'

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

struct _CppFile {
	gint		ref_count;
	gint		status;
	TinyStr*	filename;
	time_t		datetime;
	CppElem		root_scope;
};

#define cpp_elem_has_subscope(e)		\
		(  (e)->type==CPP_ET_NCSCOPE	\
		|| (e)->type==CPP_ET_NAMESPACE	\
		|| (e)->type==CPP_ET_CLASS		\
		|| (e)->type==CPP_ET_ENUM		\
		|| (e)->type==CPP_ET_FUN )

#define cpp_elem_get_subscope(e) ((e)->v_ncscope.scope)

CppFile* cpp_file_ref(CppFile* file);
void     cpp_file_unref(CppFile* file);

// parser

typedef struct {
	gint	ref_count;
	GList*	path_list;
} CppIncludePaths;

typedef struct _CppGuide CppGuide;

gchar* cpp_filename_to_filekey(const gchar* filename, glong namelen);

CppGuide* cpp_guide_new(gboolean enable_macro_replace, gboolean enable_search);
void cpp_guide_free(CppGuide* guide);

void cpp_guide_include_paths_set(CppGuide* guide, const gchar* paths);
CppIncludePaths* cpp_guide_include_paths_ref(CppGuide* guide);
void cpp_guide_include_paths_unref(CppIncludePaths* paths);

CppFile* cpp_guide_find_parsed(CppGuide* guide, const gchar* filename, gint namelen);
CppFile* cpp_guide_parse(CppGuide* guide, const gchar* filename, gint namelen, gboolean force_rebuild);

gpointer cpp_spath_find( gboolean find_startswith
			, gchar (*do_prev)(gpointer it)
			, gchar (*do_next)(gpointer it)
			, gpointer start_iter
			, gpointer end_iter );
gpointer cpp_spath_parse(gboolean find_startswith, const gchar* text);
void cpp_spath_free(gpointer spath);

typedef void (*CppMatched)(CppElem* elem, gpointer tag);

void cpp_guide_search_with_callback( CppGuide* guide
			, gpointer spath
			, CppMatched cb
			, gpointer cb_tag
			, CppFile* file
			, gint line );

#define CPP_GUIDE_SEARCH_FLAG_WITH_KEYWORDS	0x0001
#define CPP_GUIDE_SEARCH_FLAG_USE_UNIQUE_ID	0x0002

GSequence* cpp_guide_search( CppGuide* guide
			, gpointer spath
			, gint flag
			, CppFile* file
			, gint line );

#endif//PUSS_CPP_GUIDE_H

