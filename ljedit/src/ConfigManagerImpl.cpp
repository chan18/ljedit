// PluginManager.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "ConfigManagerImpl.h"
#include "RuntimeException.h"

#include <Python.h>

struct PythonImpl {
	PyObject* module;
	PyObject* regist_option_method;
	PyObject* get_option_value_method;

	PyObject* option_changed_callback;
	
	OptionValueChangeSignal	option_change_signal;
};

PyObject* ljedit_config_manager_option_changed_callback(PyObject* self, PyObject* args) {
	const char* id = 0;	 int szid = 0;
	const char* val = 0; int szval = 0;
	const char* old = 0; int szold = 0;
	if( !PyArg_ParseTuple(args, "s#s#s#:ljedit_config_manager_option_changed_callback", &id, &szid, &val, &szval, &old, &szold) )
		return 0;

	std::string sid(id, szid), sval(val, szval), sold(old, szold);
	ConfigManagerImpl::self().signal_option_changed().emit(sid, sval, sold);

	Py_INCREF(Py_None);
	return Py_None;
}

ConfigManagerImpl& ConfigManagerImpl::self() {
	static PythonImpl impl_struct_ = { 0 };
	static ConfigManagerImpl me_(&impl_struct_);
	return me_;
}

ConfigManagerImpl::ConfigManagerImpl(void* impl) : impl_(impl) {
	::Py_Initialize();
}

ConfigManagerImpl::~ConfigManagerImpl() {
	::Py_Finalize();
}

void ConfigManagerImpl::create() {
	PythonImpl* impl = (PythonImpl*)impl_;
	impl->module = PyImport_ImportModule("_ljedit_config_manager");
	if( impl->module==0 )
		throw RuntimeException("ConfigManager create failed, can not load python implement module!");

	static PyMethodDef pm = { "__option_changed_callback"
		, &ljedit_config_manager_option_changed_callback
		, METH_VARARGS
		, "ljedit config manager python implement, option changed callback" };

    impl->option_changed_callback = PyCFunction_New( &pm, NULL );
	if( impl->option_changed_callback==0 )
		throw RuntimeException("ConfigManager option changed callback!");
	PyModule_AddObject(impl->module, "__option_changed_callback", impl->option_changed_callback);

	PyObject* py_dict = PyModule_GetDict(impl->module);
	if( py_dict==0 )
		throw RuntimeException("ConfigManager create failed, PyModule_GetDict failed!");

	PyObject* py_create           = PyDict_GetItemString(py_dict, "create");
	impl->regist_option_method    = PyDict_GetItemString(py_dict, "regist_option");
	impl->get_option_value_method = PyDict_GetItemString(py_dict, "get_option_value");
	if( py_create==0 || impl->regist_option_method==0 || impl->regist_option_method==0 )
		throw RuntimeException("ConfigManager create failed, get functions failed!");


	PyObject* py_result = PyObject_CallFunction(py_create, 0);
	if( py_result==0 ) {
		PyErr_Print();
		PyErr_Clear();
	}
	Py_XDECREF(py_result);
}

void ConfigManagerImpl::destroy() {
}

void ConfigManagerImpl::regist_option( const std::string& id
	, const std::string& type
	, const std::string& default_value
	, const std::string& tip )
{
	PythonImpl* impl = (PythonImpl*)impl_;
	assert( impl != 0 );

	PyObject* py_result = PyObject_CallFunction(impl->regist_option_method
		, "s#s#s#s#"
		, id.c_str(),				id.size()
		, type.c_str(),				type.size()
		, default_value.c_str(),	default_value.size()
		, tip.c_str(),				tip.size() );

	if( py_result==0 ) {
		PyErr_Print();
		PyErr_Clear();
	}
	Py_XDECREF(py_result);
}

bool ConfigManagerImpl::get_option_value(const std::string& id, std::string& value) {
	PythonImpl* impl = (PythonImpl*)impl_;
	assert( impl != 0 );

	PyObject* py_result = PyObject_CallFunction(impl->get_option_value_method
		, "s#"
		, id.c_str(), id.size() );

	if( py_result!=0 ) {
		if( PyString_Check(py_result) ) {
			char* pv = 0;
			Py_ssize_t sz = 0;
			PyString_AsStringAndSize(py_result, &pv, &sz);
			value.assign(pv, sz);
			Py_DECREF(py_result);
			return true;
			
		} else {
			Py_DECREF(py_result);
		}
		
	} else {
		PyErr_Print();
		PyErr_Clear();
	}
	
	return false;
}

OptionValueChangeSignal& ConfigManagerImpl::signal_option_changed() {
	PythonImpl* impl = (PythonImpl*)impl_;
	assert( impl != 0 );

	return impl->option_change_signal;
}

