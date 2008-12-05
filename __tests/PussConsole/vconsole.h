// vconsole.h
// 
#ifndef PUSS_VCONSOLE_IMPLEMENT_H
#define PUSS_VCONSOLE_IMPLEMENT_H

#include <Windows.h>

typedef struct _VConsole VConsole;

typedef struct {
	VConsole*	(*create)();
	void		(*destroy)(VConsole* console);

} VConsoleAPI;

typedef VConsoleAPI* (*TGetVConsoleAPIFn)();

#endif//PUSS_VCONSOLE_IMPLEMENT_H
