// shared.h
// 

#ifndef PUSS_VCONSOLE_SHARE_H
#define PUSS_VCONSOLE_SHARE_H

#include <Windows.h>

#define SHARED_FILE_NAME_FMT L"@vconsole/shared/%d"

#define MAX_SCR_COL	256
#define MAX_SCR_ROW	256
#define MAX_SCR_LEN	(MAX_SCR_COL*MAX_SCR_ROW)

typedef struct _ShareMemory {
	DWORD	owner_pid;

	HANDLE	tohook_event_quit;
	HANDLE	tohook_event_alive;

	HANDLE	fromhook_event_update;

	HWND		hwnd;
	DWORD		screen_update_time;
	DWORD		screen_buffer_len;
	CHAR_INFO	screen_buffer[MAX_SCR_LEN];
	CONSOLE_SCREEN_BUFFER_INFO	screen_info;
	CONSOLE_CURSOR_INFO			cursor_info;

} ShareMemory;

#endif//PUSS_VCONSOLE_SHARE_H

