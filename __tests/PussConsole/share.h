// hook_protocol.h
// 

#ifndef PUSS_VCONSOLE_SHARE_H
#define PUSS_VCONSOLE_SHARE_H

#include <Windows.h>

typedef struct _ShareMemory {
	DWORD	owner_pid;
	HANDLE	event_alive;
	DWORD	testa;
	DWORD	testb;
} ShareMemory;

#endif//PUSS_VCONSOLE_SHARE_H

