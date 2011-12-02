// vconsole.c
// 
#define _CRT_SECURE_NO_WARNINGS

#include "vconsole.h"

#include "share.h"

#include <stdio.h>

#ifdef _DEBUG
	static void trace(const char* fmt, ...) {
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
		printf("TRACE : %s\n", str);
	}

#else
	void trace(const char* fmt, ...) {}
#endif

#define DEFAULT_SHELL	L"cmd.exe"

#define DEFAULT_SCREEN_UPDATE_TIME 20

extern	HMODULE	g_hModule;

void HookService();

typedef struct {
	VConsole	_parent;

	HANDLE	hCmdProcess;
	HANDLE	hMonitorThread;

	HANDLE	hShareMem;

	HANDLE	hEventStopMonitor;

	HANDLE	hToHook_Quit;
	HANDLE	hToHook_Alive;
	HANDLE	hToHook_Scroll;
	HANDLE	hToHook_Resize;
	HANDLE	hToHook_SendInput;

	HANDLE	hFromHook_Update;

	LONG			destroy_lock;
	ShareMemory*	shared;
} VCon;

static DWORD WINAPI MonitorService(VCon* con);
static VCon* vconsole_create();
static void vconsole_destroy(VCon* con);
static int vconsole_init(VCon* con);

#define ENV_STR_BLOCK_SZ	(32 * 1024)

static LPWCH create_env_strings() {
	LPWCH sEnv, sp, dEnv, dp;
	DWORD sSize, dSize, nSize;
	DWORD dwLimit = ENV_STR_BLOCK_SZ;

	sEnv = GetEnvironmentStrings();
	sp = sEnv;
	dEnv = malloc( dwLimit * sizeof(WCHAR) );
	dp = dEnv;

	dSize = 0;
	// copy current env
	while( *sp ) {
		sSize = wcslen(sp) + 1;		// str + '\0'
		nSize = dSize + sSize + 1;	// strs + '\0'
		if( nSize > dwLimit ) {
			while( nSize > dwLimit )
				dwLimit += ENV_STR_BLOCK_SZ;
			dEnv = realloc(dEnv, dwLimit * sizeof(WCHAR) );
			dp = dEnv + sSize;
		}
		memcpy(dp, sp, sSize);
		sp += sSize;
		dp += sSize;
		dSize += sSize;
	}

	// append new var
	sp = L"TEST_VAR=0";
	{
		sSize = wcslen(sp) + 1;		// str + '\0'
		nSize = dSize + sSize + 1;	// strs + '\0'
		if( nSize > dwLimit ) {
			while( nSize > dwLimit )
				dwLimit += ENV_STR_BLOCK_SZ;
			dEnv = realloc(dEnv, dwLimit * sizeof(WCHAR) );
			dp = dEnv + sSize;
		}
		memcpy(dp, sp, sSize);
		sp += sSize;
		dp += sSize;
		dSize += sSize;
	}

	*dp = 0;
	++dp;

	FreeEnvironmentStrings(sEnv);
	return dEnv;
}

static VCon* vconsole_create() {
	VCon* con = malloc(sizeof(VCon));
	if( con ) {
		memset(con, 0, sizeof(VCon));

		if( vconsole_init(con)==0 ) {
			con->hEventStopMonitor = CreateEvent(NULL, FALSE, FALSE, NULL);
			if( con->hEventStopMonitor ) {
				con->hMonitorThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MonitorService, con, 0, NULL);
				if( con->hMonitorThread )
					return con;
			}
		}

		vconsole_destroy(con);
	}

	return NULL;
}

static void vconsole_destroy(VCon* con) {
	LPTHREAD_START_ROUTINE pfnThreadRoutine;
	HANDLE hRemoteThread;

	if( !con )
		return;

	if( InterlockedCompareExchange(&(con->destroy_lock), 1, 0)!=0 )
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
		if( con->hToHook_Quit ) {
			SetEvent(con->hToHook_Quit );
			if( WaitForSingleObject(con->hCmdProcess, 5000)==WAIT_TIMEOUT) {
				pfnThreadRoutine = (LPTHREAD_START_ROUTINE)ExitProcess;

				// start the remote thread
				hRemoteThread = CreateRemoteThread(con->hCmdProcess, NULL, 0, pfnThreadRoutine, 0, 0, NULL);
				if( hRemoteThread ) {
					// wait for the thread to finish
					if( WaitForSingleObject(hRemoteThread, 5000)==WAIT_TIMEOUT)
						TerminateProcess(con->hCmdProcess, -9);

					CloseHandle(hRemoteThread);
				} else {
					TerminateProcess(con->hCmdProcess, -9);
				}
			}
		}

		CloseHandle(con->hCmdProcess);
		con->hCmdProcess = 0;
	}

	if( con->shared )
		UnmapViewOfFile(con->shared);

	if( con->hShareMem )
		CloseHandle(con->hShareMem);

	free(con);
}

