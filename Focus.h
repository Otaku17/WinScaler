#pragma once

// ============================================================
// INPUT / FOCUS
//
// Le jeu Ruby appartient à un AUTRE processus.
//
// Pour que le clavier reste réellement sur la fenêtre Ruby,
// on attache temporairement les queues d'entrée des deux
// threads pendant l'opération de focus.
//
// Aucun changement de layout/upscale ici.
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
