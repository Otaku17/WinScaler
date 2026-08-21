#pragma once

// ============================================================
// Global application state, types and shared constants.
//
// Included by every module.
// ============================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <string>

// ============================================================
// Constants
// ============================================================

constexpr int BASE_WIDTH = 320;
constexpr int BASE_HEIGHT = 240;

// --------------------------------------------------------
// Default maximum scale (fake fullscreen / windowed
// upscale). Configurable via WinScaler.ini (MaxUpscale).
// See LoadConfig().
// --------------------------------------------------------

constexpr int DEFAULT_MAX_UPSCALE = 0;

// --------------------------------------------------------
// Game scaling modes inside the panel.
//
// Applied in fake fullscreen and in windowed upscale
// (WindowedUpscale=1). Configurable via WinScaler.ini
// (ScaleMode). See LoadConfig() / GetScaledRectangle().
//
// LetterBox : INTEGER scale (x1, x2, x3...), the largest
//             that fits, centered, black bars.
//             Pixel-perfect, no distortion.
// Stretch   : fills the whole panel, stretches the image,
//             ignores aspect ratio (distortion).
// Fit       : FLOATING-POINT scale, keeps aspect ratio,
//             fills as much as possible, minimal bars.
// --------------------------------------------------------

enum class ScaleMode
{
    LetterBox,
    Stretch,
    Fit
};

// --------------------------------------------------------
// Configuration file
//
// Placed next to the exe. See LoadConfig().
// --------------------------------------------------------

constexpr wchar_t CONFIG_FILE_NAME[] = L"WinScaler.ini";

constexpr UINT_PTR TIMER_ID = 1;
constexpr UINT TIMER_INTERVAL = 50;

constexpr int HOTKEY_ID = 1;

constexpr UINT WM_ECSCALER_REFROCUS =
WM_APP + 10;

// ============================================================
// Global state
// ============================================================

struct AppState
{
    HWND hostWnd = nullptr;
    HWND panelWnd = nullptr;
    HWND gameWnd = nullptr;

    HANDLE launcherProcess = nullptr;
    DWORD launcherPid = 0;

    HANDLE rubyProcess = nullptr;
    DWORD rubyPid = 0;

    HANDLE jobHandle = nullptr;

    // --------------------------------------------------------
    // Configuration (WinScaler.ini)
    // --------------------------------------------------------

    bool enableWindowedUpscale = false;

    std::wstring gameExeName = L"game.exe";

    int maxUpscale = DEFAULT_MAX_UPSCALE;

    ScaleMode scaleMode = ScaleMode::LetterBox;

    // --------------------------------------------------------
    // Host title / icon synced to the game window
    // --------------------------------------------------------

    std::wstring launchedExePath;

    HICON hostIconBig = nullptr;
    HICON hostIconSmall = nullptr;

    bool hostIconBigOwned = false;
    bool hostIconSmallOwned = false;

    bool hostIconLoaded = false;

    // --------------------------------------------------------
    // Forced native resolution (WinScaler.ini)
    //
    // 0 = auto-detected via GetClientSize (default).
    // Otherwise, overrides auto-detection: useful because the
    // game window belongs to another process, which may not be
    // DPI-aware itself. Its size, as read from our (DPI-aware)
    // process, can then be virtualized by Windows and therefore
    // slightly wrong, which then throws off the LetterBox
    // integer scale calculation.
    // --------------------------------------------------------

    int configuredNativeWidth = 0;
    int configuredNativeHeight = 0;

    // --------------------------------------------------------
    // Fake fullscreen mode
    // --------------------------------------------------------

    bool isFakeFullscreen = false;

    // --------------------------------------------------------
    // Layout protection
    // --------------------------------------------------------

    bool layoutInProgress = false;
    bool hostResizeInProgress = false;
    bool cleanupDone = false;

    bool focusRestorePending = false;

    // --------------------------------------------------------
    // Native resolution actually used by the game
    // --------------------------------------------------------

    int gameWidth = BASE_WIDTH;
    int gameHeight = BASE_HEIGHT;

    double gameAspectRatio =
        static_cast<double>(BASE_WIDTH) /
        static_cast<double>(BASE_HEIGHT);

    // --------------------------------------------------------
    // Resolution currently imposed on the Ruby window
    // --------------------------------------------------------

    int expectedGameWidth = BASE_WIDTH;
    int expectedGameHeight = BASE_HEIGHT;

    // --------------------------------------------------------
    // Scale
    //
    // Used ONLY in fake fullscreen.
    // --------------------------------------------------------

    int gameScale = 1;

    // --------------------------------------------------------
    // Last game geometry
    // --------------------------------------------------------

    int lastGameX = -1;
    int lastGameY = -1;

    int lastGameWidth = 0;
    int lastGameHeight = 0;

    // --------------------------------------------------------
    // Last panel size
    // --------------------------------------------------------

    int lastPanelWidth = 0;
    int lastPanelHeight = 0;

    // --------------------------------------------------------
    // Last detected resolution
    // --------------------------------------------------------

    int lastDetectedGameWidth = 0;
    int lastDetectedGameHeight = 0;

    // --------------------------------------------------------
    // Host window state
    // --------------------------------------------------------

    LONG savedStyle = 0;

    WINDOWPLACEMENT savedPlacement =
    {
        sizeof(WINDOWPLACEMENT)
    };
};

extern AppState g;
