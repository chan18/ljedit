// hook.c
// 

#define _WIN32_WINNT 0x500
#include <Windows.h>
#include <stdio.h>

#include "share.h"

// global vars only for this file
// 

typedef BOOL (*EvHanle)(ShareMemory* shared);

static BOOL service_quit(ShareMemory* shared) {
	return FALSE;
}

static BOOL service_continue(ShareMemory* shared) {
	return TRUE;
}

static BOOL read_console_buffer(ShareMemory* shared) {
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

		SetEvent(shared->fromhook_event_update);
	}

	CloseHandle(hStdOut);
	return TRUE;
}

static BOOL console_on_scroll(ShareMemory* shared) {
	COORD newPos = shared->tohook_scroll_pos;
	SMALL_RECT srNew = shared->screen_info.srWindow;
	SHORT h = srNew.Bottom - srNew.Top;
	SHORT w = srNew.Right - srNew.Left;
	HANDLE hStdOut;

	if( h < 1 ) h = 1;
	if( w < 1 ) w = 1;

	srNew.Top = newPos.Y;
	srNew.Bottom = srNew.Top + h;
	srNew.Left = newPos.X;
	srNew.Right = srNew.Left + w;

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdOut, &(shared->screen_info));

	if( srNew.Bottom >= shared->screen_info.dwSize.Y ) {
		srNew.Bottom = shared->screen_info.dwSize.Y - 1;
		srNew.Top = srNew.Bottom - h;
		if( srNew.Top <= 0 )
			srNew.Top = 1;
	}

	if( srNew.Right >= shared->screen_info.dwSize.X ) {
		srNew.Left = shared->screen_info.dwSize.X - 1;
		srNew.Left = srNew.Right - w;
		if( srNew.Left <= 0 )
			srNew.Left = 1;
	}

	SetConsoleWindowInfo(hStdOut, TRUE, &srNew);

	return read_console_buffer(shared);
}

static BOOL console_on_resize(ShareMemory* shared) {
	COORD newVal = shared->tohook_resize_val;
	SMALL_RECT srNew = shared->screen_info.srWindow;
	HANDLE hStdOut;

	// TODO : width resize, now not use width
	srNew.Bottom = srNew.Top + newVal.Y - 1;

	if( srNew.Bottom > srNew.Top ) {
		hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if( GetConsoleScreenBufferInfo(hStdOut,&(shared->screen_info)) ) {
			if( shared->screen_info.dwCursorPosition.Y > srNew.Bottom ) {
				srNew.Bottom = shared->screen_info.dwCursorPosition.Y;
				srNew.Top = srNew.Bottom - newVal.Y + 1;
			}
		}

		SetConsoleWindowInfo(hStdOut, TRUE, &srNew);
	}

	return read_console_buffer(shared);
}

static BOOL console_on_send_input(ShareMemory* shared) {
	HANDLE hStdIn;

	WCHAR* ps;
	WCHAR* pe;
	INPUT_RECORD* pd;
	INPUT_RECORD records[SEND_INPUT_CHARS_MAX+1];

	hStdIn = CreateFile( L"CONIN$"
		, GENERIC_WRITE | GENERIC_READ
		, FILE_SHARE_READ | FILE_SHARE_WRITE
		, NULL
		, OPEN_EXISTING
		, 0
		, 0 );

	ps = shared->tohook_input_text;
	pe = ps + SEND_INPUT_CHARS_MAX;
	pd = records;
	for( ; ps<pe && *ps; ++ps ) {
		if( *ps==L'\r' || *ps==L'\n' ) {
			if( *ps==L'\r' && *(ps+1)==L'\n' )
				++ps;

			if( pd > records ) {
				WriteConsoleInput(hStdIn, records, (pd-records), 0);
				pd = records;
			}

			PostMessage(shared->hwnd, WM_KEYDOWN, VK_RETURN, 0x001C0001);
			PostMessage(shared->hwnd, WM_KEYUP, VK_RETURN, 0xC01C0001);
			continue;
		}

		pd->EventType = KEY_EVENT;
		pd->Event.KeyEvent.bKeyDown = TRUE;
		pd->Event.KeyEvent.wRepeatCount = 1;
		pd->Event.KeyEvent.wVirtualKeyCode = LOBYTE(VkKeyScan(*ps));
		pd->Event.KeyEvent.wVirtualScanCode = 0;
		pd->Event.KeyEvent.uChar.UnicodeChar = *ps;
		pd->Event.KeyEvent.dwControlKeyState = 0;
		++pd;
	}

	if( pd > records )
		WriteConsoleInput(hStdIn, records, (pd-records), 0);

	CloseHandle(hStdIn);
	return TRUE;
}

static void hook_service_run(ShareMemory* shared, HANDLE hParent) {
	DWORD dwRet;
	BOOL bRunSign;
	HANDLE hEvents[] = {
		  hParent
		, shared->tohook_event_quit
		, shared->tohook_event_alive
		, shared->tohook_event_scroll
		, shared->tohook_event_resize
		, shared->tohook_event_send_input
	};
	EvHanle hHandles[] = {
		  service_quit
		, service_quit
		, service_continue
		, console_on_scroll
		, console_on_resize
		, console_on_send_input
	};
	DWORD dwEventCount = sizeof(hEvents)/sizeof(HANDLE);

	read_console_buffer(shared);

	bRunSign = TRUE;

	while( bRunSign ) {
		dwRet = WaitForMultipleObjects(dwEventCount, hEvents, FALSE, shared->screen_update_time);
		if( dwRet==WAIT_TIMEOUT ) {
			read_console_buffer(shared);
			continue;

		} else if( dwRet==WAIT_FAILED ) {
			bRunSign = FALSE;
			continue;

		} else {
			dwRet -= WAIT_OBJECT_0;
			bRunSign = (*hHandles[dwRet])(shared);
		}
	}
}

DWORD HookService(HANDLE hShareMem) {
	ShareMemory* shared;
	HANDLE hParent;
	HWND hWnd = GetConsoleWindow();

	ShowWindow(hWnd, SW_HIDE);

	shared = MapViewOfFile(hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if( !shared )
		return 1;

	shared->hwnd = hWnd;
	{
		SMALL_RECT* srWindow;
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(hStdOut, &(shared->screen_info));
		srWindow = &(shared->screen_info.srWindow);
		srWindow->Bottom = srWindow->Top + 13;
		SetConsoleWindowInfo(hStdOut, TRUE, srWindow);
	}

	hParent = OpenProcess(PROCESS_ALL_ACCESS, FALSE, shared->owner_pid);

	hook_service_run(shared, hParent);

	CloseHandle(hParent);

	UnmapViewOfFile(shared);
	CloseHandle(hShareMem);

	ShowWindow(hWnd, SW_SHOW);
	SendMessage(hWnd, WM_CLOSE, 0, 0);

	ExitProcess(0);
	return 0;
}

