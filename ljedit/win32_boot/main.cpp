// main.cpp
// 

#include <Windows.h>
#include <Python.h>


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) {
	::Py_Initialize();
	::PyRun_SimpleString("from _ljedit_win32_boot import *\n" "run()");
	::Py_Finalize();
	return 0;
}

