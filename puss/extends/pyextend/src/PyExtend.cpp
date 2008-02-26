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

#include "PussPyAPI.wrapper"

struct PyExtend {
	Puss*		app;
	PyObject*	py_gobject;
	PyObject*	py_gtk;
	PyObject*	py_puss;
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

PyMethodDef puss_methods[] =
	{ { "__doc_get_url_buffer", &py_wrapper_doc_get_url_buffer, METH_VARARGS, NULL }
	, { "__doc_doc_get_buffer_from_page_num", &py_wrapper_doc_get_buffer_from_page_num, METH_VARARGS, NULL }
	, { "__doc_find_page_from_url",	&py_wrapper_doc_find_page_from_url, METH_VARARGS, NULL }
	, { "__doc_new", &py_wrapper_doc_new, METH_VARARGS, NULL }
	, { "__doc_open", &py_wrapper_doc_open, METH_VARARGS, NULL }
	, { "__doc_locate", &py_wrapper_doc_locate, METH_VARARGS, NULL }
	, { "__doc_save_current", &py_wrapper_doc_save_current, METH_VARARGS, NULL }
	, { "__doc_close_current", &py_wrapper_doc_close_current, METH_VARARGS, NULL }
	, { "__doc_save_all", &py_wrapper_doc_save_all, METH_VARARGS, NULL }
	, { "__doc_close_all", &py_wrapper_doc_close_all, METH_VARARGS, NULL }

	, { NULL, NULL, 0, NULL } };

gboolean init_puss_module(PyExtend* self) {
	self->py_puss = Py_InitModule("__puss", puss_methods);
	if( !self->py_puss )
		return FALSE;

	{
		PyModule_AddObject(self->py_puss, "__app",			PyCObject_FromVoidPtr(self->app, 0));

		PyModule_AddObject(self->py_puss, "main_window",	pygobject_new(G_OBJECT(self->app->main_window->window)));

		PyModule_AddObject(self->py_puss, "ui_manager",		pygobject_new(G_OBJECT(self->app->main_window->ui_manager)));
		PyModule_AddObject(self->py_puss, "doc_panel",		pygobject_new(G_OBJECT(self->app->main_window->doc_panel)));
		PyModule_AddObject(self->py_puss, "left_panel",		pygobject_new(G_OBJECT(self->app->main_window->left_panel)));
		PyModule_AddObject(self->py_puss, "right_panel",	pygobject_new(G_OBJECT(self->app->main_window->right_panel)));
		PyModule_AddObject(self->py_puss, "bottom_panel",	pygobject_new(G_OBJECT(self->app->main_window->bottom_panel)));
		PyModule_AddObject(self->py_puss, "status_bar",		pygobject_new(G_OBJECT(self->app->main_window->status_bar)));
	}

	gchar* extends_path = g_build_filename(self->app->module_path, "extends", NULL);
	{
		PyObject* py_sys = PyImport_ImportModule("sys");
		PyObject* py_dict = PyModule_GetDict(py_sys);
		PyObject* py_path = PyDict_GetItemString(py_dict, "path");
		PyObject* py_extends_path = PyString_FromString(extends_path);
		PyList_Append(py_path, py_extends_path);
		Py_DecRef(py_extends_path);
		Py_DecRef(py_sys);
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
		Py_DecRef(res);
		
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
		Py_DecRef(res);
		
	} else {
		PyErr_Print();
		PyErr_Clear();
	}
}

PyExtend* puss_py_extend_create(Puss* app) {
	Py_Initialize();

	PyExtend* self = (PyExtend*)g_malloc(sizeof(PyExtend));
	memset(self, 0, sizeof(PyExtend));
	self->app = app;

	if( init_pygtk_library(self) && init_puss_module(self) )
		load_python_extends(self);

	return self;
}

void puss_py_extend_destroy(PyExtend* self) {
	if( self ) {
		// unload python extends
		unload_python_extends(self);

		Py_XDECREF(self->py_impl);
		Py_XDECREF(self->py_puss);
		Py_XDECREF(self->py_gtk);
		Py_XDECREF(self->py_gobject);
	}

	Py_Finalize();
}

