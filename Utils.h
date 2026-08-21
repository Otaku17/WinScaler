#pragma once

// ============================================================
// Utilitaires génériques (chemins, ratio, taille de fenêtre).
// ============================================================

#include "AppState.h"

std::wstring GetExecutableDir();

bool FileExists(
    const std::wstring& path);

bool IsAbsolutePathW(
    const std::wstring& p);

double GetAspectRatio(
    int width,
    int height);

bool GetClientSize(
    HWND hwnd,
    int& width,
    int& height);
