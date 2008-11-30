// win32_console_hook.c
// 

#include <Windows.h>
#include <stdio.h>

#include "share.h"

#define DEFAULT_SHELL	L"cmd.exe"

extern	HMODULE	g_hModule;

void HookService();

static int vconsole_create() {
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

	HANDLE			hShareMem;
	ShareMemory*	shared;

	HANDLE	hEvents[1];
	hEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);

	// test share memory
	hShareMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(ShareMemory), NULL);
	if( !hShareMem )
		return 1;

	shared = (ShareMemory*)MapViewOfFile(hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if( !shared )
		return 2;

	memset(shared, 0, sizeof(ShareMemory));
	shared->owner_pid = GetCurrentProcessId();

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
		return 1;
	}

	{
		pRemoteAddr = VirtualAllocEx(pi.hProcess, NULL, dwHookLen*sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE);
		if( !WriteProcessMemory(pi.hProcess, pRemoteAddr, szHookPath, wcslen(szHookPath)*sizeof(WCHAR), NULL) )
			return 3;

		// get address to LoadLibraryW function
		pfnThreadRoutine = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
		if( pfnThreadRoutine==NULL)
			return 4;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, pRemoteAddr, 0, NULL);
		if( !hRemoteThread )
			return 5;

		// wait for the thread to finish
		if( WaitForSingleObject(hRemoteThread, 10000)==WAIT_TIMEOUT)
			return 6;

		CloseHandle(hRemoteThread);
		VirtualFreeEx(pi.hProcess, pRemoteAddr, dwHookLen*sizeof(WCHAR), MEM_RELEASE);
	}

	{
		if( !DuplicateHandle(GetCurrentProcess(), hEvents[0], pi.hProcess, &(shared->event_alive), 0, FALSE, DUPLICATE_SAME_ACCESS) )
			return 3;

		if( !DuplicateHandle(GetCurrentProcess(), hShareMem, pi.hProcess, &hRemoteSharedMem, 0, FALSE, DUPLICATE_SAME_ACCESS) )
			return 3;

		pfnThreadRoutine = (PTHREAD_START_ROUTINE)HookService;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, hRemoteSharedMem, 0, NULL);
		if( !hRemoteThread )
			return 5;

		CloseHandle(hRemoteThread);
	}

	// resume the console process
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	printf("puss console thread!\n");
	for(dwRet=0; dwRet<10; ++dwRet) {
		Sleep(1000);
		SetEvent(hEvents[0]);
		++shared->testa;
		printf("testa: %d   testb:%d\n", shared->testa, shared->testb);
	}

	{
		pfnThreadRoutine = (PTHREAD_START_ROUTINE)ExitProcess;

		// start the remote thread
		hRemoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, pfnThreadRoutine, 0, 0, NULL);
		if( !hRemoteThread )
			return 5;

		// wait for the thread to finish
		if( WaitForSingleObject(hRemoteThread, 10000)==WAIT_TIMEOUT) {
			TerminateProcess(pi.hProcess, 0);
		}

		CloseHandle(hRemoteThread);
	}

	UnmapViewOfFile(shared);
	CloseHandle(hShareMem);

	return 0;
}

__declspec(dllexport)
int __test() {
	vconsole_create();
	return 0;
}