static int vconsole_init(VCon* con) {
	DWORD dwRet;
	WCHAR szComspec[MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD dwStartupFlags = CREATE_NEW_CONSOLE|CREATE_SUSPENDED|CREATE_UNICODE_ENVIRONMENT;

	//LPWCH env;
	WCHAR szHookPath[4096];
	DWORD dwHookLen;

	LPVOID pRemoteAddr;
	LPTHREAD_START_ROUTINE pfnThreadRoutine;
	DWORD dwExitCode;
	HMODULE	hRemoteDLL;
	HANDLE hRemoteThread;
	HANDLE hRemoteSharedMem = 0;
	HANDLE hEvents[2];

	HANDLE hCurrentProcess = GetCurrentProcess();
	VConsole* vcon = (VConsole*)con;

	dwHookLen = GetModuleFileName(g_hModule, szHookPath, MAX_PATH);

	dwRet = GetEnvironmentVariable(L"COMSPEC", szComspec, MAX_PATH);
	if( dwRet > 0 )
		szComspec[dwRet] = 0;
	else
		wcscpy(szComspec, DEFAULT_SHELL);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb			= sizeof(STARTUPINFO);
	si.dwFlags		= STARTF_USESHOWWINDOW;
	si.wShowWindow	= SW_HIDE;

	// 1. create console process : (cmd.exe)
	//    and pause it's main thread
	// 
	if( !CreateProcess(NULL
			, szComspec
			, NULL
			, NULL
			, FALSE
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
	con->hToHook_Quit= CreateEvent(NULL, FALSE, FALSE, NULL);
	con->hToHook_Alive = CreateEvent(NULL, FALSE, FALSE, NULL);
	con->hToHook_Scroll = CreateEvent(NULL, FALSE, FALSE, NULL);
	con->hToHook_Resize = CreateEvent(NULL, FALSE, FALSE, NULL);
	con->hToHook_SendInput = CreateEvent(NULL, FALSE, FALSE, NULL);

	con->hFromHook_Update = CreateEvent(NULL, FALSE, FALSE, NULL);

	con->hShareMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShareMemory), NULL);
	if( !con->hShareMem )
		return 1001;

	con->shared = (ShareMemory*)MapViewOfFile(con->hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if( !con->shared )
		return 1002;

	memset(con->shared, 0, sizeof(ShareMemory));
	con->shared->owner_pid = GetCurrentProcessId();
	con->shared->screen_update_time = DEFAULT_SCREEN_UPDATE_TIME;

	// 3. create remote thread : insert con.dll into console process
	{
		pRemoteAddr = VirtualAllocEx(pi.hProcess, NULL, dwHookLen*sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE);
		if( !WriteProcessMemory(pi.hProcess, pRemoteAddr, szHookPath, wcslen(szHookPath)*sizeof(WCHAR), NULL) ) {
			return 3003;
		}

		// get address to LoadLibraryW function
		pfnThreadRoutine = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
		if( pfnThreadRoutine==NULL)
			return 3004;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, pRemoteAddr, 0, NULL);
		if( !hRemoteThread )
			return 3005;

		// wait for the thread to finish
		if( WaitForSingleObject(hRemoteThread, 10000)==WAIT_TIMEOUT)
			return 3006;

		if( !GetExitCodeThread(hRemoteThread, &dwExitCode) )
			return 3007;
		hRemoteDLL = (HMODULE)dwExitCode;

		CloseHandle(hRemoteThread);
		VirtualFreeEx(pi.hProcess, pRemoteAddr, dwHookLen*sizeof(WCHAR), MEM_RELEASE);
	}

	// 4. create remote thread : call HookService(witch in con.dll) in console process
	//    and pass share memory handle to it
	{
		if( !( DuplicateHandle(hCurrentProcess, con->hShareMem, pi.hProcess, &hRemoteSharedMem, 0, FALSE, DUPLICATE_SAME_ACCESS)
			&& DuplicateHandle(hCurrentProcess, con->hToHook_Quit, pi.hProcess, &(con->shared->tohook_event_quit), 0, FALSE, DUPLICATE_SAME_ACCESS)
			&& DuplicateHandle(hCurrentProcess, con->hToHook_Alive, pi.hProcess, &(con->shared->tohook_event_alive), 0, FALSE, DUPLICATE_SAME_ACCESS)
			&& DuplicateHandle(hCurrentProcess, con->hToHook_Scroll, pi.hProcess, &(con->shared->tohook_event_scroll), 0, FALSE, DUPLICATE_SAME_ACCESS)
			&& DuplicateHandle(hCurrentProcess, con->hToHook_Resize, pi.hProcess, &(con->shared->tohook_event_resize), 0, FALSE, DUPLICATE_SAME_ACCESS)
			&& DuplicateHandle(hCurrentProcess, con->hToHook_SendInput, pi.hProcess, &(con->shared->tohook_event_send_input), 0, FALSE, DUPLICATE_SAME_ACCESS)
			&& DuplicateHandle(hCurrentProcess, con->hFromHook_Update, pi.hProcess, &(con->shared->fromhook_event_update), 0, FALSE, DUPLICATE_SAME_ACCESS) ) )
		{
			return 4000;
		}

		pfnThreadRoutine = (LPTHREAD_START_ROUTINE)((char*)hRemoteDLL + ((char*)HookService - (char*)g_hModule));

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, hRemoteSharedMem, 0, NULL);
		if( !hRemoteThread )
			return 4001;

		hEvents[0] = hRemoteThread;
		hEvents[1] = con->hFromHook_Update;
		dwRet = WaitForMultipleObjects(2, hEvents, FALSE, 10000);
		if( dwRet==WAIT_FAILED || dwRet==WAIT_TIMEOUT || dwRet==(WAIT_OBJECT_0 + 0) )
			return 4002;

		CloseHandle(hRemoteThread);
	}

	// 5. resume the console process
	// 
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	vcon->screen_info = &(con->shared->screen_info);
	vcon->screen_buffer = con->shared->screen_buffer;
	vcon->hwnd = con->shared->hwnd;
	return 0;
}

