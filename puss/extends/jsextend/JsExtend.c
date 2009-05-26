// JsExtend.cpp
//

#include "JsExtend.h"

#include <seed/seed.h>
#include <JavaScriptCore/JavaScript.h>

struct _JsExtend {
	Puss*		app;

	SeedEngine*	engine;

	// Puss Module
	// 
	JSObjectRef* builder;
};

static JsExtend* g_self = 0;

//----------------------------------------------------------------
// JsExtend implements

JsExtend* puss_js_extend_create(Puss* app) {
	static gint argc = 0;
	static gchar** argv = {""};

	g_self = g_try_new0(JsExtend, 1);
	if( g_self ) {
		g_self->app = app;

		g_self->engine = seed_init(&argc, &argv);

		{
			SeedScript  * script;
			script = seed_make_script(g_self->engine->context, "Seed.print('hello');", 0, 0);
			seed_evaluate(g_self->engine->context, script, 0);
	
			g_free(script);
		}
	}

	return g_self;
}

void puss_js_extend_destroy(JsExtend* self) {
	g_self = 0;

	if( self ) {
	}
}

