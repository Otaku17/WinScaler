#pragma once

// ============================================================
// INPUT / FOCUS
//
// The Ruby game belongs to ANOTHER process.
//
// To keep the keyboard actually on the Ruby window, the input
// queues of both threads are temporarily attached during the
// focus operation.
//
// No layout/upscale changes happen here.
// ============================================================

#include "AppState.h"

bool IsGameWindowValid();

bool IsHostWindowValid();

void FocusGame();

void RequestFocusGame();

bool ForwardKeyboardMessageToGame(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);
