// LJEditorPythonPluginEngineImpl.cpp
// 

// !!! compile on win32 DEBUG need : modify pyconfig.h
// 
// !!! use python25_d.lib/dll can not use pygobject
// 
// 1. find 
//        #pragma comment(lib,"python25_d.lib")
//    replace with
//        #pragma comment(lib,"python25.lib")
// 
// 2. find
//        #ifdef _DEBUG
//        #	define Py_DEBUG
//        #endif
//    remove it
// 

#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "LJEditorImpl.h"

//-------------------------------------------------------------------
// 
PyObject* ljedit_doc_manager_create_new_file(PyObject* self, PyObject* args) {
	PyObject* py_c_ljedit = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_create_new_file", &py_c_ljedit))
		return NULL;

	LJEditorImpl* ljedit = (LJEditorImpl*)PyCObject_AsVoidPtr(py_c_ljedit);
	if( ljedit==0 )
		return 0;

	ljedit->main_window().doc_manager().create_new_file();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_open_file(PyObject* self, PyObject* args) {
	PyObject* py_c_ljedit = 0;
	const char* filepath = 0;
	int line = 0;
	if(!PyArg_ParseTuple(args, "Osi:ljedit_doc_manager_open_file", &py_c_ljedit, &filepath, &line))
		return NULL;

	LJEditorImpl* ljedit = (LJEditorImpl*)PyCObject_AsVoidPtr(py_c_ljedit);
	if( ljedit==0 )
		return 0;

	ljedit->main_window().doc_manager().open_file(filepath, line);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_save_current_file(PyObject* self, PyObject* args) {
	PyObject* py_c_ljedit = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_save_current_file", &py_c_ljedit))
		return NULL;

	LJEditorImpl* ljedit = (LJEditorImpl*)PyCObject_AsVoidPtr(py_c_ljedit);
	if( ljedit==0 )
		return 0;

	ljedit->main_window().doc_manager().save_current_file();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_close_current_file(PyObject* self, PyObject* args) {
	PyObject* py_c_ljedit = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_close_current_file", &py_c_ljedit))
		return NULL;

	LJEditorImpl* ljedit = (LJEditorImpl*)PyCObject_AsVoidPtr(py_c_ljedit);
	if( ljedit==0 )
		return 0;

	ljedit->main_window().doc_manager().close_current_file();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_save_all_files(PyObject* self, PyObject* args) {
	PyObject* py_c_ljedit = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_save_all_files", &py_c_ljedit))
		return NULL;

	LJEditorImpl* ljedit = (LJEditorImpl*)PyCObject_AsVoidPtr(py_c_ljedit);
	if( ljedit==0 )
		return 0;

	ljedit->main_window().doc_manager().save_all_files();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_close_all_files(PyObject* self, PyObject* args) {
	PyObject* py_c_ljedit = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_close_all_files", &py_c_ljedit))
		return NULL;

	LJEditorImpl* ljedit = (LJEditorImpl*)PyCObject_AsVoidPtr(py_c_ljedit);
	if( ljedit==0 )
		return 0;

	ljedit->main_window().doc_manager().close_all_files();

	Py_INCREF(Py_None);
	return Py_None;
}

PyMethodDef ljedit_methods[] = {
	{ "ljedit_doc_manager_create_new_file",    ljedit_doc_manager_create_new_file,    METH_VARARGS, "ljedit_doc_manager_create_new_file." },
	{ "ljedit_doc_manager_open_file",          ljedit_doc_manager_open_file,          METH_VARARGS, "ljedit_doc_manager_open_file." },
	{ "ljedit_doc_manager_save_current_file",  ljedit_doc_manager_save_current_file,  METH_VARARGS, "ljedit_doc_manager_save_current_file." },
	{ "ljedit_doc_manager_close_current_file", ljedit_doc_manager_close_current_file, METH_VARARGS, "ljedit_doc_manager_close_current_file." },
	{ "ljedit_doc_manager_save_all_files",     ljedit_doc_manager_save_all_files,     METH_VARARGS, "ljedit_doc_manager_save_all_files." },
	{ "ljedit_doc_manager_close_all_files",    ljedit_doc_manager_close_all_files,    METH_VARARGS, "ljedit_doc_manager_close_all_files." },
	{NULL, NULL, 0, NULL}
};

//-------------------------------------------------------------------
// 
void __ljed_init_pygobject__() { init_pygobject(); }
void __ljed_init_pygtk__()     { init_pygtk(); }

bool ljed_init_pygtk_library() {
	__ljed_init_pygobject__();
	if( PyErr_Occurred() ) {
		PyErr_Print();
		PyErr_Clear();
		return false;
	}

	__ljed_init_pygtk__();
	if( PyErr_Occurred() ) {
		PyErr_Print();
		PyErr_Clear();
		return false;
	}

	return true;
}

inline PyObject* ljed_pygobject_new(Glib::Object& o)
	{ return pygobject_new(o.gobj()); }

PyObject* ljed_py_create_main_window() {
	MainWindow& main_window = LJEditorImpl::self().main_window();
	PyObject* py_main_window = ljed_pygobject_new(main_window);
	if( py_main_window==0 )
		return 0;

	PyObject* py_ui_manager   = pygobject_new( main_window.ui_manager()->::Glib::Object::gobj() );
	PyObject* py_action_group = pygobject_new( main_window.action_group()->::Glib::Object::gobj() );
	PyObject* py_left_panel   = ljed_pygobject_new( main_window.left_panel() );
	PyObject* py_doc_manager  = ljed_pygobject_new( main_window.doc_manager() );
	PyObject* py_right_panel  = ljed_pygobject_new( main_window.right_panel() );
	PyObject* py_bottom_panel = ljed_pygobject_new( main_window.bottom_panel() );
	PyObject* py_status_bar   = ljed_pygobject_new( main_window.status_bar() );

	if( py_ui_manager==0
		|| py_action_group==0
		|| py_left_panel==0
		|| py_doc_manager==0
		|| py_right_panel==0
		|| py_bottom_panel==0
		|| py_status_bar==0 )
	{
		return 0;
	}

	PyObject_SetAttrString(py_main_window, "ui_manager",   py_ui_manager);
	PyObject_SetAttrString(py_main_window, "action_group", py_action_group);
	PyObject_SetAttrString(py_main_window, "left_panel",   py_left_panel);
	PyObject_SetAttrString(py_main_window, "doc_manager", py_ui_manager);
	PyObject_SetAttrString(py_main_window, "right_panel",  py_right_panel);
	PyObject_SetAttrString(py_main_window, "bottom_panel", py_bottom_panel);
	PyObject_SetAttrString(py_main_window, "status_bar",   py_status_bar);

	return py_main_window;
}

class PythonPluginManager {
public:
	static PythonPluginManager& self() {
		static PythonPluginManager me_;
		return me_;
	}

	bool init() {
		if( !ljed_init_pygtk_library() )
			return false;

		PyObject* py_ljedit = Py_InitModule("ljedit", ljedit_methods);
		if( py_ljedit==0 )
			return false;

		PyObject* py_c_ljedit = ::PyCObject_FromVoidPtr(&LJEditorImpl::self(), 0);
		PyModule_AddObject(py_ljedit, "__c_ljedit", py_c_ljedit);

		PyObject* py_main_window = ljed_py_create_main_window();
		if( py_main_window==0 )
			return false;
		PyModule_AddObject(py_ljedit, "main_window", py_main_window);

		py_impl_ = PyImport_ImportModule("_ljedit_py");

		return py_impl_!=0;
	}

	void load_plugins() {
		if( py_impl_==0 )
			return;

		PyObject* dict = PyModule_GetDict(py_impl_);
		if( dict==0 )
			return;

		PyObject* load_plugins = PyDict_GetItemString(dict, "load_plugins");
		if( load_plugins==0 )
			return;

		PyObject* result = PyObject_CallFunction(load_plugins, 0);
		if( result==0 )
			PyErr_Clear();
		Py_XDECREF(result);
	}

	void unload_plugins() {
		if( py_impl_==0 )
			return;

		PyObject* dict = PyModule_GetDict(py_impl_);
		if( dict==0 )
			return;

		PyObject* unload_plugins = PyDict_GetItemString(dict, "unload_plugins");
		if( unload_plugins==0 )
			return;

		PyObject* result = PyObject_CallFunction(unload_plugins, 0);
		if( result==0 )
			PyErr_Clear();
		Py_XDECREF(result);
	}

private:
	PythonPluginManager() : py_impl_(0) {}
	~PythonPluginManager() {}

	PyObject* py_impl_;
};

void ljed_start_python_plugin_engine() {
	::Py_Initialize();

	if( !PythonPluginManager::self().init() ) {
		PyErr_Clear();
		return;
	}

	PythonPluginManager::self().load_plugins();
}

void ljed_stop_python_plugin_engine() {
	PythonPluginManager::self().unload_plugins();

	::Py_Finalize();
}

