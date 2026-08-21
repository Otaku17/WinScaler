#pragma once

// ============================================================
// Window procedures: panel (render surface) and host
// (top-level window, fake fullscreen, timer, focus).
// ============================================================

#include "AppState.h"

LRESULT CALLBACK PanelProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT CALLBACK HostProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);
