// module.c
// 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libintl.h>

#include "IPuss.h"

#include "cpp/parser.h"

#define TEXT_DOMAIN "language_tips"

#define _(str) dgettext(TEXT_DOMAIN, str)

typedef struct {
	GHashTable*		parsed_files;
} CppEnviron;

typedef struct {
	Puss* app;

	ParserEnviron	cpp_environ;
	CppParser		cpp_parser;

	GAsyncQueue*	parse_queue;
	GThread*		parse_thread;
} LanguageTips;

static LanguageTips* g_self = 0;

static gchar* PARSE_THREAD_EXIT_SIGN = "";

static gpointer tips_parse_thread(gpointer args) {
	gchar* filename = 0;
	GAsyncQueue* queue = (GAsyncQueue*)args;
	if( !queue )
		return 0;

	while( (filename = (gchar*)g_async_queue_pop(queue)) != PARSE_THREAD_EXIT_SIGN ) {
		// parse file
		cpp_parser_parse(&(g_self->cpp_parser), filename, -1, 0, 0);

		g_free(filename);
	}

	g_async_queue_unref(queue);

	return 0;
}

PUSS_EXPORT void* puss_plugin_create(Puss* app) {
	bindtextdomain(TEXT_DOMAIN, app->get_locale_path());
	bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");

	g_self = g_new0(LanguageTips, 1);
	g_self->app = app;

	cpp_parser_init( &(g_self->cpp_parser), TRUE );

	g_self->parse_queue = g_async_queue_new_full(g_free);
	g_self->parse_thread = g_thread_create(tips_parse_thread, g_self->parse_queue, TRUE, 0);
	if( g_self->parse_thread ) {
		g_async_queue_ref(g_self->parse_queue);
	}
	return g_self;
}

PUSS_EXPORT void puss_plugin_destroy(void* ext) {
	if( !g_self )
		return;

	if( g_self->parse_queue ) {
		g_async_queue_push(g_self->parse_queue, PARSE_THREAD_EXIT_SIGN);
		g_async_queue_unref(g_self->parse_queue);
	}

	if( g_self->parse_thread ) {
		g_thread_join(g_self->parse_thread);
	}

	cpp_parser_final( &(g_self->cpp_parser) );

	g_free(g_self);
}


