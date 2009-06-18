// ParseThread.c
// 

#include "LanguageTips.h"

static gchar* PARSE_THREAD_EXIT_SIGN = "";

static gpointer tips_parse_thread(LanguageTips* self) {
	CppFile* file;
	gchar* filename = 0;
	GAsyncQueue* queue = self->parse_queue;
	if( !queue )
		return 0;

	for(;;) {
		filename = (gchar*)g_async_queue_pop(queue);
		if( filename==PARSE_THREAD_EXIT_SIGN )
			break;

		if( !filename )
			continue;

		// parse file
		file = cpp_guide_parse(self->cpp_guide, filename, -1, FALSE);
		if( file )
			cpp_file_unref(file);

		g_free(filename);
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