static void vconsole_scroll(VCon* con, int left, int top) {
	if( !con )
		return;

	con->shared->tohook_scroll_pos.X = left;
	con->shared->tohook_scroll_pos.Y = top;
	SetEvent(con->hToHook_Scroll);
}

static void vconsole_resize(VCon* con, int width, int height) {
	if( !con )
		return;

	// trace("resize - width=%d, height=%d\n", width, height);
	if( con->shared->tohook_resize_val.X == width
		&& con->shared->tohook_resize_val.Y == height )
	{
		return;
	}

	con->shared->tohook_resize_val.X = width;
	con->shared->tohook_resize_val.Y = height;
	SetEvent(con->hToHook_Resize);
}

static void vconsole_send_input(VCon* con, WCHAR* text) {
	if( !con || !text )
		return;

	wcsncpy(con->shared->tohook_input_text, text, SEND_INPUT_CHARS_MAX);
	SetEvent(con->hToHook_SendInput);
}

static VConsoleAPI g_api =
	{ (void*)vconsole_create
	, (void*)vconsole_destroy
	, (void*)vconsole_scroll
	, (void*)vconsole_resize
	, (void*)vconsole_send_input
	};

__declspec(dllexport) VConsoleAPI* get_vconsole_api() { return &g_api; }

static DWORD WINAPI MonitorService(VCon* con) {
	DWORD dwRet;
	BOOL bRunSign = TRUE;
	HANDLE hEvents[] = {
		  con->hEventStopMonitor
		, con->hCmdProcess
		, con->hFromHook_Update
	};
	DWORD dwEventCount = sizeof(hEvents)/sizeof(HANDLE);
	VConsole* vcon = (VConsole*)con;

	//trace("monitor thread start!\n");

	while( bRunSign ) {
		dwRet = WaitForMultipleObjects(dwEventCount, hEvents, FALSE, 1000);
		if( dwRet==WAIT_TIMEOUT ) {
			//trace("monitor thread test!\n");

			SetEvent( con->hToHook_Alive );
			continue;

		} else if( dwRet==WAIT_FAILED ) {
			bRunSign = FALSE;
			continue;
		}

		dwRet -= WAIT_OBJECT_0;
        switch(dwRet) {
        case 0:
			bRunSign = FALSE;
            break;

        case 1:
			bRunSign = FALSE;
			if( con->hCmdProcess ) {
				CloseHandle(con->hCmdProcess);
				con->hCmdProcess = 0;
			}
            break;

		case 2:
			if( vcon->on_screen_changed )
				(vcon->on_screen_changed)(vcon);
			break;

		default:
			break;
        }
	}

	//trace("monitor thread stoped!\n");
	if( vcon->on_quit )
		(vcon->on_quit)(vcon);

	return 0;
}

