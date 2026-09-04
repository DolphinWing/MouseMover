// MouseMover.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>

// Unique message and identifiers
#define WM_TRAYICON            (WM_APP + 1)
#define TIMER_CHECK_ID         1

// Context menu command identifiers
#define IDM_TOGGLE_ENABLE      1001
#define IDM_INTERVAL_1M        1002
#define IDM_INTERVAL_3M        1003
#define IDM_INTERVAL_5M        1004
#define IDM_TRIGGER_NOW        1005
#define IDM_EXIT               1006

// Application constants
static const WCHAR WINDOW_CLASS_NAME[] = L"dolphin.apps.win32.MouseMover.WndClass";
static const WCHAR WINDOW_TITLE[]      = L"MouseMover";
static const WCHAR MUTEX_NAME[]         = L"dolphin.apps.win32.MouseMover.Mutex";

// Timer interval to check system idle status (every 10 seconds)
static const UINT CHECK_INTERVAL_MS = 10 * 1000;

// Runtime state
static HINSTANCE g_hInstance = NULL;
static NOTIFYICONDATAW g_nid = {};
static UINT g_uTaskbarRestartMsg = 0;

static bool  g_bEnabled       = true;
static DWORD g_idleTimeoutMs  = 5 * 60 * 1000; // Default: 5 minutes

/**
 * Loads a crisp built-in icon matching small tray icon metrics.
 */
static HICON GetTrayAppIcon()
{
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);
    return (HICON)LoadImageW(NULL, IDI_INFORMATION, IMAGE_ICON, cx, cy, LR_SHARED);
}

/**
 * Simulates micro mouse movements using SendInput.
 * Relative motion (+1, 0) followed by (-1, 0) resets Windows idle timer
 * while leaving visible cursor coordinates unchanged.
 */
