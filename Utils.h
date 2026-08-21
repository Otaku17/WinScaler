#pragma once

// ============================================================
// Generic utilities (paths, aspect ratio, window size).
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
