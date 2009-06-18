// searcher.h
// 

#ifndef PUSS_CPP_SEARCHER_H
#define PUSS_CPP_SEARCHER_H

#include "guide.h"

typedef struct {
	gchar (*do_prev)(gpointer it);
	gchar (*do_next)(gpointer it);
} SearchIterEnv;

gchar* cpp_find_key(SearchIterEnv* env, gpointer ps, gpointer pe, gboolean find_startswith);
gchar* cpp_parse_key(const gchar* text, gboolean find_startswith);

/*

typedef void (*CppMatched)(CppElem* elem, gpointer* tag);

void cpp_search( CppGuide* guide
	, const gchar* key
	, CppMatched cb
	, gpointer cb_tag
	, CppSearchTree* stree
	, CppFile* file
	, gint line );
*/

#endif//PUSS_CPP_SEARCHER_H

