// win32_main.c

#ifdef _USE_THIS_MODULE

#ifndef _DEBUG
	#define USE_WINMAIN
#endif

#ifndef USE_WINMAIN
	#define proxy_main	main
#endif

#define _WIN32_WINNT 0x0502
#include <Windows.h>
#include <tchar.h>

static void set_3rd_dll_path(void) {
	wchar_t path[8192];
	DWORD i = GetModuleFileNameW(0, path, 8192);
	for( --i; i>0; --i ) {
		if( path[i]==L'\\' || path[i]==L'/' )
			break;

		wcscpy(path+i, L"\\3rd\\bin");
		SetDllDirectoryW(path);
	}
}

typedef int (*PussMain)(int argc, char* argv[]);

int proxy_main(int argc, char* argv[]) {
	HINSTANCE hInstance;
	PussMain puss_main;

	set_3rd_dll_path();

#ifdef _DEBUG
	hInstance = LoadLibraryW(L"extends\\puss_d.dll");
#else
	hInstance = LoadLibraryW(L"extends\\puss.dll");
#endif
	if( !hInstance ) {
		MessageBoxW(NULL, L"not find puss dll", L"Puss Error", MB_ICONERROR);
		return 1;
	}


	puss_main = (PussMain)GetProcAddress(hInstance, "main");
	if( !puss_main ) {
		MessageBoxW(NULL, L"not find main() in puss dll", L"Puss Error", MB_ICONERROR);
		return 1;
	}

	return (*puss_main)(argc, argv);
}

#ifdef USE_WINMAIN
	int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {
		int res = 0;
		if( nCmdShow != SW_HIDE ) {
			char szApp[4096];
			STARTUPINFOA si;
			PROCESS_INFORMATION pi;
			GetModuleFileNameA(NULL, szApp, sizeof(szApp));
			ZeroMemory(&si, sizeof(STARTUPINFO));
			si.cb			= sizeof(STARTUPINFO);
			si.dwFlags		= STARTF_USESHOWWINDOW;
			si.wShowWindow	= SW_HIDE;
			CreateProcessA(szApp, lpCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
			return res;
		}

		AllocConsole();
		res = proxy_main(__argc, __argv);
		FreeConsole();

		return res;
	}
#endif//USE_WINMAIN

#endif//_USE_THIS_MODULE

