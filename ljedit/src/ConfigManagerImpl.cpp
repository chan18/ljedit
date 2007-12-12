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
};

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

void ConfigManagerImpl::regist_option(const std::string& id, const std::string& type, const std::string& default_value) {
	PythonImpl* impl = (PythonImpl*)impl_;
	assert( impl != 0 );

	PyObject* py_result = PyObject_CallFunction(impl->regist_option_method
		, "s#s#s#"
		, id.c_str(), id.size()
		, type.c_str(), type.size()
		, default_value.c_str(), default_value.size() );

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
		, "s#s#s#"
		, id.c_str(), id.size() );

	if( py_result==0 ) {
		PyErr_Print();
		PyErr_Clear();
	}
	Py_XDECREF(py_result);
	return py_result!=0;
}

