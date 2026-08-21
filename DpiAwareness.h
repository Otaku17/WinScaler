#pragma once

// ============================================================
// DPI awareness
//
// Without this, Windows virtualizes coordinates (GetClientRect,
// MonitorInfo...) on a scaled display (125%, 150%...) and then
// bitmap-stretches the whole window back: the integer upscale
// (LetterBox) calculation then relies on a smaller-than-actual
// physical resolution, losing integer scale steps for nothing,
// on top of blurring the whole render. Must be called before
// any window creation or screen size reading.
// ============================================================

#include "AppState.h"

void EnableDpiAwareness();
