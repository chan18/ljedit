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

static SeedValue js_wrapper_doc_get_url(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GString* url;
	GObject* buf;

	if( argc != 1 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_get_url expected 1 args");
	} else {
		buf = seed_value_to_object(ctx, argv[0], e);
		if( buf && GTK_TEXT_BUFFER(buf) ) {
			url = g_self->app->doc_get_url(GTK_TEXT_BUFFER(buf));
			return seed_value_from_string(ctx, url->str, e);
		} else {
			seed_make_exception(ctx, e, "ArgumentError", "bad arg format");
		}
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_set_url(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GObject* buf;
	gchar* url = 0;

	if( argc != 2 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_set_url expected 2 args");
	} else {
		buf = seed_value_to_object(ctx, argv[0], e);
		url = seed_value_to_string(ctx, argv[1], e);
		if( buf && GTK_TEXT_BUFFER(buf) ) {
			g_self->app->doc_set_url(GTK_TEXT_BUFFER(buf), url);
			g_free(url);
		} else {
			seed_make_exception(ctx, e, "ArgumentError", "bad arg format");
		}
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_get_charset(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GString* charset;
	GObject* buf;

	if( argc != 1 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_get_charset expected 1 args");
	} else {
		buf = seed_value_to_object(ctx, argv[0], e);
		if( buf && GTK_TEXT_BUFFER(buf) ) {
			charset = g_self->app->doc_get_charset(GTK_TEXT_BUFFER(buf));
			return seed_value_from_string(ctx, charset->str, e);
		} else {
			seed_make_exception(ctx, e, "ArgumentError", "bad arg format");
		}
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_set_charset(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GObject* buf;
	gchar* charset = 0;

	if( argc != 2 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_set_charset expected 2 args");
	} else {
		buf = seed_value_to_object(ctx, argv[0], e);
		charset = seed_value_to_string(ctx, argv[1], e);
		if( buf && GTK_TEXT_BUFFER(buf) ) {
			g_self->app->doc_set_charset(GTK_TEXT_BUFFER(buf), charset);
			g_free(charset);
		} else {
			seed_make_exception(ctx, e, "ArgumentError", "bad arg format");
		}
	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_doc_get_view_from_page_num(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GtkTextView* view;
	gint page_num = 0;

	if( argc != 1 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_get_view_from_page_num expected 1 args");
	} else {
		page_num = seed_value_to_int(ctx, argv[0], e);
		view = g_self->app->doc_get_view_from_page_num(page_num);
		if( view )
			return seed_value_from_object(ctx, G_OBJECT(view), e);
	}
	return seed_make_null(ctx);
}

static SeedValue js_wrapper_doc_get_buffer_from_page_num(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GtkTextBuffer* buf;
	gint page_num = 0;

	if( argc != 1 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_get_buffer_from_page_num expected 1 args");
	} else {
		page_num = seed_value_to_int(ctx, argv[0], e);
		buf = g_self->app->doc_get_buffer_from_page_num(page_num);
		if( buf )
			return seed_value_from_object(ctx, G_OBJECT(buf), e);
	}
	return seed_make_null(ctx);
}

static SeedValue js_wrapper_doc_find_page_from_url(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	gint res = -1;
	gchar* url = 0;

	if( argc != 1 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_find_page_from_url expected 1 args");
		return seed_make_undefined(ctx);
	}

	url = seed_value_to_string(ctx, argv[0], e);
	if( url ) {
		res = g_self->app->doc_find_page_from_url(url);
		g_free(url);
	}
	return seed_value_from_int(ctx, res, e);
}

static SeedValue js_wrapper_doc_new(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	gint page_num = -1;
	if( argc != 0 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_new expected 0 args");
		return seed_make_undefined(ctx);
	}

	page_num = g_self->app->doc_new();
	return seed_value_from_int(ctx, page_num, e);
}

static SeedValue js_wrapper_doc_open(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	gboolean res;
	gchar* url = 0;
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
	g_free(url);

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

static SeedValue js_wrapper_send_focus_change(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GObject* widget = 0;
	gboolean force_in = FALSE;

	if( argc != 2 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.doc_close_all expected 2 args");
	} else {
		widget = seed_value_to_object(ctx, argv[0], e);
		force_in = seed_value_to_boolean(ctx, argv[1], e);

		if( widget && GTK_WIDGET(widget) )
			g_self->app->send_focus_change(GTK_WIDGET(widget), force_in);
		else
			seed_make_exception(ctx, e, "ArgumentError", "bad arg format");

	}
	return seed_make_undefined(ctx);
}

static SeedValue js_wrapper_active_panel_page(SeedContext ctx, SeedObject fn, SeedObject self, gsize argc, const SeedValue argv[], SeedException* e) {
	GObject* nb = 0;
	gint page_num = 0;

	if( argc != 2 ) {
		seed_make_exception(ctx, e, "ArgumentError", "puss.active_panel_page expected 2 args");
	} else {
		nb = seed_value_to_object(ctx, argv[0], e);
		page_num = seed_value_to_int(ctx, argv[1], e);

		if( nb && GTK_NOTEBOOK(nb) )
			g_self->app->active_panel_page(GTK_NOTEBOOK(nb), page_num);
		else
			seed_make_exception(ctx, e, "ArgumentError", "bad arg format");
	}
	return seed_make_undefined(ctx);
}

// puss option manager
/*
static PyObject* py_wrapper_option_reg(PyObject* self, PyObject* args) {
	const char* group = 0;
	const char* key = 0;
	const char* default_value = 0;
	const Option* option = 0;
	if( !PyArg_ParseTuple(args, "zzz:option_reg", &group, &key, &default_value))
		return 0;

	option = g_self->app->option_reg(group, key, default_value);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* py_wrapper_option_set(PyObject* self, PyObject* args) {
	const Option* option;
	const char* group = 0;
	const char* key = 0;
	const char* value = 0;

	if( !PyArg_ParseTuple(args, "zz|z:option_set", &group, &key, &value))
		return 0;

	option = g_self->app->option_find(group, key);
	if( option )
		g_self->app->option_set(option, value ? value : option->default_value);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* py_wrapper_option_get(PyObject* self, PyObject* args) {
	const Option* option;
	const char* group = 0;
	const char* key = 0;

	if( !PyArg_ParseTuple(args, "zz:option_get", &group, &key))
		return 0;

	option = g_self->app->option_find(group, key);
	if( option )
		return PyString_FromString(option->value);

	Py_INCREF(Py_None);
	return Py_None;
}

static void py_option_changed_wrapper(const Option* option, const gchar* old, PyObject* cb) {
	PyObject* res;

	g_assert( PyCallable_Check(cb) );

	res = PyObject_CallFunction(cb, "zzzz", option->group, option->key, option->value, old);
	if( !res ) {
		PyErr_Print();
		PyErr_Clear();
	}

	Py_XDECREF(res);
}

static PyObject* py_wrapper_option_monitor_reg(PyObject* self, PyObject* args) {
	const char* group = 0;
	const char* key = 0;
	PyObject* cb = 0;
	const Option* option;
	void* handler;

	if( !PyArg_ParseTuple(args, "zzN:option_monitor_reg", &group, &key, &cb))
		return 0;

	if( !cb || !PyCallable_Check(cb) ) {
		PyErr_SetString(PyExc_TypeError, "arg(monitor) not callable!");
		return 0;
	}

	option = g_self->app->option_find(group, key);
	if( !option ) {
		PyErr_SetString(PyExc_Exception, "reg option monitor error, not find option!");
		return 0;
	}

	handler = g_self->app->option_monitor_reg(option, (OptionChanged)&py_option_changed_wrapper, cb, (GFreeFunc)&py_object_decref);
	Py_INCREF(cb);

	return PyCObject_FromVoidPtr(handler, 0);
}

static PyObject* py_wrapper_option_monitor_unreg(PyObject* self, PyObject* args) {
	PyObject* handler;
	if( !PyArg_ParseTuple(args, "O:option_monitor_unreg", &handler))
		return 0;

	g_self->app->option_monitor_unreg(PyCObject_AsVoidPtr(handler));

	Py_INCREF(Py_None);
	return Py_None;
}
*/
// option setup
/*
static GtkWidget* py_option_setup_widget_creator_wrapper(gpointer tag) {
	GtkWidget* res = 0;
	PyObject* cb = (PyObject*)tag;
	if( cb && PyCallable_Check(cb) ) {
		PyObject* retval = PyObject_CallFunction(cb, "");
		if( retval ) {
			res = GTK_WIDGET(pygobject_get(retval));
			g_object_ref(res);
			Py_XDECREF(retval);
			
		} else {
			PyErr_Print();
			PyErr_Clear();
		}
	}

	return res;
}

static PyObject* py_wrapper_option_setup_reg(PyObject* self, PyObject* args) {
	const char* id = 0;
	const char* name = 0;
	PyObject* cb = 0;
	gboolean res;

	if( !PyArg_ParseTuple(args, "zzO:option_setup_reg", &id, &name, &cb))
		return 0;

	if( !cb || !PyCallable_Check(cb) ) {
		Py_XDECREF(cb);
		PyErr_SetString(PyExc_TypeError, "arg(creator) not callable!");
		return 0;
	}

	res = g_self->app->option_setup_reg(id, name, py_option_setup_widget_creator_wrapper, cb, (GDestroyNotify)py_object_decref);
	if( res ) {
		Py_INCREF(cb);
	}

	return PyBool_FromLong(res);
}

static PyObject* py_wrapper_option_setup_unreg(PyObject* self, PyObject* args) {
	const char* id = 0;
	if( !PyArg_ParseTuple(args, "z:option_setup_unreg", &id))
		return 0;

	g_self->app->option_setup_unreg(id);

	Py_INCREF(Py_None);
	return Py_None;
}
*/
// panels
/*
static PyObject* py_wrapper_panel_append(PyObject* self, PyObject* args) {
	GtkWidget* panel = 0;
	GtkWidget* tab_label = 0;
	gchar* id = 0;
	int default_pos = PUSS_PANEL_POS_BOTTOM;

	if( !PyArg_ParseTuple(args, "O&O&z|i:panel_append", &widget_convert, &panel, &widget_convert, &tab_label, &id, &default_pos) )
		return 0;

	g_self->app->panel_append(panel, tab_label, id, default_pos);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* py_wrapper_panel_remove(PyObject* self, PyObject* args) {
	GtkWidget* panel = 0;

	if( !PyArg_ParseTuple(args, "O&:panel_remove", &widget_convert, &panel) )
		return 0;

	g_self->app->panel_remove(panel);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* py_wrapper_panel_get_pos(PyObject* self, PyObject* args) {
	GtkWidget* panel = 0;
	GtkNotebook* parent = 0;
	gint page_num = 0;

	if( !PyArg_ParseTuple(args, "O&:panel_get_pos", &widget_convert, &panel) )
		return 0;

	if( !g_self->app->panel_get_pos(panel, &parent, &page_num) ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return Py_BuildValue("(Ni)", pygobject_new(G_OBJECT(parent)), page_num);
}
*/

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

	seed_create_function(engine->context, "doc_get_url",		&js_wrapper_doc_get_url, g_self->js_puss);
	seed_create_function(engine->context, "doc_set_url",		&js_wrapper_doc_set_url, g_self->js_puss);
	seed_create_function(engine->context, "doc_get_charset",	&js_wrapper_doc_get_charset, g_self->js_puss);
	seed_create_function(engine->context, "doc_get_charset",	&js_wrapper_doc_set_charset, g_self->js_puss);
	seed_create_function(engine->context, "doc_get_view_from_page_num",		&js_wrapper_doc_get_view_from_page_num, g_self->js_puss);
	seed_create_function(engine->context, "doc_get_buffer_from_page_num",	&js_wrapper_doc_get_buffer_from_page_num, g_self->js_puss);
	seed_create_function(engine->context, "doc_find_page_from_url",			&js_wrapper_doc_find_page_from_url, g_self->js_puss);
	seed_create_function(engine->context, "doc_new",			&js_wrapper_doc_new, g_self->js_puss);
	seed_create_function(engine->context, "doc_open",			&js_wrapper_doc_open, g_self->js_puss);
	seed_create_function(engine->context, "doc_locate",			&js_wrapper_doc_locate, g_self->js_puss);
	seed_create_function(engine->context, "doc_save_current",	&js_wrapper_doc_save_current, g_self->js_puss);
	seed_create_function(engine->context, "doc_close_current",	&js_wrapper_doc_close_current, g_self->js_puss);
	seed_create_function(engine->context, "doc_save_all",		&js_wrapper_doc_save_all, g_self->js_puss);
	seed_create_function(engine->context, "doc_close_all",		&js_wrapper_doc_close_all, g_self->js_puss);

	seed_create_function(engine->context, "send_focus_change",	&js_wrapper_send_focus_change, g_self->js_puss);
	seed_create_function(engine->context, "active_panel_page",	&js_wrapper_active_panel_page, g_self->js_puss);

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

