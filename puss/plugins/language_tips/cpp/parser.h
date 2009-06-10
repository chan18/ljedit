// parser.h
// 
#ifndef PUSS_CPP_PARSER_H
#define PUSS_CPP_PARSER_H

#include "ds.h"

typedef struct {
	gint	ref_count;
	GList*	paths;
} IncludePaths;

typedef struct {
	gboolean		enable_macro_replace;
	gpointer		keywords_table;

	GStaticRWLock	include_paths_lock;
	IncludePaths*	include_paths;

	GStaticRWLock	files_lock;
	GHashTable*		files;
} CppParser;

void cpp_parser_init(CppParser* parser, gboolean enable_macro_replace);
void cpp_parser_final(CppParser* parser);

void cpp_parser_set_include_paths(CppParser* parser, GList* paths);
CppFile* cpp_parser_find_parsed(CppParser* parser, const gchar* filekey);
CppFile* cpp_parser_parse(CppParser* parser, const gchar* filekey);

#endif//PUSS_CPP_PARSER_H

