// PyExtend.cpp
//

#include "PyExtend.h"

#ifdef G_OS_WIN32
	#ifdef _DEBUG
		#undef _DEBUG
		#include <Python.h>
		#define _DEBUG
	#else
		#include <Python.h>
	#endif
#else
	#include <Python.h>
#endif

static Puss* g_app = 0;

#include <pygobject.h>
#include <pygtk/pygtk.h>

//----------------------------------------------------------------
// !!! check type in puss.py

int buf_convert(PyObject* py_obj, GtkTextBuffer** pbuf)
	{ *pbuf = GTK_TEXT_BUFFER(pygobject_get(py_obj)); return 1; }

int widget_convert(PyObject* py_obj, GtkWidget** pwidget)
	{ *pwidget = GTK_WIDGET(pygobject_get(py_obj));	return 1; }

int notebook_convert(PyObject* py_obj, GtkNotebook** pnb)
	{ *pnb = GTK_NOTEBOOK(pygobject_get(py_obj)); return 1; }

//----------------------------------------------------------------
// IPuss wrappers

PyObject* py_wrapper_doc_get_url(PyObject* self, PyObject* args) {
	GString* url;
	GtkTextBuffer* buf = 0;
	if( !PyArg_ParseTuple(args, "O&:py_wrapper_doc_get_url", &buf_convert, &buf))
		return 0;

	url = g_app->doc_get_url(buf);
	if( !url ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(url->str);
}

PyObject* py_wrapper_doc_set_url(PyObject* self, PyObject* args) {
	GtkTextBuffer* buf = 0;
	const gchar* url = 0;
	if( !PyArg_ParseTuple(args, "O&s:py_wrapper_doc_set_url", &buf_convert, &buf, &url))
		return 0;

	g_app->doc_set_url(buf, url);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_get_charset(PyObject* self, PyObject* args) {
	GString* charset;
	GtkTextBuffer* buf = 0;
	if( !PyArg_ParseTuple(args, "O&:py_wrapper_doc_get_charset", &buf_convert, &buf))
		return 0;

	charset = g_app->doc_get_charset(buf);
	if( !charset ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(charset->str);
}

PyObject* py_wrapper_doc_set_charset(PyObject* self, PyObject* args) {
	GtkTextBuffer* buf = 0;
	const gchar* charset = 0;
	if( !PyArg_ParseTuple(args, "O&s:py_wrapper_doc_set_url", &buf_convert, &buf, &charset))
		return 0;

	g_app->doc_set_charset(buf, charset);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_get_view_from_page_num(PyObject* self, PyObject* args) {
	GtkTextView* view;
	PyObject* py_view;
	gint page_num = 0;
	if( !PyArg_ParseTuple(args, "i:py_wrapper_doc_get_view_from_page_num", &page_num))
		return 0;

	view = g_app->doc_get_view_from_page_num(page_num);
	if( !view ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	py_view = pygobject_new(G_OBJECT(view));
	return py_view;
}

PyObject* py_wrapper_doc_get_buffer_from_page_num(PyObject* self, PyObject* args) {
	GtkTextBuffer* buf;
	PyObject* py_buf;
	gint page_num = 0;
	if( !PyArg_ParseTuple(args, "i:py_wrapper_doc_get_buffer_from_page_num", &page_num))
		return 0;

	buf = g_app->doc_get_buffer_from_page_num(page_num);
	if( !buf ) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	py_buf = pygobject_new(G_OBJECT(buf));
	return py_buf;
}

PyObject* py_wrapper_doc_find_page_from_url(PyObject* self, PyObject* args) {
	gint res;
	const char* url = 0;
	if( !PyArg_ParseTuple(args, "s:py_wrapper_doc_find_page_from_url", &url))
		return 0;

	res = g_app->doc_find_page_from_url(url);
	return PyInt_FromLong((long)res);
}

PyObject* py_wrapper_doc_new(PyObject* self, PyObject* args) {
	g_app->doc_new();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_open(PyObject* self, PyObject* args) {
	gboolean res;
	const char* url = 0;
	int line = 0;
	int offset = 0;
	gboolean flag = FALSE;
	if( !PyArg_ParseTuple(args, "ziii:py_wrapper_doc_open", &url, &line, &offset, &flag))
		return 0;

	res = g_app->doc_open(url, line, offset, flag);
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_doc_locate(PyObject* self, PyObject* args) {
	gboolean res;
	int page_num = 0;
	int line = 0;
	int offset = 0;
	int add_pos_locate = 0;
	if( !PyArg_ParseTuple(args, "iiii:py_wrapper_doc_locate", &page_num, &line, &offset, &add_pos_locate))
		return 0;

	res = g_app->doc_locate(page_num, line, offset, add_pos_locate);
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_doc_save_current(PyObject* self, PyObject* args) {
	int save_as = 0;
	if( !PyArg_ParseTuple(args, "i:py_wrapper_doc_save_current", &save_as))
		return 0;

	g_app->doc_save_current((gboolean)save_as);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_close_current(PyObject* self, PyObject* args) {
	gboolean res;
	res = g_app->doc_close_current();
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_doc_save_all(PyObject* self, PyObject* args) {
	g_app->doc_save_all();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_close_all(PyObject* self, PyObject* args) {
	gboolean res;
	res = g_app->doc_close_all();
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_send_focus_change(PyObject* self, PyObject* args) {
	GtkWidget* widget = 0;
	int force_in = 0;
	if( !PyArg_ParseTuple(args, "O&i:py_wrapper_doc_close_all", &widget_convert, &widget, &force_in))
		return 0;

	g_app->send_focus_change(widget, force_in);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_active_panel_page(PyObject* self, PyObject* args) {
	GtkNotebook* nb = 0;
	gint page_num = 0;
	if( !PyArg_ParseTuple(args, "O&i:py_wrapper_doc_close_all", &notebook_convert, &nb, &page_num))
		return 0;

	g_app->active_panel_page(nb, page_num);
	Py_INCREF(Py_None);
	return Py_None;
}

// puss option manager

void py_object_free_wrapper(PyObject* cb) {
	Py_XDECREF(cb);
}

PyObject* py_wrapper_option_manager_find(PyObject* self, PyObject* args) {
	const Option* option;
	const char* group = 0;
	const char* key = 0;
	if( !PyArg_ParseTuple(args, "zz:py_wrapper_option_manager_find", &group, &key))
		return 0;

	option = g_app->option_manager_find(group, key);
	if( !option ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return Py_BuildValue("(zz)", option->value, option->default_value);
}

gboolean py_option_setter_wrapper(GtkWindow* parent, Option* option, PyObject* cb) {
	gboolean retval;
	PyObject* res;

	g_assert( PyCallable_Check(cb) );

	res = PyObject_CallFunction(cb, "Ozzzz", pygobject_new(G_OBJECT(parent)), option->group, option->key, option->value, option->default_value);
	if( !res ) {
		PyErr_Print();
		PyErr_Clear();
		return FALSE;
	}

	retval = FALSE;
	if( PyBool_Check(res) && res==Py_True )
		retval = TRUE;

	Py_DECREF(res);
	return retval;
}

PyObject* py_wrapper_option_manager_option_reg(PyObject* self, PyObject* args) {
	const char* group = 0;
	const char* key = 0;
	const char* default_value = 0;
	PyObject* cb = 0;
	const Option* option = 0;
	if( !PyArg_ParseTuple(args, "zzzN:py_wrapper_option_manager_option_reg", &group, &key, &default_value, &cb))
		return 0;

	if( !cb ) {
		option = g_app->option_manager_option_reg(group, key, default_value, 0, 0, 0);

	} else {
		if( PyCallable_Check(cb) ) {
			Py_INCREF(cb);
			option = g_app->option_manager_option_reg(group, key, default_value, (OptionSetter)&py_option_setter_wrapper, cb, (GFreeFunc)&py_object_free_wrapper);

		} else if( PyString_Check(cb) ) {
			gchar* tag = g_strdup(PyString_AS_STRING(cb));
			option = g_app->option_manager_option_reg(group, key, default_value, 0, tag, &g_free);
			if( !option )
				g_free(tag);
		}
	}

	if( !option ) {
		PyErr_SetString(PyExc_TypeError, "setter must None, Str or Function");
		return 0;
	}

	return Py_BuildValue("(zz)", option->value, option->default_value);
}


void py_option_changed_wrapper(const Option* option, PyObject* cb) {
	PyObject* res;

	g_assert( PyCallable_Check(cb) );

	res = PyObject_CallFunction(cb, "zzz", option->group, option->key, option->value);
	if( !res ) {
		PyErr_Print();
		PyErr_Clear();
	}

	Py_XDECREF(res);
}

PyObject* py_wrapper_option_manager_monitor_reg(PyObject* self, PyObject* args) {
	const char* group = 0;
	const char* key = 0;
	PyObject* cb = 0;
	const Option* option;
	if( !PyArg_ParseTuple(args, "zzN:py_wrapper_option_manager_monitor_reg", &group, &key, &cb))
		return 0;

	if( !cb || !PyCallable_Check(cb) ) {
		PyErr_SetString(PyExc_TypeError, "arg(monitor) not callable!");
		return 0;
	}

	option = g_app->option_manager_find(group, key);
	if( !g_app->option_manager_monitor_reg(option, (OptionChanged)&py_option_changed_wrapper, cb, (GFreeFunc)&py_object_free_wrapper) ) {
		PyErr_SetString(PyExc_Exception, "reg option monitor error, not find option!");
		return 0;
	}

	Py_INCREF(cb);
	Py_INCREF(Py_None);
	return Py_None;
}

PyMethodDef puss_methods[] =
	{ { "__doc_get_url", &py_wrapper_doc_get_url, METH_VARARGS, NULL }
	, { "__doc_set_url", &py_wrapper_doc_set_url, METH_VARARGS, NULL }
	, { "__doc_get_charset", &py_wrapper_doc_get_charset, METH_VARARGS, NULL }
	, { "__doc_set_charset", &py_wrapper_doc_set_charset, METH_VARARGS, NULL }

	, { "__doc_doc_get_view_from_page_num", &py_wrapper_doc_get_view_from_page_num, METH_VARARGS, NULL }
	, { "__doc_doc_get_buffer_from_page_num", &py_wrapper_doc_get_buffer_from_page_num, METH_VARARGS, NULL }

	, { "__doc_find_page_from_url",	&py_wrapper_doc_find_page_from_url, METH_VARARGS, NULL }
	, { "__doc_new", &py_wrapper_doc_new, METH_VARARGS, NULL }
	, { "__doc_open", &py_wrapper_doc_open, METH_VARARGS, NULL }
	, { "__doc_locate", &py_wrapper_doc_locate, METH_VARARGS, NULL }
	, { "__doc_save_current", &py_wrapper_doc_save_current, METH_VARARGS, NULL }
	, { "__doc_close_current", &py_wrapper_doc_close_current, METH_VARARGS, NULL }
	, { "__doc_save_all", &py_wrapper_doc_save_all, METH_VARARGS, NULL }
	, { "__doc_close_all", &py_wrapper_doc_close_all, METH_VARARGS, NULL }

	, { "__send_focus_change", &py_wrapper_send_focus_change, METH_VARARGS, NULL }
	, { "__active_panel_page", &py_wrapper_active_panel_page, METH_VARARGS, NULL }

	, { "__option_manager_find", &py_wrapper_option_manager_find, METH_VARARGS, NULL }
	, { "__option_manager_option_reg", &py_wrapper_option_manager_option_reg, METH_VARARGS, NULL }
	, { "__option_manager_monitor_reg", &py_wrapper_option_manager_monitor_reg, METH_VARARGS, NULL }

	, { NULL, NULL, 0, NULL } };


//----------------------------------------------------------------
// PyExtend implements

struct _PyExtend {
	PyObject*	py_gobject;
	PyObject*	py_gtk;
};

gboolean init_pygtk_library(PyExtend* self) {
	PyObject* cobject;

	self->py_gobject = pygobject_init(-1, -1, -1);
	if( !self->py_gobject ) {
		PyErr_Print();
		PyErr_Clear();
		return FALSE;
	}

	self->py_gtk = PyImport_ImportModule("gtk");
	if( !self->py_gtk ) {
		PyErr_Print();
		PyErr_Clear();
		return FALSE;
	}

	cobject = PyDict_GetItemString(PyModule_GetDict(self->py_gtk), "_PyGtk_API");
	if( PyCObject_Check(cobject) )
		_PyGtk_API = (struct _PyGtk_FunctionStruct*)PyCObject_AsVoidPtr(cobject);
	else
		return FALSE;

	if( PyErr_Occurred() ) {
		PyErr_Print();
		PyErr_Clear();
		return FALSE;
	}

	return TRUE;
}

gboolean init_puss_module(PyExtend* self) {
	PyObject* py_puss = Py_InitModule("__puss", puss_methods);
	if( !py_puss )
		return FALSE;

	PyModule_AddObject( py_puss, "__builder", pygobject_new(G_OBJECT(g_app->get_ui_builder())) );

	{
		PyObject* py_sys_path = PySys_GetObject("path");
		PyObject* py_plugins_path = PyString_FromString(g_app->get_plugins_path());

		if( !PySequence_Contains(py_sys_path, py_plugins_path) )
			PyList_Insert(py_sys_path, 0, py_plugins_path);

		Py_DECREF(py_plugins_path);
	}

	return TRUE;
}

// [Python-Dev] Signals, threads, blocking C functions
// CTRL+c signal bug
// 
static gboolean python_do_pending_calls(gpointer data) {
	gboolean quit = FALSE;

	pyg_block_threads();
	if( PyErr_CheckSignals() == -1 ) {
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		quit = TRUE;
	}
	pyg_unblock_threads();

	if (quit && gtk_main_level() > 0)
		gtk_main_quit();

	return TRUE;
}

gpointer py_plugin_load(const gchar* plugin_id, GKeyFile* keyfile, PyExtend* self) {
	PyObject* py_res;
	PyObject* py_plugin;
	PyObject* py_dict;
	PyObject* py_active_method;

	py_plugin = PyImport_ImportModule(plugin_id);
	if( !py_plugin ) {
		PyErr_Print();
		PyErr_Clear();
		return 0;
	}

	py_dict = PyModule_GetDict(py_plugin);
	py_active_method = PyDict_GetItemString(py_dict, "puss_plugin_active");

	if( PyCallable_Check(py_active_method) ) {
		py_res = PyObject_CallFunction(py_active_method, 0);
		if( py_res ) {
			Py_DECREF(py_res);
		} else {
			PyErr_Print();
			PyErr_Clear();
		}
	}

	return py_plugin;
}

void py_plugin_unload(gpointer plugin, PyExtend* self) {
	PyObject* py_res;
	PyObject* py_plugin;
	PyObject* py_dict;
	PyObject* py_deactive_method;

	if( plugin==0 )
		return;

	py_plugin = (PyObject*)plugin;
	py_dict = PyModule_GetDict(py_plugin);
	py_deactive_method = PyDict_GetItemString(py_dict, "puss_plugin_deactive");
	if( !PyCallable_Check(py_deactive_method) )
		return;

	py_res = PyObject_CallFunction(py_deactive_method, 0);
	Py_DECREF(py_plugin);

	if( py_res ) {
		Py_DECREF(py_res);
		
	} else {
		PyErr_Print();
		PyErr_Clear();
	}
}

PyExtend* puss_py_extend_create(Puss* app) {
	PyExtend* self = g_try_new0(PyExtend, 1);
	if( self ) {
		g_app = app;

		Py_Initialize();

		g_timeout_add(100, python_do_pending_calls, 0);

		if( init_pygtk_library(self) && init_puss_module(self) )
			g_app->plugin_engine_regist( "python"
				, py_plugin_load
				, py_plugin_unload
				, 0
				, self );
	}

	return self;
}

void puss_py_extend_destroy(PyExtend* self) {
	if( self ) {
		Py_XDECREF(self->py_gtk);
		Py_XDECREF(self->py_gobject);

		while( PyGC_Collect() )
			;

		Py_Finalize();
	}
}

