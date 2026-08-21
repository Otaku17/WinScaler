#pragma once

// ============================================================
// Titre / icône host
//
// La fenêtre host affiche par défaut "Edelweiss_Chronicles"
// et l'icône d'application générique. On les remplace par le
// titre et l'icône réels de la fenêtre du jeu encapsulée, dès
// qu'elle est disponible.
// ============================================================

#include "AppState.h"

void SyncHostTitle();

// --------------------------------------------------------
// Icône : d'abord celle de la fenêtre/classe du jeu, sinon
// celle de l'exécutable lancé (Game.exe a en général l'icône
// du jeu, même quand la fenêtre Ruby elle-même n'en a pas).
//
// Chargée une seule fois par fenêtre de jeu attachée : appeler
// ResetHostIcon() avant de réessayer si le jeu est réattaché.
// --------------------------------------------------------

void ResetHostIcon();

void SyncHostIcon();
