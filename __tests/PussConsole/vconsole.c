// vconsole.c
// 

#include "vconsole.h"

#include "share.h"

#include <stdio.h>

#ifdef _DEBUG
	void trace(const char* fmt, ...) {
		#define DEBUG_INFO_SIZE 8192
		int len = DEBUG_INFO_SIZE;
		char str[DEBUG_INFO_SIZE];
		#undef DEBUG_INFO_SIZE

		va_list args;
		va_start( args,fmt );
		len = vsnprintf(str, len, fmt, args);
		va_end(args);
		str[len] = '\0';

		OutputDebugStringA(str);
	}

#else
	void trace(const char* fmt, ...) {}
#endif

#define DEFAULT_SHELL	L"cmd.exe"

extern	HMODULE	g_hModule;

void HookService();

struct _VConsole {
	HANDLE	hCmdProcess;
	HANDLE	hMonitorThread;

	HANDLE	hShareMem;

	HANDLE	hEventAlive;
	HANDLE	hEventStopMonitor;

	ShareMemory*	shared;
};

static DWORD WINAPI MonitorService(VConsole* con);
static VConsole* vconsole_create();
static void vconsole_destroy(VConsole* con);
static int vconsole_init(VConsole* con);

static VConsole* vconsole_create() {
	VConsole* con = malloc(sizeof(VConsole));

	if( con ) {
		memset(con, 0, sizeof(VConsole));

		if( vconsole_init(con)==0 ) {
			con->hEventStopMonitor = CreateEvent(NULL, FALSE, FALSE, NULL);
			if( con->hEventStopMonitor ) {
				con->hMonitorThread = CreateThread(NULL, 0, &MonitorService, con, 0, NULL);
				if( con->hMonitorThread )
					return con;
			}
		}
	}

	vconsole_destroy(con);
	return NULL;
}

static void vconsole_destroy(VConsole* con) {
	PTHREAD_START_ROUTINE pfnThreadRoutine;
	HANDLE hRemoteThread;

	if( !con )
		return;

	if( con->hMonitorThread ) {
		SetEvent( con->hEventStopMonitor );
		if( WaitForSingleObject(con->hMonitorThread, 5000)==WAIT_TIMEOUT)
			TerminateThread(con->hMonitorThread, 0);
		CloseHandle(con->hMonitorThread);
	}

	if( con->hEventStopMonitor )
		CloseHandle(con->hEventStopMonitor);

	if( con->hCmdProcess ) {
		pfnThreadRoutine = (PTHREAD_START_ROUTINE)ExitProcess;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(con->hCmdProcess, NULL, 0, pfnThreadRoutine, 0, 0, NULL);
		if( hRemoteThread ) {
			// wait for the thread to finish
			if( WaitForSingleObject(hRemoteThread, 5000)==WAIT_TIMEOUT)
				TerminateProcess(con->hCmdProcess, 0);

			CloseHandle(hRemoteThread);
		} else {
			TerminateProcess(con->hCmdProcess, 0);
		}

		CloseHandle(con->hCmdProcess);
	}

	if( con->shared )
		UnmapViewOfFile(con->shared);

	if( con->hShareMem )
		CloseHandle(con->hShareMem);

	free(con);
}

