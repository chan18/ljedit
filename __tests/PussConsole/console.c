// console.c
// 

#include <windows.h>

int main(int argc, char* argv[]) {
	FARPROC test;
	HMODULE hDLL = LoadLibrary(L"vconsole.dll");
	if( !hDLL )
		return 1;

	test = GetProcAddress(hDLL, "__test");

	return (*test)();
}


/*
// tevb.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
// teva.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
    HANDLE hHandles[2];
    hHandles[0] = CreateEvent(NULL, FALSE, FALSE, L"tee_eee");
    hHandles[1] = CreateEvent(NULL, FALSE, FALSE, L"tee_fff");
    DWORD dwRet = 0;
    DWORD dwIndex = 0;
    while(TRUE) {
        dwRet = WaitForMultipleObjects(2, hHandles, FALSE, 5000);
        switch(dwRet) {
        case WAIT_TIMEOUT:
            printf("timeout\n");
            break;
        case WAIT_FAILED:
            printf("failed\n");
            break;
        default:
            dwIndex = dwRet - WAIT_OBJECT_0;
            printf("event %d be send!\n", dwIndex);
            break;
        }
    }

    return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
    HANDLE hEvt1 = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"tee_eee");
    HANDLE hEvt2 = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"tee_fff");
    for(int i=0; i<1024; ++i) {
        SetEvent(hEvt1);
        SetEvent(hEvt2);
    }
    CloseHandle(hEvt1);
    CloseHandle(hEvt2);

    return 0;
}

*/
