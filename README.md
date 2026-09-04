# MouseMover

[![CI and Release](https://github.com/DolphinWing/MouseMover/actions/workflows/ci.yml/badge.svg)](https://github.com/DolphinWing/MouseMover/actions/workflows/ci.yml)
[![Release](https://img.shields.io/badge/release-v2.0.0-blue.svg)](https://github.com/DolphinWing/MouseMover/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%20(Win32)-lightgrey.svg)](https://microsoft.com/windows)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

**MouseMover** is an ultra-lightweight (~12 KB), zero-dependency Win32 background utility designed to run in the Windows Notification Area (System Tray). It prevents screen locks, display sleeping, and idle detection (such as aggressive enterprise GPO policies) by simulating micro-movements when genuine user inactivity is detected.

---

## Why MouseMover? (Architecture & Trade-offs)

### 1. Synthetic Input vs. `SetThreadExecutionState`
Many power-saving prevention tools rely exclusively on the Win32 `SetThreadExecutionState` API. While effective against standard OS display sleeps, it **fails against enterprise environments** where Group Policy (GPO), EDR agents, or monitoring software track user input queues to enforce workstation locking.
MouseMover uses `SendInput` to send relative micro-movements (`dx: +1, dy: 0` followed immediately by `dx: -1, dy: 0`). This resets the Windows session idle timer without displacing the cursor or disrupting user workflow.

### 2. Zero-Overhead Idle Detection (`GetLastInputInfo`)
Earlier versions and naive mouse movers install low-level Windows hooks (`WH_MOUSE_LL` and `WH_KEYBOARD_LL`), which intercept every keystroke and mouse motion system-wide, introducing input latency and risking removal by the OS if a callback exceeds `LowLevelHooksTimeout`.
MouseMover 2.0 periodically polls `GetLastInputInfo` (every 10 seconds), introducing **zero hook overhead and zero context switch penalties**.

### 3. Native System Tray Resilience
- **Zero Taskbar Clutter**: Runs via a hidden top-level window; never occupies the taskbar.
- **Single Instance**: Enforced via a named Win32 mutex (`dolphin.apps.win32.MouseMover.Mutex`).
- **Explorer Crash Recovery**: Listens to the `TaskbarCreated` broadcast message to automatically restore the tray icon if `explorer.exe` restarts.
- **Standard Menu Behavior**: Adheres to Win32 KB135788 to ensure context menus dismiss reliably on outside clicks.

---

## Features

- **Ultra-Lightweight**: ~12 KB compiled native binary, ~2-3 MB runtime memory footprint.
- **Zero Dependencies**: Pure Win32 C++, linked solely against standard Windows libraries (`kernel32`, `user32`, `advapi32`, `shell32`).
- **Configurable Intervals**: Choose between 1 Minute, 3 Minutes, or 5 Minutes idle thresholds.
- **Registry Persistence**: User preferences are saved under `HKCU\Software\dolphin.apps.win32.MouseMover`.
- **Manual Trigger**: "Trigger Move Now" option for immediate one-click testing.
- **Delayed Boot Startup**: Avoids enterprise "Login Storms" with an included Task Scheduler script.

---

## Context Menu

Right-click (or left-click) the notification area icon to open the context menu:

| Menu Item | Description |
| :--- | :--- |
| **MouseMover v2.0.0** | Version header |
| **Enabled** | Toggle between active monitoring and paused state |
| **Idle Interval** | Set idle threshold before simulated movement (`1m`, `3m`, `5m`) |
| **Trigger Move Now** | Manually execute a simulated micro-movement once |
| **Exit** | Remove tray icon, destroy timers, and terminate cleanly |

---

## Delayed Startup Setup (Avoiding Login Storms)

In corporate environments with slow hard drives or heavy background security agents, auto-starting programs via the standard `HKCU\...\Run` registry key can worsen login latency ("Login Storm") and trigger security audits.

MouseMover provides Task Scheduler scripts that start the tool **2 minutes after user logon** with standard non-elevated user permissions:

- **Install**: Run `install-startup-task.bat`
- **Uninstall**: Run `uninstall-startup-task.bat`

---

## Building from Source

### Prerequisites
- Windows 10 / 11
- Visual Studio 2019 or Visual Studio 2022 (with the "Desktop development with C++" workload)

### Build via Command Line (MSBuild)
```cmd
msbuild MouseMover.sln /p:Configuration=Release /p:Platform=Win32
```
The compiled binary will be located at `Release\MouseMover.exe`.

---

## Versioning

MouseMover adheres to [Semantic Versioning](https://semver.org/).
Version numbers are centralized in `MouseMover/Version.h` as a Single Source of Truth (SSOT), synchronizing the Windows binary `VERSIONINFO` metadata (`Resource.rc`) and the runtime UI.

---

## License

Licensed under the [MIT License](LICENSE).  
Copyright (c) DolphinWing.