static void SimulateMouseMove()
{
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = 1;
    inputs[0].mi.dy = 0;
    inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dx = -1;
    inputs[1].mi.dy = 0;
    inputs[1].mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

/**
 * Formats status tooltip string based on current runtime state.
 */
static void FormatTooltip(LPWSTR szBuf, size_t cchBuf)
{
    UINT intervalMins = g_idleTimeoutMs / (60 * 1000);
    if (g_bEnabled)
    {
        StringCchPrintfW(szBuf, cchBuf, L"MouseMover - Active (%u min)", intervalMins);
    }
    else
    {
        StringCchCopyW(szBuf, cchBuf, L"MouseMover - Paused");
    }
}

/**
 * Updates the tray icon tooltip with current active/paused state and interval.
 */
static void UpdateTrayTooltip(HWND hWnd)
{
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd   = hWnd;
    nid.uID    = 1;
    nid.uFlags = NIF_TIP;

    FormatTooltip(nid.szTip, ARRAYSIZE(nid.szTip));

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

/**
 * Adds the application icon to the system notification area.
 */
static BOOL AddTrayIcon(HWND hWnd)
{
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize           = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd             = hWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = GetTrayAppIcon();

    FormatTooltip(g_nid.szTip, ARRAYSIZE(g_nid.szTip));

    return Shell_NotifyIconW(NIM_ADD, &g_nid);
}

/**
 * Removes the application icon from the notification area.
 */
static void RemoveTrayIcon(HWND hWnd)
{
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd   = hWnd;
    nid.uID    = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

/**
 * Displays the context menu at the current cursor position.
 */
static void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
    {
        return;
    }

    // Toggle Active / Paused
    AppendMenuW(hMenu, MF_STRING | (g_bEnabled ? MF_CHECKED : MF_UNCHECKED),
                IDM_TOGGLE_ENABLE, L"Enabled");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // Interval submenu
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING | (g_idleTimeoutMs == 1 * 60 * 1000 ? MF_CHECKED : MF_UNCHECKED),
                IDM_INTERVAL_1M, L"1 Minute");
    AppendMenuW(hSubMenu, MF_STRING | (g_idleTimeoutMs == 3 * 60 * 1000 ? MF_CHECKED : MF_UNCHECKED),
                IDM_INTERVAL_3M, L"3 Minutes");
    AppendMenuW(hSubMenu, MF_STRING | (g_idleTimeoutMs == 5 * 60 * 1000 ? MF_CHECKED : MF_UNCHECKED),
                IDM_INTERVAL_5M, L"5 Minutes");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"Idle Interval");

    // Move Now
    AppendMenuW(hMenu, MF_STRING, IDM_TRIGGER_NOW, L"Trigger Move Now");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // Exit
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Exit");

    // Required by Win32 KB135788 to ensure popup menu dismisses properly
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    PostMessageW(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

/**
 * Checks system idle time using GetLastInputInfo and triggers simulated move if threshold reached.
 */
static void CheckIdleTime()
{
    if (!g_bEnabled)
    {
        return;
    }

    LASTINPUTINFO lii = {};
    lii.cbSize = sizeof(LASTINPUTINFO);

    if (GetLastInputInfo(&lii))
    {
        DWORD currentTick = GetTickCount();
        DWORD idleMs = currentTick - lii.dwTime;

        if (idleMs >= g_idleTimeoutMs)
        {
            SimulateMouseMove();
        }
    }
}

/**
 * Window procedure for the hidden message window.
 */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uTaskbarRestartMsg)
    {
        // Re-add icon if Windows Explorer restarts
        AddTrayIcon(hWnd);
        return 0;
    }

    switch (uMsg)
    {
    case WM_CREATE:
        AddTrayIcon(hWnd);
        SetTimer(hWnd, TIMER_CHECK_ID, CHECK_INTERVAL_MS, NULL);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_CHECK_ID)
        {
            CheckIdleTime();
        }
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP || lParam == WM_CONTEXTMENU)
        {
            ShowContextMenu(hWnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_TOGGLE_ENABLE:
            g_bEnabled = !g_bEnabled;
            UpdateTrayTooltip(hWnd);
            break;

        case IDM_INTERVAL_1M:
            g_idleTimeoutMs = 1 * 60 * 1000;
            UpdateTrayTooltip(hWnd);
            break;

        case IDM_INTERVAL_3M:
            g_idleTimeoutMs = 3 * 60 * 1000;
            UpdateTrayTooltip(hWnd);
            break;

        case IDM_INTERVAL_5M:
            g_idleTimeoutMs = 5 * 60 * 1000;
            UpdateTrayTooltip(hWnd);
            break;

        case IDM_TRIGGER_NOW:
            SimulateMouseMove();
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_CHECK_ID);
        RemoveTrayIcon(hWnd);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

/**
 * Win32 Application entry point.
 */
int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_     LPWSTR    lpCmdLine,
                      _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Prevent multiple instances running concurrently
    HANDLE hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex)
        {
            CloseHandle(hMutex);
        }
        return 0;
    }

    g_hInstance = hInstance;
    g_uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");

    // Register an invisible window class to receive messages and broadcast events
    WNDCLASSEXW wcex = {};
    wcex.cbSize        = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&wcex))
    {
        if (hMutex)
        {
            CloseHandle(hMutex);
        }
        return 0;
    }

    // Create a hidden top-level window (never shown, so it never appears on the taskbar)
    HWND hWnd = CreateWindowExW(0, WINDOW_CLASS_NAME, WINDOW_TITLE,
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                                NULL, NULL, hInstance, NULL);
    if (!hWnd)
    {
        UnregisterClassW(WINDOW_CLASS_NAME, hInstance);
        if (hMutex)
        {
            CloseHandle(hMutex);
        }
        return 0;
    }

    // Standard message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterClassW(WINDOW_CLASS_NAME, hInstance);
    if (hMutex)
    {
        CloseHandle(hMutex);
    }

    return (int)msg.wParam;
}
