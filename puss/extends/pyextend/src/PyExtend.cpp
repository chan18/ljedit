// PyExtend.cpp
//

#include "PyExtend.h"
#include "IPuss.h"

#ifdef WIN32
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

#include <pygobject.h>
#include <pygtk/pygtk.h>

int app_convert(PyObject* py_obj, Puss** papp) {
	if( !PyCObject_Check(py_obj) ) {
		PyErr_SetString(PyExc_TypeError,  "need puss app cobject");
		return 0;
	}

	*papp = (Puss*)PyCObject_AsVoidPtr(py_obj);
	return 1;
}

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
	Puss* app = 0;
	GtkTextBuffer* buf = 0;
	if(!PyArg_ParseTuple(args, "O&O&:py_wrapper_doc_get_url", &app_convert, &app, &buf_convert, &buf))
		return 0;

	GString* url = app->api->doc_get_url(buf);
	if( !url ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(url->str);
}

PyObject* py_wrapper_doc_set_url(PyObject* self, PyObject* args) {
	Puss* app = 0;
	GtkTextBuffer* buf = 0;
	const gchar* url = 0;
	if(!PyArg_ParseTuple(args, "O&O&s:py_wrapper_doc_set_url", &app_convert, &app, &buf_convert, &buf, &url))
		return 0;

	app->api->doc_set_url(buf, url);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_get_charset(PyObject* self, PyObject* args) {
	Puss* app = 0;
	GtkTextBuffer* buf = 0;
	if(!PyArg_ParseTuple(args, "O&O&:py_wrapper_doc_get_charset", &app_convert, &app, &buf_convert, &buf))
		return 0;

	GString* charset = app->api->doc_get_charset(buf);
	if( !charset ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(charset->str);
}

PyObject* py_wrapper_doc_set_charset(PyObject* self, PyObject* args) {
	Puss* app = 0;
	GtkTextBuffer* buf = 0;
	const gchar* charset = 0;
	if(!PyArg_ParseTuple(args, "O&O&s:py_wrapper_doc_set_url", &app_convert, &app, &buf_convert, &buf, &charset))
		return 0;

	app->api->doc_set_charset(buf, charset);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_get_view_from_page_num(PyObject* self, PyObject* args) {
	Puss* app = 0;
	gint page_num = 0;
	if(!PyArg_ParseTuple(args, "O&i:py_wrapper_doc_get_view_from_page_num", &app_convert, &app, &page_num))
		return 0;

	GtkTextView* view = app->api->doc_get_view_from_page_num(app, page_num);
	if( !view ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* py_view = pygobject_new(G_OBJECT(view));
	return py_view;
}

PyObject* py_wrapper_doc_get_buffer_from_page_num(PyObject* self, PyObject* args) {
	Puss* app = 0;
	gint page_num = 0;
	if(!PyArg_ParseTuple(args, "O&i:py_wrapper_doc_get_buffer_from_page_num", &app_convert, &app, &page_num))
		return 0;

	GtkTextBuffer* buf = app->api->doc_get_buffer_from_page_num(app, page_num);
	if( !buf ) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* py_buf = pygobject_new(G_OBJECT(buf));
	return py_buf;
}

PyObject* py_wrapper_doc_find_page_from_url(PyObject* self, PyObject* args) {
	Puss* app = 0;
	const char* url = 0;
	if(!PyArg_ParseTuple(args, "O&s:py_wrapper_doc_find_page_from_url", &app_convert, &app, &url))
		return 0;

	gint res = app->api->doc_find_page_from_url(app, url);
	return PyInt_FromLong((long)res);
}

PyObject* py_wrapper_doc_new(PyObject* self, PyObject* args) {
	Puss* app = 0;
	if(!PyArg_ParseTuple(args, "O&:py_wrapper_doc_new", &app_convert, &app))
		return 0;

	app->api->doc_new(app);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_open(PyObject* self, PyObject* args) {
	Puss* app = 0;
	const char* url = 0;
	int line = 0;
	int offset = 0;
	if(!PyArg_ParseTuple(args, "O&zii:py_wrapper_doc_open", &app_convert, &app, &url, &line, &offset))
		return 0;

	gboolean res = app->api->doc_open(app, url, line, offset);
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_doc_locate(PyObject* self, PyObject* args) {
	Puss* app = 0;
	int page_num = 0;
	int line = 0;
	int offset = 0;
	int add_pos_locate = 0;
	if(!PyArg_ParseTuple(args, "O&iiii:py_wrapper_doc_locate", &app_convert, &app, &page_num, &line, &offset, &add_pos_locate))
		return 0;

	gboolean res = app->api->doc_locate(app, page_num, line, offset, add_pos_locate);
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_doc_save_current(PyObject* self, PyObject* args) {
	Puss* app = 0;
	int save_as = 0;
	if(!PyArg_ParseTuple(args, "O&i:py_wrapper_doc_save_current", &app_convert, &app, &save_as))
		return 0;

	app->api->doc_save_current(app, (gboolean)save_as);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_close_current(PyObject* self, PyObject* args) {
	Puss* app = 0;
	if(!PyArg_ParseTuple(args, "O&:py_wrapper_doc_close_current", &app_convert, &app))
		return 0;

	gboolean res = app->api->doc_close_current(app);
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_doc_save_all(PyObject* self, PyObject* args) {
	Puss* app = 0;
	if(!PyArg_ParseTuple(args, "O&:py_wrapper_doc_save_all", &app_convert, &app))
		return 0;

	app->api->doc_save_all(app);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_doc_close_all(PyObject* self, PyObject* args) {
	Puss* app = 0;
	if(!PyArg_ParseTuple(args, "O&:py_wrapper_doc_close_all", &app_convert, &app))
		return 0;

	gboolean res = app->api->doc_close_all(app);
	return PyBool_FromLong((long)res);
}

PyObject* py_wrapper_send_focus_change(PyObject* self, PyObject* args) {
	Puss* app = 0;
	GtkWidget* widget = 0;
	int force_in = 0;
	if(!PyArg_ParseTuple(args, "O&O&i:py_wrapper_doc_close_all", &app_convert, &app, &widget_convert, &widget, &force_in))
		return 0;

	app->api->send_focus_change(widget, force_in);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* py_wrapper_active_panel_page(PyObject* self, PyObject* args) {
	Puss* app = 0;
	GtkNotebook* nb = 0;
	gint page_num = 0;
	if(!PyArg_ParseTuple(args, "O&O&i:py_wrapper_doc_close_all", &app_convert, &app, &notebook_convert, &nb, &page_num))
		return 0;

	app->api->active_panel_page(nb, page_num);
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

	, { NULL, NULL, 0, NULL } };


//----------------------------------------------------------------
// PyExtend implements

struct PyExtend {
	Puss*		app;
	PyObject*	py_gobject;
	PyObject*	py_gtk;
	PyObject*	py_impl;
};

gboolean init_pygtk_library(PyExtend* self) {
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

	PyObject* cobject = PyDict_GetItemString(PyModule_GetDict(self->py_gtk), "_PyGtk_API");
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

	{
		// TODO : remove those!!!!

		PyModule_AddObject(py_puss, "__app",		PyCObject_FromVoidPtr(self->app, 0));

		PyModule_AddObject(py_puss, "main_window",	pygobject_new(G_OBJECT(puss_get_main_window(self->app))));

		PyModule_AddObject(py_puss, "ui_manager",	pygobject_new(G_OBJECT(puss_get_ui_manager(self->app))));
		PyModule_AddObject(py_puss, "doc_panel",	pygobject_new(G_OBJECT(puss_get_doc_panel(self->app))));
		PyModule_AddObject(py_puss, "left_panel",	pygobject_new(G_OBJECT(puss_get_left_panel(self->app))));
		PyModule_AddObject(py_puss, "right_panel",	pygobject_new(G_OBJECT(puss_get_right_panel(self->app))));
		PyModule_AddObject(py_puss, "bottom_panel",	pygobject_new(G_OBJECT(puss_get_bottom_panel(self->app))));
		PyModule_AddObject(py_puss, "status_bar",	pygobject_new(G_OBJECT(puss_get_statusbar(self->app))));
	}

	gchar* extends_path = g_build_filename(self->app->module_path, "extends", NULL);
	{
		PyObject* py_sys_path = PySys_GetObject("path");
		PyObject* py_extends_path = PyString_FromString(extends_path);

		if( !PySequence_Contains(py_sys_path, py_extends_path) )
			PyList_Insert(py_sys_path, 0, py_extends_path);

		Py_DECREF(py_extends_path);
	}
	g_free(extends_path);

	self->py_impl = PyImport_ImportModule("pyextend");
	if( PyErr_Occurred() ) {
		PyErr_Print();
		PyErr_Clear();
		return FALSE;
	}

	return TRUE;
}

void load_python_extends(PyExtend* self) {
	PyObject* py_dict = PyModule_GetDict(self->py_impl);
	PyObject* py_load_method = PyDict_GetItemString(py_dict, "puss_load_python_extends");
	if( !PyCallable_Check(py_load_method) )
		return;

	PyObject* res = PyObject_CallFunction(py_load_method, 0);
	if( res ) {
		Py_DECREF(res);
		
	} else {
		PyErr_Print();
		PyErr_Clear();
	}
}

void unload_python_extends(PyExtend* self) {
	if( !self->py_impl )
		return;

	PyObject* py_dict = PyModule_GetDict(self->py_impl);
	PyObject* py_unload_method = PyDict_GetItemString(py_dict, "puss_unload_python_extends");
	if( !PyCallable_Check(py_unload_method) )
		return;

	PyObject* res = PyObject_CallFunction(py_unload_method, 0);
	if( res ) {
		Py_DECREF(res);
		
	} else {
		PyErr_Print();
		PyErr_Clear();
	}
}

PyExtend* puss_py_extend_create(Puss* app) {
	PyExtend* self = g_try_new0(PyExtend, 1);
	if( self ) {
		self->app = app;

		Py_Initialize();

		if( init_pygtk_library(self) && init_puss_module(self) )
			load_python_extends(self);
	}

	return self;
}

void puss_py_extend_destroy(PyExtend* self) {
	if( self ) {
		// unload python extends
		unload_python_extends(self);

		Py_XDECREF(self->py_impl);
		Py_XDECREF(self->py_gtk);
		Py_XDECREF(self->py_gobject);

		while( PyGC_Collect() )
			;	
			
		Py_Finalize();
	}
}

