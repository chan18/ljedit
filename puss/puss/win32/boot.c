// main.c
// 

#ifdef WIN32

#include <Windows.h>

#pragma warning(disable : 4996)

typedef int (*PussMain)(int argc, char* argv[]);

//int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) {

int main(int argc, char* argv[]) {
	// STARTUPINFOA si;
	// PROCESS_INFORMATION pi;
	char basePath[8192] = { '\0' };
	char buf[8192] = { '\0' };
	DWORD size = 0;
	HMODULE hDLL;
	int retval;
	PussMain puss_main;

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
	if( !GetEnvironmentVariableA("GTK_BASEPATH", buf, 8192) ) {
		strcpy(buf, basePath);
		strcat(buf, "environ\\gtk");
		SetEnvironmentVariableA("GTK_BASEPATH", buf);

		GetEnvironmentVariableA("PATH", buf, 8192);
		strcat(buf, ";");
		strcat(buf, basePath);
		strcat(buf, "environ");
		strcat(buf, ";");
		strcat(buf, basePath);
		strcat(buf, "environ\\gtk\\bin");
		SetEnvironmentVariableA("PATH", buf);

	}

	if( !GetEnvironmentVariableA("PYTHONPATH", buf, 8192) ) {
		strcpy(buf, basePath);
		strcat(buf, "environ\\pylibs");
		SetEnvironmentVariableA("PYTHONPATH", buf);
	}

	// load ./_puss.dll
	// 
	strcpy(buf, basePath);
#ifdef _DEBUG
	strcat(buf, "_puss_d.dll");
#else
	strcat(buf, "_puss.dll");
#endif

	// ShellExecuteA(HWND_DESKTOP, "open", buf, lpCmdLine, NULL, nShowCmd);

	hDLL = LoadLibraryA(buf);
	if( !hDLL ) {
		MessageBoxA(NULL, "Not find _puss.dll(or _puss_d.dll in debug version)", "Puss Error!", MB_OK);
		return 1;
	}

	puss_main = (PussMain)GetProcAddress(hDLL, "main");
	retval = (*puss_main)(argc, argv);
	FreeLibrary(hDLL);

	return retval;
}

#endif

