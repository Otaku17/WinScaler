#include "Utils.h"

// ============================================================
// Utilitaires
// ============================================================

std::wstring GetExecutableDir()
{
    wchar_t path[MAX_PATH] = {};

    GetModuleFileNameW(
        nullptr,
        path,
        MAX_PATH);

    std::wstring p(path);

    size_t pos =
        p.find_last_of(L"\\/");

    if (pos != std::wstring::npos)
        p.resize(pos);

    return p;
}

bool FileExists(
    const std::wstring& path)
{
    DWORD attr =
        GetFileAttributesW(
            path.c_str());

    return
        attr != INVALID_FILE_ATTRIBUTES &&
        !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool IsAbsolutePathW(
    const std::wstring& p)
{
    if (p.size() >= 2 &&
        p[1] == L':')
    {
        return true;
    }

    if (p.size() >= 2 &&
        p[0] == L'\\' &&
        p[1] == L'\\')
    {
        return true;
    }

    return false;
}

// ============================================================
// Ratio
// ============================================================

double GetAspectRatio(
    int width,
    int height)
{
    if (width <= 0 ||
        height <= 0)
    {
        return 4.0 / 3.0;
    }

    return
        static_cast<double>(width) /
        static_cast<double>(height);
}

// ============================================================
// Taille client d'une fenêtre
// ============================================================

bool GetClientSize(
    HWND hwnd,
    int& width,
    int& height)
{
    width = 0;
    height = 0;

    if (!hwnd ||
        !IsWindow(hwnd))
    {
        return false;
    }

    RECT rc = {};

    if (!GetClientRect(
        hwnd,
        &rc))
    {
        return false;
    }

    width =
        rc.right - rc.left;

    height =
        rc.bottom - rc.top;

    return
        width > 0 &&
        height > 0;
}
