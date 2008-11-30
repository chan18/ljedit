// hook_service.c
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
