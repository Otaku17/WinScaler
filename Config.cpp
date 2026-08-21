#include "Config.h"
#include "Utils.h"

#include <cstring>

static void CreateDefaultConfigFile(
    const std::wstring& path)
{
    const char* defaultContent =
        "[Settings]\r\n"
        "; Name (or full path) of the executable to launch.\r\n"
        "; If relative, it is resolved from this exe's folder.\r\n"
        "GameExe=game.exe\r\n"
        "\r\n"
        "; 0 = windowed/maximized mode stays at the game's\r\n"
        ";     native resolution, centered, no upscale.\r\n"
        "; 1 = the game is upscaled according to ScaleMode,\r\n"
        ";     centered, following the host window's resize\r\n"
        ";     / maximize dynamically.\r\n"
        "WindowedUpscale=1\r\n"
        "\r\n"
        "; Maximum scale applied in fake fullscreen and in\r\n"
        "; windowed upscale. Lower this on a large screen if\r\n"
        "; you want to cap how much the game gets enlarged.\r\n"
        "; 0 = unlimited (the largest scale that fits).\r\n"
        "MaxUpscale=0\r\n"
        "\r\n"
        "; Game scaling mode, used in fake fullscreen\r\n"
        "; (Alt+Enter) and in windowed upscale\r\n"
        "; (WindowedUpscale=1):\r\n"
        ";   LetterBox = INTEGER scale (x1,x2,x3...), centered,\r\n"
        ";               black bars, pixel-perfect.\r\n"
        ";   Stretch   = fills all available space, stretches\r\n"
        ";               the image, ignores aspect ratio.\r\n"
        ";   Fit       = floating-point scale, keeps aspect\r\n"
        ";               ratio, fills as much as possible,\r\n"
        ";               minimal bars.\r\n"
        "ScaleMode=Fit\r\n"
        "\r\n"
        "; Real native resolution of the game. Forces this value\r\n"
        "; instead of auto-detection. Useful if the game (a\r\n"
        "; separate process, not necessarily DPI-aware) reports\r\n"
        "; a slightly wrong size once read from this host, which\r\n"
        "; would throw off the LetterBox integer scale\r\n"
        "; calculation. 0 = auto-detect.\r\n"
        "NativeWidth=640\r\n"
        "NativeHeight=480\r\n";

    HANDLE hFile =
        CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
        return;

    DWORD written = 0;

    WriteFile(
        hFile,
        defaultContent,
        static_cast<DWORD>(
            strlen(defaultContent)),
        &written,
        nullptr);

    CloseHandle(hFile);
}

static ScaleMode ParseScaleMode(
    const wchar_t* s)
{
    if (_wcsicmp(s, L"Stretch") == 0)
        return ScaleMode::Stretch;

    if (_wcsicmp(s, L"Fit") == 0)
        return ScaleMode::Fit;

    return ScaleMode::LetterBox;
}

void LoadConfig()
{
    std::wstring path =
        GetExecutableDir() +
        L"\\" +
        CONFIG_FILE_NAME;

    if (!FileExists(path))
    {
        CreateDefaultConfigFile(
            path);
    }

    wchar_t buffer[MAX_PATH] = {};

    GetPrivateProfileStringW(
        L"Settings",
        L"GameExe",
        L"game.exe",
        buffer,
        MAX_PATH,
        path.c_str());

    if (buffer[0] != L'\0')
        g.gameExeName = buffer;

    g.enableWindowedUpscale =
        GetPrivateProfileIntW(
            L"Settings",
            L"WindowedUpscale",
            1,
            path.c_str()) != 0;

    int maxUpscale =
        GetPrivateProfileIntW(
            L"Settings",
            L"MaxUpscale",
            DEFAULT_MAX_UPSCALE,
            path.c_str());

    g.maxUpscale =
        (maxUpscale < 0) ? 0 : maxUpscale;

    wchar_t scaleModeBuffer[64] = {};

    GetPrivateProfileStringW(
        L"Settings",
        L"ScaleMode",
        L"Fit",
        scaleModeBuffer,
        64,
        path.c_str());

    g.scaleMode =
        ParseScaleMode(
            scaleModeBuffer);

    int nativeWidth =
        GetPrivateProfileIntW(
            L"Settings",
            L"NativeWidth",
            0,
            path.c_str());

    int nativeHeight =
        GetPrivateProfileIntW(
            L"Settings",
            L"NativeHeight",
            0,
            path.c_str());

    g.configuredNativeWidth =
        (nativeWidth > 0) ? nativeWidth : 0;

    g.configuredNativeHeight =
        (nativeHeight > 0) ? nativeHeight : 0;
}
