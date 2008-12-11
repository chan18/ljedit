// hook.c
// 

#define _WIN32_WINNT 0x500
#include <Windows.h>
#include <stdio.h>

#include "share.h"

#define ALIVE_TIMEOUT	30000

// global vars only for this file
// 

static void read_console_buffer(ShareMemory* shared) {
	HANDLE hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbiConsole;
	CHAR_INFO sbuf[MAX_SCR_LEN];
	COORD coordConsoleSize;

	DWORD dwScreenBufferSize;
	DWORD dwScreenBufferOffset;

	COORD coordBufferSize;
	// start coordinates for the buffer are always (0, 0) - we use offset
	COORD coordStart = {0, 0};
	SMALL_RECT srBuffer;
	SHORT i = 0;

	// we take a fresh STDOUT handle - seems to work better (in case a program
	// has opened a new screen output buffer)
	hStdOut = CreateFile( L"CONOUT$"
		, GENERIC_WRITE | GENERIC_READ
		, FILE_SHARE_READ | FILE_SHARE_WRITE
		, NULL
		, OPEN_EXISTING
		, 0
		, 0 );

	// get total console size

	GetConsoleScreenBufferInfo(hStdOut, &csbiConsole);

	coordConsoleSize.X	= csbiConsole.srWindow.Right - csbiConsole.srWindow.Left + 1;
	coordConsoleSize.Y	= csbiConsole.srWindow.Bottom - csbiConsole.srWindow.Top + 1;

	//TRACE(L"ReadConsoleBuffer console buffer size: %ix%i\n", csbiConsole.dwSize.X, csbiConsole.dwSize.Y);
	//TRACE(L"ReadConsoleBuffer console rect: %ix%i - %ix%i\n", csbiConsole.srWindow.Left, csbiConsole.srWindow.Top, csbiConsole.srWindow.Right, csbiConsole.srWindow.Bottom);
	//TRACE(L"console window rect: (%i, %i) - (%i, %i)\n", csbiConsole.srWindow.Top, csbiConsole.srWindow.Left, csbiConsole.srWindow.Bottom, csbiConsole.srWindow.Right);

	// do console output buffer reading
	dwScreenBufferSize = coordConsoleSize.X * coordConsoleSize.Y;
	dwScreenBufferOffset = 0;

//	TRACE(L"===================================================================\n");

	// ReadConsoleOutput seems to fail for large (around 6k CHAR_INFO's) buffers
	// here we calculate max buffer size (row count) for safe reading
	coordBufferSize.X	= csbiConsole.srWindow.Right - csbiConsole.srWindow.Left + 1;
	coordBufferSize.Y	= 6144 / coordBufferSize.X;

	// initialize reading rectangle
	srBuffer.Top		= csbiConsole.srWindow.Top;
	srBuffer.Bottom		= csbiConsole.srWindow.Top + coordBufferSize.Y - 1;
	srBuffer.Left		= csbiConsole.srWindow.Left;
	srBuffer.Right		= csbiConsole.srWindow.Left + csbiConsole.srWindow.Right - csbiConsole.srWindow.Left;

	//TRACE(L"Buffer size for loop reads: %ix%i\n", coordBufferSize.X, coordBufferSize.Y);
	//TRACE(L"-------------------------------------------------------------------\n");

	// read rows 'chunks'
	for (i = 0; i < coordConsoleSize.Y / coordBufferSize.Y; ++i)
	{
//		TRACE(L"Reading region: (%i, %i) - (%i, %i)\n", srBuffer.Left, srBuffer.Top, srBuffer.Right, srBuffer.Bottom);

		ReadConsoleOutput(hStdOut, sbuf + dwScreenBufferOffset, coordBufferSize, coordStart, &srBuffer);

		srBuffer.Top = srBuffer.Top + coordBufferSize.Y;
		srBuffer.Bottom = srBuffer.Bottom + coordBufferSize.Y;

		dwScreenBufferOffset += coordBufferSize.X * coordBufferSize.Y;
	}

	// read the last 'chunk', we need to calculate the number of rows in the
	// last chunk and update bottom coordinate for the region
	coordBufferSize.Y	= coordConsoleSize.Y - i * coordBufferSize.Y;
	srBuffer.Bottom		= csbiConsole.srWindow.Bottom;

	//TRACE(L"Buffer size for last read: %ix%i\n", coordBufferSize.X, coordBufferSize.Y);
	//TRACE(L"-------------------------------------------------------------------\n");
	//TRACE(L"Reading region: (%i, %i) - (%i, %i)\n", srBuffer.Left, srBuffer.Top, srBuffer.Right, srBuffer.Bottom);

	ReadConsoleOutput(hStdOut, sbuf + dwScreenBufferOffset, coordBufferSize, coordStart, &srBuffer);

//	TRACE(L"===================================================================\n");

	// compare previous buffer, and if different notify Console
	if( (shared->screen_buffer_len != dwScreenBufferSize) ||
		(memcmp(&(shared->screen_info), &csbiConsole, sizeof(CONSOLE_SCREEN_BUFFER_INFO)) != 0) ||
		(memcmp(shared->screen_buffer, sbuf, dwScreenBufferSize*sizeof(CHAR_INFO)) != 0) )
	{
		shared->screen_buffer_len = dwScreenBufferSize;
		CopyMemory(&(shared->screen_info), &csbiConsole, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
		CopyMemory(shared->screen_buffer, sbuf, dwScreenBufferSize*sizeof(CHAR_INFO));

		GetConsoleCursorInfo(hStdOut, &(shared->cursor_info));

		SetEvent(shared->fromhook_event_update);
	}

	CloseHandle(hStdOut);
}

static void hook_service_run(ShareMemory* shared, HANDLE hParent) {
	DWORD dwRet;
	BOOL bRunSign;
	DWORD dwLastAlive;
	HANDLE hEvents[] = {
		  hParent
		, shared->tohook_event_quit
		, shared->tohook_event_alive
	};
	DWORD dwEventCount = sizeof(hEvents)/sizeof(HANDLE);

	//printf("thread start!\n");
	read_console_buffer(shared);

	bRunSign = TRUE;
	dwLastAlive = 0;

	while( bRunSign ) {
		dwRet = WaitForMultipleObjects(dwEventCount, hEvents, FALSE, shared->screen_update_time);
		if( dwRet==WAIT_TIMEOUT ) {
			++dwLastAlive;
			if( (shared->screen_update_time * dwLastAlive) > ALIVE_TIMEOUT )
				bRunSign = FALSE;
			else
				read_console_buffer(shared);
			continue;

		} else if( dwRet==WAIT_FAILED ) {
			bRunSign = FALSE;
			continue;

		} else {
			dwLastAlive = 0;
			dwRet -= WAIT_OBJECT_0;
		}

        switch(dwRet) {
		case 0:
			bRunSign = FALSE;
			break;
		case 1:
			bRunSign = FALSE;
			SetEvent(shared->tohook_event_quit);
			break;
		case 2:
			//printf("alive event\n");
			break;
		}
	}
}

DWORD HookService(HANDLE hShareMem) {
	ShareMemory* shared;
	HANDLE hParent;
	HWND hWnd = GetConsoleWindow();
	//printf("service start : %p  - %p\n", hShareMem, &hShareMem);

	//ShowWindow(hWnd, SW_HIDE);

	{
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD dwSize = { 100, 300 };
		SMALL_RECT srWindow = { 0, 0, dwSize.X-1, 13 };
		SetConsoleScreenBufferSize(hStdOut, dwSize);
		SetConsoleWindowInfo(hStdOut, TRUE, &srWindow);
	}

	//printf("hook monitor thread!\n");
	shared = MapViewOfFile(hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	//printf("hH:%p  gg:%p\n",  hShareMem, shared);

	if( !shared )
		return 1;
	shared->hwnd = hWnd;
	hParent = OpenProcess(PROCESS_ALL_ACCESS, FALSE, shared->owner_pid);

	hook_service_run(shared, hParent);

	CloseHandle(hParent);

	UnmapViewOfFile(shared);
	CloseHandle(hShareMem);

	//printf("thread stoped!\n");

	ShowWindow(hWnd, SW_SHOW);
	SendMessage(hWnd, WM_CLOSE, 0, 0);

	ExitProcess(0);
	return 0;
}

