// vconsole.h
// 
#ifndef PUSS_VCONSOLE_IMPLEMENT_H
#define PUSS_VCONSOLE_IMPLEMENT_H

#include <Windows.h>

typedef struct _VConsole VConsole;

struct _VConsole {
	HWND						hwnd;
	CONSOLE_SCREEN_BUFFER_INFO*	screen_info;
	CHAR_INFO*					screen_buffer;
	CONSOLE_CURSOR_INFO*		cursor_info;

	void*		tag;

	void		(*on_quit)(VConsole* vcon);
	void		(*on_screen_changed)(VConsole* vcon);
};

typedef struct {
	VConsole*	(*create)();
	void		(*destroy)(VConsole* console);
} VConsoleAPI;

typedef VConsoleAPI* (*TGetVConsoleAPIFn)();

#endif//PUSS_VCONSOLE_IMPLEMENT_H