// console.c
// 

#include "vconsole.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
	HMODULE hDLL;
	TGetVConsoleAPIFn get_vconsole_api;
	VConsoleAPI* api;
	VConsole* vconsole;

	hDLL = LoadLibrary(L"vconsole.dll");
	if( !hDLL )
		return 1;

	get_vconsole_api = (TGetVConsoleAPIFn)GetProcAddress(hDLL, "get_vconsole_api");
	api = get_vconsole_api();
	vconsole = api->create();

	printf("press any key to exit!\n");
	system("pause");

	printf("wait exit!\n");
	api->destroy(vconsole);

	printf("unload dll\n");
	FreeLibrary(hDLL);

	printf("exited!\n");
	return 0;
}
