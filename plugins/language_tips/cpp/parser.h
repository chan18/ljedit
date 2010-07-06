// parser.h
// 
#ifndef PUSS_CPP_PARSER_H
#define PUSS_CPP_PARSER_H

#include "ds.h"

typedef void (*FileInsertCallback)(CppFile* file, gpointer tag);
typedef void (*FileRemoveCallback)(CppFile* file, gpointer tag);

typedef struct {
	gboolean			enable_macro_replace;
	gpointer			keywords_table;

	GStaticRWLock		settings_lock;
	CppIncludePaths*	include_paths;
	GList*				predefineds;

	GStaticRWLock		files_lock;
	GHashTable*			files;

	FileInsertCallback	cb_file_insert;
	FileRemoveCallback	cb_file_remove;
	gpointer			cb_tag;
} CppParser;

void cpp_parser_init(CppParser* self, gboolean enable_macro_replace);

void cpp_parser_final(CppParser* self);

void cpp_parser_predefineds_set(CppParser* self, GList* files);

void cpp_parser_include_paths_set(CppParser* self, GList* paths);
CppIncludePaths* cpp_parser_include_paths_ref(CppParser* self);
void cpp_parser_include_paths_unref(CppIncludePaths* paths);

CppFile* cpp_parser_find_parsed(CppParser* self, const gchar* filekey);
CppFile* cpp_parser_parse(CppParser* self, const gchar* filekey, gboolean force_rebuild);

#endif//PUSS_CPP_PARSER_H

