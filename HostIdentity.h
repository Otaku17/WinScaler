#pragma once

// ============================================================
// Host title / icon
//
// The host window shows "Edelweiss_Chronicles" and the generic
// application icon by default. They get replaced by the game
// window's actual title and icon as soon as it's available.
// ============================================================

#include "AppState.h"

void SyncHostTitle();

// --------------------------------------------------------
// Icon: first the game window/class icon, otherwise the icon
// of the launched executable (Game.exe usually has the game's
// icon, even when the Ruby window itself doesn't).
//
// Loaded only once per attached game window: call
// ResetHostIcon() before retrying if the game gets reattached.
// --------------------------------------------------------

void ResetHostIcon();

void SyncHostIcon();
