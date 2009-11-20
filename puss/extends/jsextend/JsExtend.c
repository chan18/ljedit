// JsExtend.cpp
//

#include "JsExtend.h"

#include <seed/seed.h>

struct _JsExtend {
	Puss*		app;

	SeedEngine*	engine;
	SeedObject	js_puss;
};

static JsExtend* g_self = 0;

//----------------------------------------------------------------
// IPuss wrappers

static SeedValue js_wrapper_doc_new(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	if( argc != 0 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_new expected 0 args");
	} else {
		g_self->app->doc_new();
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_open(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	gboolean res;
	const char* url = 0;
	gint line = -1;
	gint offset = -1;
	gboolean flag = FALSE;

	if( argc==0 || argc > 4 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_open expected 1 ~ 4 args");
		return seed_make_undefined(ctx);
	}

	if( argc > 0 )	url = seed_value_to_string(ctx, argv[0], e);
	if( argc > 1 )	line = seed_value_to_int(ctx, argv[1], e);
	if( argc > 2 )	offset = seed_value_to_int(ctx, argv[2], e);
	if( argc > 3 )	flag = seed_value_to_boolean(ctx, argv[3], e);

	res = g_self->app->doc_open(url, line, offset, flag);

	return seed_value_from_boolean(ctx, res, e);
}

static SeedValue js_wrapper_doc_locate(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	gboolean res;
	gint page = 0;
	gint line = -1;
	gint offset = -1;
	gboolean flag = FALSE;

	if( argc==0 || argc > 4 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_locate expected 1 ~ 4 args");
		return seed_make_undefined(ctx);
	}

	if( argc > 0 )	page = seed_value_to_int(ctx, argv[0], e);
	if( argc > 1 )	line = seed_value_to_int(ctx, argv[1], e);
	if( argc > 2 )	offset = seed_value_to_int(ctx, argv[2], e);
	if( argc > 3 )	flag = seed_value_to_boolean(ctx, argv[3], e);

	res = g_self->app->doc_locate(page, line, offset, flag);

	return seed_value_from_boolean(ctx, res, e);
}

static SeedValue js_wrapper_doc_save_current(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	gboolean save_as;

	if( argc > 1 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_save_current expected 0 ~ 1 args");
		return seed_make_undefined(ctx);
	}

	if( argc > 0 )	save_as = seed_value_to_boolean(ctx, argv[0], e);

	g_self->app->doc_save_current(save_as);
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_close_current(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	if( argc != 0 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_close_current expected 0 args");
	} else {
		g_self->app->doc_close_current();
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_save_all(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	if( argc != 0 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_save_all expected 0 args");
	} else {
		g_self->app->doc_save_all();
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_close_all(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	if( argc != 0 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_close_all expected 0 args");
	} else {
		g_self->app->doc_close_all();
	}
	return seed_make_undefined(ctx);
}

//----------------------------------------------------------------
// JsExtend implements

typedef struct {
	gchar*				id;
	SeedGlobalContext	ctx;
	SeedValue			value;
} PussJsPlugin;

static gpointer js_plugin_load(const gchar* plugin_id, GKeyFile* keyfile, JsExtend* self) {
	gchar* url = 0;
	SeedScript* script = 0;
	PussJsPlugin* js_plugin = 0;
	gchar* sbuf = 0;
	SeedValue global = 0;
	SeedValue obj = 0;
	SeedException e = 0;
	gchar* msg = 0;

	sbuf = g_strdup_printf("%s.js", plugin_id);
	url = g_build_filename(self->app->get_plugins_path(), sbuf, NULL);
	g_free(sbuf);
	sbuf = 0;

	if( !url )
		goto finished;

	js_plugin = g_new0(PussJsPlugin, 1);
	js_plugin->id = g_strdup(plugin_id);
	js_plugin->ctx = seed_context_create(self->engine->group, 0);
	global = seed_context_get_global_object(js_plugin->ctx);
	seed_prepare_global_context(js_plugin->ctx);
	seed_object_set_property(js_plugin->ctx, global, "puss", self->js_puss);

	script = seed_script_new_from_file(js_plugin->ctx, url);
	e = seed_script_exception(script);
	if( e ) {
		msg = seed_exception_to_string(js_plugin->ctx, e);
		g_critical("%s", msg);
		g_free(msg);
		goto finished;
	}

	seed_evaluate(js_plugin->ctx, script, 0);
	e = seed_script_exception(script);
	if( e ) {
		msg = seed_exception_to_string(js_plugin->ctx, e);
		g_critical("[%s] %s", plugin_id, msg);
		g_free(msg);
		goto finished;
	}

	obj = seed_object_get_property(js_plugin->ctx, global, "puss_plugin_active");
	if( !obj || !seed_value_is_function(js_plugin->ctx, obj) ) {
		g_critical("[%s] not find active function!", plugin_id);
		goto finished;
	}

	js_plugin->value = seed_object_call(js_plugin->ctx, obj, global, 0, 0, &e);
	if( e ) {
		msg = seed_exception_to_string(js_plugin->ctx, e);
		g_critical("[%s] %s", plugin_id, msg);
		g_free(msg);
		goto finished;
	}
	seed_value_protect(js_plugin->ctx, js_plugin->value);
	// g_printf("load!!!\n");

finished:
	g_free(url);
	g_free(sbuf);
	g_free(script);

	return js_plugin;
}

static void js_plugin_unload(gpointer plugin, JsExtend* self) {
	PussJsPlugin* js_plugin = (PussJsPlugin*)plugin;
	SeedValue global = 0;
	SeedValue obj = 0;
	SeedException e = 0;
	gchar* msg = 0;

	if( !js_plugin )
		return;

	if( js_plugin->ctx && js_plugin->value) {
		global = seed_context_get_global_object(js_plugin->ctx);
		obj = seed_object_get_property(js_plugin->ctx, global, "puss_plugin_deactive");
		if( obj ) {
			if( !seed_value_is_function(js_plugin->ctx, obj) ) {
				g_critical("[%s] not find deactive function!", js_plugin->id);

			} else {
				seed_object_call(js_plugin->ctx, obj, global, 1, &(js_plugin->value), &e);
				if( e ) {
					msg = seed_exception_to_string(js_plugin->ctx, e);
					g_critical("[%s] %s", js_plugin->id, msg);
					g_free(msg);
				}
			}
		}
		seed_value_unprotect(js_plugin->ctx, js_plugin->value);
	}

	seed_context_unref(js_plugin->ctx);
	g_free(js_plugin->id);
	g_free(js_plugin);
	// g_printf("unloadl!!!\n");
}

JsExtend* puss_js_extend_create(Puss* app) {
	static gint argc = 0;
	static gchar* argv[1] = {""};

	SeedEngine* engine;
	SeedValue js_val;
	GObject* gobj;

	g_self = g_try_new0(JsExtend, 1);
	if( !g_self )
		return g_self;

	engine = seed_init(&argc, (gchar***)&argv);
	g_self->app = app;
	g_self->engine = engine;

	g_self->js_puss = seed_make_object(engine->context, 0, 0);
	seed_object_set_property(engine->context, engine->global, "puss", g_self->js_puss);

	gobj = G_OBJECT(app->get_ui_builder());
	js_val = seed_value_from_object(engine->context, g_object_ref(gobj), 0);
	seed_object_set_property(engine->context, g_self->js_puss, "ui", js_val);

	seed_object_set_property(engine->context, g_self->js_puss, "module_path",	seed_value_from_string(engine->context, app->get_module_path(), 0));
	seed_object_set_property(engine->context, g_self->js_puss, "locale_path",	seed_value_from_string(engine->context, app->get_locale_path(), 0));
	seed_object_set_property(engine->context, g_self->js_puss, "extends_path",	seed_value_from_string(engine->context, app->get_extends_path(), 0));
	seed_object_set_property(engine->context, g_self->js_puss, "plugins_path",	seed_value_from_string(engine->context, app->get_plugins_path(), 0));

	seed_create_function(engine->context, "doc_new",			&js_wrapper_doc_new, g_self->js_puss);
	seed_create_function(engine->context, "doc_open",			&js_wrapper_doc_open, g_self->js_puss);
	seed_create_function(engine->context, "doc_locate",			&js_wrapper_doc_locate, g_self->js_puss);
	seed_create_function(engine->context, "doc_save_current",	&js_wrapper_doc_save_current, g_self->js_puss);
	seed_create_function(engine->context, "doc_save_all",		&js_wrapper_doc_save_all, g_self->js_puss);
	seed_create_function(engine->context, "doc_close_all",		&js_wrapper_doc_close_all, g_self->js_puss);

	app->plugin_engine_regist( "javascript"
		, (PluginLoader)js_plugin_load
		, (PluginUnloader)js_plugin_unload
		, 0
		, g_self );

	return g_self;
}

void puss_js_extend_destroy(JsExtend* self) {
	g_self = 0;
	if( self ) {
		// TODO : destroy engine
		g_free(self);
	}
}

