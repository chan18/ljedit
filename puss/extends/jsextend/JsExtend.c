// JsExtend.cpp
//

#include "JsExtend.h"

#include <seed/seed.h>
#include <JavaScriptCore/JavaScript.h>

struct _JsExtend {
	Puss*		app;

	SeedEngine*	engine;
};

static JsExtend* g_self = 0;

//----------------------------------------------------------------
// IPuss wrappers

void js_wrapper_doc_new(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	if( argc != 0 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_new expected 1 args");
		return;
	}

	g_self->app->doc_new();
}

//----------------------------------------------------------------
// JsExtend implements

gpointer js_plugin_load(const gchar* plugin_id, GKeyFile* keyfile, JsExtend* self) {
	gchar* url = 0;
	gchar* sbuf = 0;
	gsize  slen = 0;
	SeedScript* script = 0;
	SeedValue js_plugin = 0;
	SeedValue fn = 0;
	SeedException e = 0;

	SeedGlobalContext ctx = self->engine->context;

	sbuf = g_strdup_printf("%s.js", plugin_id);
	url = g_build_filename(self->app->get_plugins_path(), sbuf, 0);
	g_free(sbuf);
	sbuf = 0;

	if( !url )
		goto finished;

	if( !g_file_get_contents(url, &sbuf, &slen, 0) )
		goto finished;

	script = seed_make_script(ctx, sbuf, url, 1);
	e = seed_script_exception(script);
	if( e ) {
		g_critical("%s. %s in %s at line %d",
				   seed_exception_get_name(ctx, e),
				   seed_exception_get_message(ctx, e),
				   seed_exception_get_file(ctx, e),
				   seed_exception_get_line(ctx, e));
		g_free(e);
		e = 0;
		goto finished;
	}

	js_plugin = seed_evaluate(ctx, script, 0);
	e = seed_script_exception(script);
	if( e ) {
		g_critical("%s. %s in %s at line %d",
				   seed_exception_get_name(ctx, e),
				   seed_exception_get_message(ctx, e),
				   seed_exception_get_file(ctx, e),
				   seed_exception_get_line(ctx, e));
		g_free(e);
		e = 0;
		goto finished;
	}

	seed_value_protect(ctx, js_plugin);

	fn = seed_object_get_property(ctx, js_plugin, "active");
	if( fn ) {
		seed_object_call(ctx, fn, js_plugin, 0, 0, &e);
		if( e ) {
			g_critical("%s. %s in %s at line %d",
					   seed_exception_get_name(ctx, e),
					   seed_exception_get_message(ctx, e),
					   seed_exception_get_file(ctx, e),
					   seed_exception_get_line(ctx, e));
			g_free(e);
			e = 0;
			goto finished;
		}
	}
	// g_printf("load!!!\n");

finished:
	g_free(url);
	g_free(sbuf);
	g_free(script);

	return js_plugin;
}

void js_plugin_unload(gpointer plugin, JsExtend* self) {
	SeedException e = 0;
	SeedValue fn = 0;
	SeedObject js_plugin = (SeedObject)plugin;
	SeedGlobalContext ctx = self->engine->context;

	fn = seed_object_get_property(ctx, js_plugin, "deactive");
	if( fn ) {
		seed_object_call(ctx, fn, js_plugin, 0, 0, &e);
		if( e ) {
			g_critical("%s. %s in %s at line %d",
					   seed_exception_get_name(ctx, e),
					   seed_exception_get_message(ctx, e),
					   seed_exception_get_file(ctx, e),
					   seed_exception_get_line(ctx, e));
			g_free(e);
			e = 0;
		}
	}

	seed_value_unprotect(self->engine->context, js_plugin);
	// g_printf("unloadl!!!\n");
}

JsExtend* puss_js_extend_create(Puss* app) {
	static gint argc = 0;
	static gchar** argv = {""};

	SeedEngine* engine;
	SeedValue js_puss;
	SeedValue js_val;
	GObject* gobj;

	g_self = g_try_new0(JsExtend, 1);
	if( !g_self )
		return g_self;

	engine = seed_init(&argc, &argv);
	g_self->app = app;
	g_self->engine = engine;

	js_puss = seed_make_object(engine->context, 0, 0);
	seed_object_set_property(engine->context, engine->global, "puss", js_puss);

	gobj = G_OBJECT(app->get_ui_builder());
	js_val = seed_value_from_object(engine->context, g_object_ref(gobj), 0);
	seed_object_set_property(engine->context, js_puss, "ui", js_val);

	seed_object_set_property(engine->context, js_puss, "module_path", seed_value_from_string(engine->context, app->get_module_path(), 0));
	seed_object_set_property(engine->context, js_puss, "locale_path", seed_value_from_string(engine->context, app->get_locale_path(), 0));
	seed_object_set_property(engine->context, js_puss, "extends_path", seed_value_from_string(engine->context, app->get_extends_path(), 0));
	seed_object_set_property(engine->context, js_puss, "plugins_path", seed_value_from_string(engine->context, app->get_plugins_path(), 0));

	seed_create_function(engine->context, "doc_new", &js_wrapper_doc_new, js_puss);

	app->plugin_engine_regist( "javascript"
		, (PluginLoader)js_plugin_load
		, (PluginUnloader)js_plugin_unload
		, 0
		, g_self );

	return g_self;
}

void puss_js_extend_destroy(JsExtend* self) {
	g_self = 0;
}

