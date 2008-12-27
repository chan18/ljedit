// shared.h
// 

#ifndef PUSS_VCONSOLE_SHARE_H
#define PUSS_VCONSOLE_SHARE_H

#include <Windows.h>

#define SHARED_FILE_NAME_FMT L"@vconsole/shared/%d"

#define MAX_SCR_COL	256
#define MAX_SCR_ROW	256
#define MAX_SCR_LEN	(MAX_SCR_COL*MAX_SCR_ROW)

#define SEND_INPUT_CHARS_MAX 4096

typedef struct _ShareMemory {
	DWORD		owner_pid;

	HANDLE		tohook_event_quit;
	HANDLE		tohook_event_alive;
	HANDLE		tohook_event_scroll;
	COORD		tohook_scroll_pos;
	HANDLE		tohook_event_resize;
	COORD		tohook_resize_val;
	HANDLE		tohook_event_send_input;
	WCHAR		tohook_input_text[SEND_INPUT_CHARS_MAX+1];

	HANDLE		fromhook_event_update;

	HWND		hwnd;
	DWORD		screen_update_time;
	DWORD		screen_buffer_len;
	CHAR_INFO	screen_buffer[MAX_SCR_LEN];
	CONSOLE_SCREEN_BUFFER_INFO	screen_info;

} ShareMemory;

#endif//PUSS_VCONSOLE_SHARE_H

