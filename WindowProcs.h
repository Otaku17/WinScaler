#pragma once

// ============================================================
// Window procedures : panel (surface de rendu) et host
// (fenêtre top-level, faux plein écran, timer, focus).
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
