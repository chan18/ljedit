// ParseThread.c
// 

#include "LanguageTips.h"

static gchar* PARSE_THREAD_EXIT_SIGN = "";

static void push_implement_file_into_queue(LanguageTips* self, const gchar* filename, const gchar* suffix) {
	gchar* filepath = g_strdup_printf("%c%s%s", ' ', filename, suffix, 0);
	g_async_queue_push(self->parse_queue, filepath);
}

static void on_parse_header_file(LanguageTips* self, const gchar* filename, const gchar* suffix) {
	if( g_str_equal(suffix, "h") ) {
		push_implement_file_into_queue(self, filename, "c");
		push_implement_file_into_queue(self, filename, "cpp");

	} else if( g_str_equal(suffix, "hpp") ) {
		push_implement_file_into_queue(self, filename, "cpp");

	} else if( g_str_equal(suffix, "hh") ) {
		push_implement_file_into_queue(self, filename, "cc");

	} else if( g_str_equal(suffix, "hxx") ) {
		push_implement_file_into_queue(self, filename, "cxx");

	}
}

static void on_file_parsed(CppFile* file, LanguageTips* self) {
	char old;
	char suffix[5];
	char* ps;
	char* pe;
	char* suffix_ptr;
	guint len;

	len = tiny_str_len(file->filename);
	if( !len )
		return;

	ps = file->filename->buf;
	for( pe = (ps + len - 1); (pe > ps) && (*pe!='.'); --pe )
		;

	if( *pe != '.' || ( (len - (pe-ps)) > 4 ) )
		return;

	++pe;
	suffix_ptr = pe;

	ps = suffix;
	for( ps=suffix; *pe; ++ps ) {
		*ps = g_ascii_tolower(*pe);
		++pe;
	}
	*ps = '\0';

	old = *suffix_ptr;
	*suffix_ptr = '\0';

	on_parse_header_file(self, file->filename->buf, suffix);

	*suffix_ptr = old;
}

static gpointer tips_parse_thread(LanguageTips* self) {
	CppFile* file;
	gchar* str;
	gchar rebuild_sign;
	gchar* filename;
	GAsyncQueue* queue = self->parse_queue;
	if( !queue )
		return 0;

	for(;;) {
		str = (gchar*)g_async_queue_pop(queue);
		if( str==PARSE_THREAD_EXIT_SIGN )
			break;

		if( !str )
			continue;

		// parse file
		rebuild_sign = str[0];
		filename = str + 1;

		file = cpp_guide_parse(self->cpp_guide, filename, -1, rebuild_sign=='!');
		if( file )
			cpp_file_unref(file);

		g_free(str);
	}

	g_async_queue_unref(queue);

	return 0;
}

void parse_thread_init(LanguageTips* self) {
	self->cpp_guide = cpp_guide_new(TRUE, TRUE, on_file_parsed, self);

	self->parse_queue = g_async_queue_new_full(g_free);
	g_async_queue_ref(self->parse_queue);
	self->parse_thread = g_thread_create(tips_parse_thread, self, TRUE, 0);
}

void parse_thread_final(LanguageTips* self) {
	if( self->parse_queue ) {
		g_async_queue_push(self->parse_queue, PARSE_THREAD_EXIT_SIGN);
		g_async_queue_unref(self->parse_queue);
	}

	if( self->parse_thread ) {
		g_thread_join(self->parse_thread);
	}

	cpp_guide_free(self->cpp_guide);
}

void parse_thread_push(LanguageTips* self, const gchar* filename, gboolean force_rebuild) {
	gchar* filepath;
	if( filename ) {
		filepath = g_strdup_printf("%c%s", force_rebuild ? '!' : ' ', filename);
		g_async_queue_push(self->parse_queue, filepath);
	}
}

