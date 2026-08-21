#pragma once

// ============================================================
// Résolution du jeu, calcul du scale et positionnement dans
// le panel.
// ============================================================

#include "AppState.h"

bool SetGameResolution(
    int width,
    int height);

// ============================================================
// Layout
//
// Fenêtré normal / maximisé :
//     jeu natif 1:1, centré.
//
// Faux plein écran :
//     upscale entier, centré.
// ============================================================

void LayoutGame();

// ============================================================
// Redimensionne le host pour avoir EXACTEMENT la taille
// client demandée.
//
// Utilisé uniquement en fenêtre normale.
// ============================================================

void ResizeHostToClient(
    HWND hwnd,
    int clientW,
    int clientH);

// ============================================================
// Surveillance résolution
// ============================================================

void PollGameResolution();
