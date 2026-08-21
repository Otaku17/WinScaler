#pragma once

// ============================================================
// DPI awareness
//
// Sans ça, Windows virtualise les coordonnées (GetClientRect,
// MonitorInfo...) sur un écran mis à l'échelle (125%, 150%...)
// et ré-étire ensuite toute la fenêtre en bitmap : le calcul
// d'upscale entier (LetterBox) se base alors sur une résolution
// plus petite que la résolution physique réelle, perdant des
// paliers de scale entiers pour rien, en plus de flouter tout
// le rendu. Doit être appelé avant toute création de fenêtre
// ou lecture de taille d'écran.
// ============================================================

#include "AppState.h"

void EnableDpiAwareness();
