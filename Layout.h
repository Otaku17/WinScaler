#pragma once

// ============================================================
// Game resolution, scale calculation and positioning inside
// the panel.
// ============================================================

#include "AppState.h"

bool SetGameResolution(
    int width,
    int height);

// ============================================================
// Layout
//
// Normal / maximized windowed:
//     native 1:1 game, centered.
//
// Fake fullscreen:
//     integer upscale, centered.
// ============================================================

void LayoutGame();

// ============================================================
// Resizes the host to have EXACTLY the requested client size.
//
// Used only in normal windowed mode.
// ============================================================

void ResizeHostToClient(
    HWND hwnd,
    int clientW,
    int clientH);

// ============================================================
// Resolution monitoring
// ============================================================

void PollGameResolution();
