// LJEditorPythonPluginEngine.cpp
// 

#include "StdAfx.h"	// for vc precompile header

#include "LJEditorPythonPluginEngine.h"

#ifndef ENABLE_PYTHON_PLUGIN_ENGINE
	void ljed_start_python_plugin_engine() {}
	void ljed_stop_python_plugin_engine() {}

#else
	#include "LJEditorPythonPluginEngineImpl.cpp"
#endif

