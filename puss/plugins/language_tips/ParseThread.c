// ParseThread.c
// 

#include "LanguageTips.h"

static gchar* PARSE_THREAD_EXIT_SIGN = "";

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
}

void parse_thread_push(LanguageTips* self, const gchar* filename, gboolean force_rebuild) {
	gchar* filepath;
	if( filename ) {
		filepath = g_strdup_printf("%c%s", force_rebuild ? '!' : ' ', filename);
		g_async_queue_push(self->parse_queue, filepath);
	}
}

