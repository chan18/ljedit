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
// wrapper doc manager
// 
PyObject* ljedit_doc_manager_create_new_file(PyObject* self, PyObject* args) {
	PyObject* py_c_doc_manager = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_create_new_file", &py_c_doc_manager))
		return 0;

	DocManager* dm = (DocManager*)PyCObject_AsVoidPtr(py_c_doc_manager);
	if( dm==0 )
		return 0;

	dm->create_new_file();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_open_file(PyObject* self, PyObject* args) {
	PyObject* py_c_doc_manager = 0;
	const char* filepath = 0;
	int line = 0;
	if(!PyArg_ParseTuple(args, "Osi:ljedit_doc_manager_open_file", &py_c_doc_manager, &filepath, &line))
		return 0;

	DocManager* dm = (DocManager*)PyCObject_AsVoidPtr(py_c_doc_manager);
	if( dm==0 || filepath==0 )
		return 0;

	dm->open_file(filepath, line);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_save_current_file(PyObject* self, PyObject* args) {
	PyObject* py_c_doc_manager = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_save_current_file", &py_c_doc_manager))
		return 0;

	DocManager* dm = (DocManager*)PyCObject_AsVoidPtr(py_c_doc_manager);
	if( dm==0 )
		return 0;

	dm->save_current_file();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_close_current_file(PyObject* self, PyObject* args) {
	PyObject* py_c_doc_manager = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_close_current_file", &py_c_doc_manager))
		return 0;

	DocManager* dm = (DocManager*)PyCObject_AsVoidPtr(py_c_doc_manager);
	if( dm==0 )
		return 0;

	dm->close_current_file();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_save_all_files(PyObject* self, PyObject* args) {
	PyObject* py_c_doc_manager = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_save_all_files", &py_c_doc_manager))
		return 0;

	DocManager* dm = (DocManager*)PyCObject_AsVoidPtr(py_c_doc_manager);
	if( dm==0 )
		return 0;

	dm->save_all_files();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* ljedit_doc_manager_close_all_files(PyObject* self, PyObject* args) {
	PyObject* py_c_doc_manager = 0;
	if(!PyArg_ParseTuple(args, "O:ljedit_doc_manager_close_all_files", &py_c_doc_manager))
		return 0;

	DocManager* dm = (DocManager*)PyCObject_AsVoidPtr(py_c_doc_manager);
	if( dm==0 )
		return 0;

	dm->close_all_files();

	Py_INCREF(Py_None);
	return Py_None;
}

// wrapper gtkmm Notebook
// 
PyObject* gtkmm_notebook_append_page(PyObject* self, PyObject* args) {
	PyObject* py_c_notebook = 0;
	const char* name = 0;
	PyGObject* py_widget = 0;
	if(!PyArg_ParseTuple(args, "OsO:gtkmm_notebook_append_page", &py_c_notebook, &name, &py_widget))
		return 0;

	Gtk::Notebook* nb = (Gtk::Notebook*)PyCObject_AsVoidPtr(py_c_notebook);
	if( py_c_notebook==0 || name==0 || py_widget==0 || nb==0 || py_widget->obj==0 )
		return 0;

	GtkWidget* obj = (GtkWidget*)(py_widget->obj);
	Gtk::Widget* widget = Gtk::manage(Glib::wrap(obj));
	int page = nb->append_page(*widget, name);

	return PyInt_FromLong((long)page);
}

PyObject* gtkmm_notebook_remove_page(PyObject* self, PyObject* args) {
	PyObject* py_c_notebook = 0;
	int page = 0;
	if(!PyArg_ParseTuple(args, "Oi:gtkmm_notebook_remove_page", &py_c_notebook, &page))
		return 0;

	Gtk::Notebook* nb = (Gtk::Notebook*)PyCObject_AsVoidPtr(py_c_notebook);
	if( py_c_notebook==0 || page==0 || nb==0 )
		return 0;

	nb->remove_page(page);

	Py_INCREF(Py_None);
	return Py_None;
}

// ljedit methods
// 
PyMethodDef ljedit_methods[] = {
	{ "ljedit_doc_manager_create_new_file",    ljedit_doc_manager_create_new_file,    METH_VARARGS, "ljedit_doc_manager_create_new_file." },
	{ "ljedit_doc_manager_open_file",          ljedit_doc_manager_open_file,          METH_VARARGS, "ljedit_doc_manager_open_file." },
	{ "ljedit_doc_manager_save_current_file",  ljedit_doc_manager_save_current_file,  METH_VARARGS, "ljedit_doc_manager_save_current_file." },
	{ "ljedit_doc_manager_close_current_file", ljedit_doc_manager_close_current_file, METH_VARARGS, "ljedit_doc_manager_close_current_file." },
	{ "ljedit_doc_manager_save_all_files",     ljedit_doc_manager_save_all_files,     METH_VARARGS, "ljedit_doc_manager_save_all_files." },
	{ "ljedit_doc_manager_close_all_files",    ljedit_doc_manager_close_all_files,    METH_VARARGS, "ljedit_doc_manager_close_all_files." },
	{ "gtkmm_notebook_append_page",            gtkmm_notebook_append_page,            METH_VARARGS, "gtkmm_notebook_append_page." },
	{ "gtkmm_notebook_remove_page",            gtkmm_notebook_remove_page,            METH_VARARGS, "gtkmm_notebook_remove_page." },
	{NULL, NULL, 0, NULL}
};

//-------------------------------------------------------------------
// 
void __init_pygobject__() {
	init_pygobject();
}

bool init_pygobject_library() {
	__init_pygobject__();

	PyObject* err = PyErr_Occurred();
	if( err != 0 ) {
		PyErr_Print();
		PyErr_Clear();
		return false;
	}
	return true;
}

bool py_ljedit_add(PyObject* py_ljedit, const char* name, void* object) {
	assert( object != 0 );
	PyObject* py_c_object = PyCObject_FromVoidPtr(object, 0);
	return( py_c_object!=0 && PyModule_AddObject(py_ljedit, name, py_c_object)==0 );
}

class PythonPluginManager {
public:
	static PythonPluginManager& self() {
		static PythonPluginManager me_;
		return me_;
	}

	bool init() {
		if( !init_pygobject_library() )
			return false;

		PyObject* py_ljedit = Py_InitModule("ljedit", ljedit_methods);
		if( py_ljedit==0 )
			return false;

		LJEditorImpl& ljedit = LJEditorImpl::self();

		if( !( py_ljedit_add( py_ljedit, "__c_main_window", &ljedit.main_window())
			&& py_ljedit_add( py_ljedit, "__c_main_window_ui_manager",   &ljedit.main_window().ui_manager()   )
			&& py_ljedit_add( py_ljedit, "__c_main_window_action_group", &ljedit.main_window().action_group() )
			&& py_ljedit_add( py_ljedit, "__c_main_window_left_panel",   &ljedit.main_window().left_panel()   )
			&& py_ljedit_add( py_ljedit, "__c_main_window_doc_manager",  &ljedit.main_window().doc_manager()  )
			&& py_ljedit_add( py_ljedit, "__c_main_window_right_panel",  &ljedit.main_window().right_panel()  )
			&& py_ljedit_add( py_ljedit, "__c_main_window_bottom_panel", &ljedit.main_window().bottom_panel() )
			&& py_ljedit_add( py_ljedit, "__c_main_window_status_bar",   &ljedit.main_window().status_bar()   ) ) )
		{
			return false;
		}

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

		PyObject* load_plugins = PyDict_GetItemString(dict, "unload_plugins");
		if( load_plugins==0 )
			return;

		PyObject* result = PyObject_CallFunction(load_plugins, 0);
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

