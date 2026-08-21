#pragma once

// ============================================================
// Configuration (WinScaler.ini)
//
// Fichier optionnel placé à côté de l'exe. S'il est absent,
// un fichier par défaut est créé.
//
// [Settings]
// GameExe=game.exe          -> nom ou chemin de l'exe à lancer
// WindowedUpscale=1         -> 1 = upscale suit la fenêtre host
//                               en mode fenêtré/maximisé
// MaxUpscale=0              -> scale maximum (0 = illimité)
// ScaleMode=Fit             -> mode de mise à l'échelle, utilisé
//                               en faux plein écran et en
//                               upscale fenêtré
// NativeWidth=0             -> résolution native forcée du jeu
// NativeHeight=0             (0 = auto-détection)
// ============================================================

#include "AppState.h"

void LoadConfig();
