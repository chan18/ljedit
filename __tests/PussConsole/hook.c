// hook.c
// 

#define _WIN32_WINNT 0x500
#include <Windows.h>
#include <stdio.h>

#include "share.h"

#define VCONSOLE_EVENT_QUIT		0
#define VCONSOLE_EVENT_ALIVE	1
#define VCONSOLE_EVENT_MAX		2

#define ALIVE_TIMEOUT	30000

// global vars only for this file
// 

static void read_console_buffer() {
	/*
	HANDLE hStdOut;
	COORD pos;
	CONSOLE_SCREEN_BUFFER_INFO si;

	// Notice : Copy FROM Console2
	//
	// we take a fresh STDOUT handle - seems to work better (in case a program
	// has opened a new screen output buffer)
	//
	hStdOut = CreateFile( L"CONOUT$"
		, GENERIC_WRITE | GENERIC_READ
		, FILE_SHARE_READ | FILE_SHARE_WRITE
		, NULL
		, OPEN_EXISTING
		, 0
		, 0 );

	// get total console size
	GetConsoleScreenBufferInfo(hStdOut, &si);
	pos.X	= si.srWindow.Right - si.srWindow.Left + 1;
	pos.Y	= si.srWindow.Bottom - si.srWindow.Top + 1;

	// do console output buffer reading
	DWORD					dwScreenBufferSize	= coordConsoleSize.X * coordConsoleSize.Y;
	DWORD					dwScreenBufferOffset= 0;

	shared_array<CHAR_INFO> pScreenBuffer(new CHAR_INFO[dwScreenBufferSize]);

	COORD		coordBufferSize;
	// start coordinates for the buffer are always (0, 0) - we use offset
	COORD		coordStart = {0, 0};
	SMALL_RECT	srBuffer;

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

	TRACE(L"Buffer size for loop reads: %ix%i\n", coordBufferSize.X, coordBufferSize.Y);
	TRACE(L"-------------------------------------------------------------------\n");

	// read rows 'chunks'
	SHORT i = 0;
	for (; i < coordConsoleSize.Y / coordBufferSize.Y; ++i)
	{
//		TRACE(L"Reading region: (%i, %i) - (%i, %i)\n", srBuffer.Left, srBuffer.Top, srBuffer.Right, srBuffer.Bottom);

		::ReadConsoleOutput(
			hStdOut.get(), 
			pScreenBuffer.get() + dwScreenBufferOffset, 
			coordBufferSize, 
			coordStart, 
			&srBuffer);

		srBuffer.Top		= srBuffer.Top + coordBufferSize.Y;
		srBuffer.Bottom		= srBuffer.Bottom + coordBufferSize.Y;

		dwScreenBufferOffset += coordBufferSize.X * coordBufferSize.Y;
	}

	// read the last 'chunk', we need to calculate the number of rows in the
	// last chunk and update bottom coordinate for the region
	coordBufferSize.Y	= coordConsoleSize.Y - i * coordBufferSize.Y;
	srBuffer.Bottom		= csbiConsole.srWindow.Bottom;

	TRACE(L"Buffer size for last read: %ix%i\n", coordBufferSize.X, coordBufferSize.Y);
	TRACE(L"-------------------------------------------------------------------\n");
	TRACE(L"Reading region: (%i, %i) - (%i, %i)\n", srBuffer.Left, srBuffer.Top, srBuffer.Right, srBuffer.Bottom);

	::ReadConsoleOutput(
		hStdOut.get(), 
		pScreenBuffer.get() + dwScreenBufferOffset, 
		coordBufferSize, 
		coordStart, 
		&srBuffer);


//	TRACE(L"===================================================================\n");

	// compare previous buffer, and if different notify Console
	if ((::memcmp(m_consoleInfo.Get(), &csbiConsole, sizeof(CONSOLE_SCREEN_BUFFER_INFO)) != 0) ||
		(m_dwScreenBufferSize != dwScreenBufferSize) ||
		(::memcmp(m_consoleBuffer.Get(), pScreenBuffer.get(), m_dwScreenBufferSize*sizeof(CHAR_INFO)) != 0))
	{
		SharedMemoryLock bufferLock(m_consoleBuffer);

		// update screen buffer variables
		m_dwScreenBufferSize = dwScreenBufferSize;
		::CopyMemory(m_consoleBuffer.Get(), pScreenBuffer.get(), m_dwScreenBufferSize*sizeof(CHAR_INFO));
		::CopyMemory(m_consoleInfo.Get(), &csbiConsole, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
		::GetConsoleCursorInfo(hStdOut.get(), m_cursorInfo.Get());

		m_consoleBuffer.SetReqEvent();
	}
	
	CloseHandle(hStdOut);
	*/
}

DWORD HookService(HANDLE hShareMem) {
	HANDLE	hEvents[VCONSOLE_EVENT_MAX];
	DWORD dwRet;
	BOOL bRunSign;
	ShareMemory* shared;
	DWORD dwLastAlive;

	printf("service start : %p  - %p\n", hShareMem, &hShareMem);

	//ShowWindow(GetConsoleWindow(), SW_HIDE);

	printf("hook monitor thread!\n");
	shared = MapViewOfFile(hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	printf("hH:%p  gg:%p\n",  hShareMem, shared);

	if( !shared )
		return 1;

	hEvents[VCONSOLE_EVENT_QUIT] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvents[VCONSOLE_EVENT_ALIVE] = shared->event_alive;

	bRunSign = TRUE;
	dwLastAlive = GetTickCount();

	while( bRunSign ) {
		dwRet = WaitForMultipleObjects(VCONSOLE_EVENT_MAX, hEvents, FALSE, 1000);
        switch(dwRet) {
        case WAIT_TIMEOUT:
			++shared->testb;
			printf("testa: %d   testb:%d	ret:%d\n", shared->testa, shared->testb, dwRet);
			if( (GetTickCount() - dwLastAlive) > ALIVE_TIMEOUT )
				bRunSign = FALSE;
            break;

        case WAIT_FAILED:
            printf("wait failed\n");
			bRunSign = FALSE;
            break;

		default:
			dwLastAlive = GetTickCount();
			dwRet -= WAIT_OBJECT_0;
			switch( dwRet ) {
			case VCONSOLE_EVENT_QUIT:
				printf("wait quit event\n");
				bRunSign = FALSE;
			case VCONSOLE_EVENT_ALIVE:
				printf("alive event\n");
				break;
			}
        }
	}

	UnmapViewOfFile(shared);
	CloseHandle(hShareMem);

	printf("thread stoped!\n");
	ExitProcess(0);
	return 0;
}
