// main.c

#include <Windows.h>

int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {
	char szApp[4096];
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	DWORD len = GetModuleFileNameA(NULL, szApp, sizeof(szApp));
	for( ; len > 0; --len ) {
		if( szApp[len-1]=='\\' || szApp[len-1]=='/' )
			break;
	}

#ifdef _DEBUG
	strcpy(szApp+len, "bin\\puss_d.exe");
#else
	strcpy(szApp+len, "bin\\puss.exe");
#endif

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb			= sizeof(STARTUPINFO);
	si.dwFlags		= STARTF_USESHOWWINDOW;
	si.wShowWindow	= SW_HIDE;
	if( !CreateProcessA(szApp, lpCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) ) {
		char errmsg[8192];
		sprintf(errmsg, "not find puss application :\n   %s\n", szApp);
		MessageBoxA(NULL, errmsg, "Puss Error", MB_ICONERROR | MB_OK);
	}

	return 0;
}

