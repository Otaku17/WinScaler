#pragma once

// ============================================================
// Configuration (WinScaler.ini)
//
// Optional file placed next to the exe. If absent, a default
// file is created.
//
// [Settings]
// GameExe=game.exe          -> name or path of the exe to launch
// WindowedUpscale=1         -> 1 = upscale follows the host window
//                               in windowed/maximized mode
// MaxUpscale=0              -> maximum scale (0 = unlimited)
// ScaleMode=Fit             -> scaling mode, used in fake
//                               fullscreen and in windowed
//                               upscale
// NativeWidth=0             -> forced native resolution of the
// NativeHeight=0               game (0 = auto-detect)
// ============================================================

#include "AppState.h"

void LoadConfig();
