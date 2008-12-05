// shared.h
// 

#ifndef PUSS_VCONSOLE_SHARE_H
#define PUSS_VCONSOLE_SHARE_H

#include <Windows.h>

typedef struct _ShareMemory {
	DWORD	owner_pid;
	HANDLE	event_alive;

	CONSOLE_SCREEN_BUFFER_INFO	screen_info;
	DWORD	testa;
	DWORD	testb;
} ShareMemory;

#endif//PUSS_VCONSOLE_SHARE_H

