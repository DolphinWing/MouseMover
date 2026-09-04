// MouseMover.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

// Default idle threshold before simulating movement (5 minutes)
static const DWORD DEFAULT_IDLE_TIMEOUT_MS = 5 * 60 * 1000;

// Timer interval to check system idle status (10 seconds)
static const UINT CHECK_INTERVAL_MS = 10 * 1000;

// Simulated movement relative offset in pixels
static const LONG MOUSE_OFFSET_DELTA = 1;

static DWORD g_idleTimeoutMs = DEFAULT_IDLE_TIMEOUT_MS;

/**
 * Simulates micro mouse movements using SendInput.
 * Moves (+1, 0) then immediately (-1, 0) to reset the system idle timer
 * without altering the visible cursor position.
 */
void SimulateMouseMove()
{
    INPUT inputs[2] = {};

    // Move right by 1 pixel
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = MOUSE_OFFSET_DELTA;
    inputs[0].mi.dy = 0;
    inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;

    // Move back left by 1 pixel
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dx = -MOUSE_OFFSET_DELTA;
    inputs[1].mi.dy = 0;
    inputs[1].mi.dwFlags = MOUSEEVENTF_MOVE;

    UINT sent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (sent != ARRAYSIZE(inputs))
    {
        printf("SendInput failed with error: %lu\n", GetLastError());
    }
}

/**
 * Timer procedure called periodically to check system idle time via GetLastInputInfo.
 */
VOID CALLBACK CheckIdleTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    LASTINPUTINFO lii = {};
    lii.cbSize = sizeof(LASTINPUTINFO);

    if (!GetLastInputInfo(&lii))
    {
        printf("GetLastInputInfo failed: %lu\n", GetLastError());
        return;
    }

    DWORD currentTick = GetTickCount();
    // Unsigned subtraction correctly handles 49.7 day tick rollover
    DWORD idleMs = currentTick - lii.dwTime;

    if (idleMs >= g_idleTimeoutMs)
    {
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        printf("\n[%02d:%02d:%02d] Idle: %lu ms (>= %lu ms). Triggering synthetic mouse move.\n",
               lt.wHour, lt.wMinute, lt.wSecond, idleMs, g_idleTimeoutMs);

        SimulateMouseMove();
    }
    else
    {
        // Heartbeat dot indicating active monitoring
        printf(".");
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    printf("====================================================\n");
    printf(" MouseMover - Idle Prevention (Phase 1: Core Engine)\n");
    printf("====================================================\n");
    printf("Idle Timeout   : %lu seconds\n", g_idleTimeoutMs / 1000);
    printf("Check Interval : %u seconds\n", CHECK_INTERVAL_MS / 1000);
    printf("Monitoring system idle status via GetLastInputInfo...\n\n");

    UINT_PTR timerId = SetTimer(NULL, 0, CHECK_INTERVAL_MS, (TIMERPROC)CheckIdleTimerProc);
    if (!timerId)
    {
        printf("Failed to create timer: %lu\n", GetLastError());
        return -1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(NULL, timerId);
    return 0;
}
