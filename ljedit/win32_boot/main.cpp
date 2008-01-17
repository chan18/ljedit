// main.cpp
// 

#include <Windows.h>

#ifdef _DEBUG
	#undef _DEBUG
	#include <Python.h>
	#define _DEBUG
#else
	#include <Python.h>
#endif

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) {
	::Py_Initialize();
	::PyRun_SimpleString("from _ljedit_win32_boot import *\n" "run()");
	::Py_Finalize();
	return 0;
}

