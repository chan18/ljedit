// main.c
// 

#include <Windows.h>

#pragma warning(disable : 4996)

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) {
	char basePath[8192] = { '\0' };
	char buf[8192] = { '\0' };
	DWORD size = 0;

	size = GetModuleFileNameA(NULL, basePath, 4096);
	for( ; size > 0; --size ) {
		if( basePath[size - 1]=='\\' ) {
			basePath[size] = '\0';
			break;
		}
	}

	// set PATH = %PATH%; environ\gtk\bin
	// set GTK_BASEPATH = environ\gtk
	// set PYTHONPATH = environ\pylibs
	// 
	GetEnvironmentVariableA("PATH", buf, 8192);
	strcat(buf, ";");
	strcat(buf, basePath);
	strcat(buf, "environ");
	strcat(buf, ";");
	strcat(buf, basePath);
	strcat(buf, "environ\\gtk\\bin");
	SetEnvironmentVariableA("PATH", buf);

	strcpy(buf, basePath);
	strcat(buf, "environ\\gtk");
	SetEnvironmentVariableA("GTK_BASEPATH", buf);

	strcpy(buf, basePath);
	strcat(buf, "environ\\pylibs");
	SetEnvironmentVariableA("PYTHONPATH", buf);

	// run ./puss.exe
	// 
	strcpy(buf, basePath);
	strcat(buf, "_puss.exe");

	ShellExecuteA(HWND_DESKTOP, "open", buf, lpCmdLine, NULL, nShowCmd);

	return 0;
}

