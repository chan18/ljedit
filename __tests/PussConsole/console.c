// console.c
// 

#define _WIN32_WINNT 0x500
#include "vconsole.h"

#include <stdio.h>
#include <conio.h>

VConsole* g_vcon = 0;

void on_quit() {
	printf("on_quit\n");
}

void on_screen_changed() {
	printf("on_screen_changed\n");
}

int main(int argc, char* argv[]) {
	HMODULE hDLL;
	TGetVConsoleAPIFn get_vconsole_api;
	VConsoleAPI* api;

	hDLL = LoadLibrary(L"vconsole.dll");
	if( !hDLL )
		return 1;

	get_vconsole_api = (TGetVConsoleAPIFn)GetProcAddress(hDLL, "get_vconsole_api");
	api = get_vconsole_api();
	g_vcon = api->create();
	g_vcon->on_screen_changed = on_screen_changed;
	g_vcon->on_quit = on_quit;

	SetForegroundWindow(GetConsoleWindow());

	for(;;) {
		getch();
		PostMessage(g_vcon->hwnd, WM_CHAR, 'a', 'b');
	}

	printf("wait exit!\n");
	api->destroy(g_vcon);

	printf("unload dll\n");
	FreeLibrary(hDLL);

	printf("exited!\n");
	return 0;
}
