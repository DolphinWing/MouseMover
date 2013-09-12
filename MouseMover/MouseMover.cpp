// MouseMover.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <winioctl.h>
//#include <winable.h>

#define TIMER_CHECK_TIMEOUT		(60*1000*5)

HHOOK g_hMouse;
HHOOK g_hKeyBoard;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
VOID CALLBACK MyTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

int _tmain(int argc, _TCHAR* argv[])
{
	MSG msg;

	printf("SetWindowsHookEx to WH_MOUSE_LL/WH_KEYBOARD_LL\n");
	g_hMouse = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
	if (!g_hMouse) {
		printf("Hook Mouse error: %d\n", GetLastError());
		return -1;//end program
	}

	g_hKeyBoard = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
	if (!g_hKeyBoard) {
		printf("Hook Keyboard error: %d\n", GetLastError());
		return -1;//end program
	}

	//printf("SetTimer\n");
	SetTimer(NULL, 1, TIMER_CHECK_TIMEOUT, (TIMERPROC) MyTimerProc);

	printf("start hacking, you can put it to background\n");
	while ( GetMessage(&msg, NULL, 0, 0) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//printf("KillTimer\n");
	KillTimer(NULL, 1);

	printf("UnhookWindowsHookEx\n");
	if(g_hKeyBoard) UnhookWindowsHookEx(g_hKeyBoard);
	if(g_hMouse) UnhookWindowsHookEx(g_hMouse);

	return 0;
}

static int mouseX = 0, lastMouseX = 0, lastPadX = 0;
static int mouseY = 0, lastMouseY = 0, lastPadY = 0;
static bool tflag =false;
//bool toggle = false;

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms644986(v=vs.85).aspx
//nCode [in]
//	Type: int
//		A code the hook procedure uses to determine how to process the message. 
//wParam [in]
//	Type: WPARAM
//		The identifier of the mouse message. This parameter can be one of the 
//		following messages: WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE, 
//		WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_RBUTTONDOWN, or WM_RBUTTONUP.
//lParam [in]
//	Type: LPARAM
//		A pointer to an MSLLHOOKSTRUCT structure.
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
	tflag = true;

	PMSLLHOOKSTRUCT pmll = (PMSLLHOOKSTRUCT) lParam;	  		
	//MSLLHOOKSTRUCT *info=(MSLLHOOKSTRUCT*) lParam;
	mouseX = pmll->pt.x;
	mouseY = pmll->pt.y;

	//printf("msg: %lu, x:%ld, y:%ld\n", wParam, pmll->pt.x, pmll->pt.y);

	return CallNextHookEx(g_hMouse, nCode, wParam, lParam);
} 

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms644985(v=vs.85).aspx
//nCode [in]
//	Type: int
//		A code the hook procedure uses to determine how to process the message. 
//wParam [in]
//	Type: WPARAM
//		The identifier of the keyboard message. This parameter can be one of the 
//		following messages: WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN or WM_SYSKEYUP.
//lParam [in]
//	Type: LPARAM
//		A pointer to a KBDLLHOOKSTRUCT structure.
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	tflag = true;

	//printf("msg: %lu, code: %ld\n", code, wParam);

	return CallNextHookEx(g_hMouse, code, wParam, lParam);
}

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms644907(v=vs.85).aspx
//hwnd [in]
//	Type: HWND
//		A handle to the window associated with the timer.
//uMsg [in]
//	Type: UINT
//		The WM_TIMER message.
//idEvent [in]
//	Type: UINT_PTR
//		The timer's identifier.
//dwTime [in]
//	Type: DWORD
//		The number of milliseconds that have elapsed since the system was started. 
VOID CALLBACK MyTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if(tflag) {
		//printf("MyTimerProc %d %d %d\n", uMsg, idEvent, dwTime);
		printf(".");
	}
	else {//no one moves the mouse or type any keys
		//http://msdn.microsoft.com/en-us/library/windows/desktop/ms724950(v=vs.85).aspx
		SYSTEMTIME /*st, */lt;

		//GetSystemTime(&st);
		GetLocalTime(&lt);

		printf("\nTimerProc: send fake move to system %d %d (%02d:%02d)\n", 
			mouseX, mouseY, lt.wHour, lt.wMinute);
		//printf("MyTimerProc %d %d\n", uMsg, idEvent);
		//SetCursorPos(mouseX+10, mouseY+10);
		//mouse_event(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE, mouseX+10, mouseY+10, 0, 0);
		SetCursorPos(mouseX, mouseY);

		//http://msdn.microsoft.com/en-us/library/windows/desktop/ms646260(v=vs.85).aspx
		//mouse_event(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE, mouseX, mouseY, NULL, NULL);
		//http://edisonx.pixnet.net/blog/post/37622951-%5Bw%5D-mouse-and-keybd
		mouse_event(MOUSEEVENTF_MOVE, 1, 1, NULL, NULL);//move is relative points
		//mouse_event(MOUSEEVENTF_LEFTUP|MOUSEEVENTF_ABSOLUTE, mouseX, mouseY, NULL, NULL);

		SetCursorPos(mouseX, mouseY);//set back to old position
	}

	tflag = false;//always set the flag false
}