static int vconsole_init(VConsole* con) {
	DWORD dwRet;
	WCHAR szComspec[MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD dwStartupFlags = CREATE_NEW_CONSOLE|CREATE_SUSPENDED|CREATE_UNICODE_ENVIRONMENT;

	WCHAR szHookPath[MAX_PATH];
	DWORD dwHookLen;

	LPVOID pRemoteAddr;
	PTHREAD_START_ROUTINE pfnThreadRoutine;
	HANDLE hRemoteThread;
	HANDLE hRemoteSharedMem = 0;

	dwHookLen = GetModuleFileName(g_hModule, szHookPath, MAX_PATH);

	dwRet = GetEnvironmentVariable(L"COMSPEC", szComspec, MAX_PATH);
	if( dwRet > 0 )
		szComspec[dwRet] = 0;
	else
		wcscpy(szComspec, DEFAULT_SHELL);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.dwFlags		= STARTF_USEPOSITION;
	si.cb			= sizeof(STARTUPINFO);
	si.wShowWindow	= SW_HIDE;
	si.lpTitle		= NULL;
	si.dwX			= 0x7FFF;
	si.dwY			= 0x7FFF;

	// 1. create console process : (cmd.exe)
	//    and pause it's main thread
	// 
	if( !CreateProcess(NULL
			, szComspec
			, NULL
			, NULL
			, TRUE
			, dwStartupFlags
			, NULL
			, NULL
			, &si
			, &pi) )
	{
		return 1000;
	}

	// 2. create events and shared memory
	// 
	con->hCmdProcess = pi.hProcess;

	// 2.1 remote comm events
	con->hEventAlive = CreateEvent(NULL, FALSE, FALSE, NULL);

	con->hShareMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShareMemory), NULL);
	if( !con->hShareMem )
		return 2001;

	con->shared = (ShareMemory*)MapViewOfFile(con->hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if( !con->shared )
		return 2002;

	memset(con->shared, 0, sizeof(ShareMemory));
	con->shared->owner_pid = GetCurrentProcessId();

	// 3. create remote thread : insert con.dll into console process
	{
		pRemoteAddr = VirtualAllocEx(pi.hProcess, NULL, dwHookLen*sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE);
		if( !WriteProcessMemory(pi.hProcess, pRemoteAddr, szHookPath, wcslen(szHookPath)*sizeof(WCHAR), NULL) ) {
			return 3003;
		}

		// get address to LoadLibraryW function
		pfnThreadRoutine = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
		if( pfnThreadRoutine==NULL)
			return 3004;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, pRemoteAddr, 0, NULL);
		if( !hRemoteThread )
			return 3005;

		// wait for the thread to finish
		if( WaitForSingleObject(hRemoteThread, 10000)==WAIT_TIMEOUT)
			return 3006;

		CloseHandle(hRemoteThread);
		VirtualFreeEx(pi.hProcess, pRemoteAddr, dwHookLen*sizeof(WCHAR), MEM_RELEASE);
	}

	// 4. create remote thread : call HookService(witch in con.dll) in console process
	//    and pass share memory handle to it
	{
		if( !DuplicateHandle(GetCurrentProcess(), con->hEventAlive, pi.hProcess, &(con->shared->event_alive), 0, FALSE, DUPLICATE_SAME_ACCESS) )
			return 4001;

		if( !DuplicateHandle(GetCurrentProcess(), con->hShareMem, pi.hProcess, &hRemoteSharedMem, 0, FALSE, DUPLICATE_SAME_ACCESS) )
			return 4002;

		pfnThreadRoutine = (PTHREAD_START_ROUTINE)HookService;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, hRemoteSharedMem, 0, NULL);
		if( !hRemoteThread )
			return 4003;

		CloseHandle(hRemoteThread);
	}

	// 5. resume the console process
	// 
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	return 0;
}

static DWORD WINAPI MonitorService(VConsole* con) {
	DWORD dwRet;
	HANDLE hEvents[10];
	BOOL bRunSign = TRUE;

	hEvents[0] = con->hEventStopMonitor;
	//hEvents[1] = con->xxxxx;

	trace("monitor thread start!\n");

	while( bRunSign ) {
		dwRet = WaitForMultipleObjects(1, hEvents, FALSE, 1000);

        switch(dwRet) {
        case WAIT_TIMEOUT:
			trace("monitor thread test!\n");

			++(con->shared->testa);
			SetEvent( con->hEventAlive );
            break;

        case WAIT_FAILED:
			bRunSign = FALSE;
            break;

		default:
			dwRet -= WAIT_OBJECT_0;
			switch( dwRet ) {
			case 0:
				bRunSign = FALSE;
				break;
			}
        }
	}

	trace("monitor thread stoped!\n");
	return 0;
}

VConsoleAPI g_api =	{
	  vconsole_create
	, vconsole_destroy
	//, vconsole_xxx
};

__declspec(dllexport) VConsoleAPI* get_vconsole_api() { return &g_api; }

