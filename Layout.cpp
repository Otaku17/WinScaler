#include "Layout.h"
#include "Utils.h"
#include "Focus.h"

#include <algorithm>
#include <cmath>

// ============================================================
// Native game resolution
// ============================================================

bool SetGameResolution(
    int width,
    int height)
{
    // --------------------------------------------------------
    // Forced native resolution (NativeWidth/NativeHeight in
    // WinScaler.ini): overrides any detected value, which can
    // be thrown off by cross-process DPI virtualization if the
    // game itself isn't DPI-aware.
    // --------------------------------------------------------

    if (g.configuredNativeWidth > 0 &&
        g.configuredNativeHeight > 0)
    {
        width = g.configuredNativeWidth;
        height = g.configuredNativeHeight;
    }

    if (width < 160 ||
        height < 120)
    {
        return false;
    }

    bool changed =
        width != g.gameWidth ||
        height != g.gameHeight;

    g.gameWidth = width;
    g.gameHeight = height;

    g.gameAspectRatio =
        GetAspectRatio(
            width,
            height);

    g.lastDetectedGameWidth =
        width;

    g.lastDetectedGameHeight =
        height;

    return changed;
}

// ============================================================
// Computes the game's scaled rectangle
//
// Used in fake fullscreen, and also in windowed/maximized mode
// if enableWindowedUpscale is active (WinScaler.ini), using the
// configured ScaleMode.
//
// IMPORTANT:
// The game's native resolution stays gameW x gameH.
// Only its window's size is enlarged.
//
// See ScaleMode for details on each mode. The game is always
// centered in the panel (the panel, in black, already covers
// all the available space).
// ============================================================

static void GetScaledRectangle(
    int panelW,
    int panelH,
    int gameW,
    int gameH,
    int maxUpscale,
    ScaleMode mode,
    int& outX,
    int& outY,
    int& outW,
    int& outH)
{
    outX = 0;
    outY = 0;
    outW = gameW;
    outH = gameH;

    if (panelW <= 0 ||
        panelH <= 0 ||
        gameW <= 0 ||
        gameH <= 0)
    {
        return;
    }

    // --------------------------------------------------------
    // Stretch: fills the whole panel exactly, never exceeding
    // its bounds, distorts the image if needed.
    // --------------------------------------------------------

    if (mode == ScaleMode::Stretch)
    {
        outW = panelW;
        outH = panelH;

        outX = 0;
        outY = 0;

        return;
    }

    // --------------------------------------------------------
    // Fit: FLOATING-POINT scale, keeps aspect ratio, fills as
    // much as possible without exceeding the panel (minimal
    // bars, but possible).
    // --------------------------------------------------------

    if (mode == ScaleMode::Fit)
    {
        double scaleX =
            static_cast<double>(panelW) /
            static_cast<double>(gameW);

        double scaleY =
            static_cast<double>(panelH) /
            static_cast<double>(gameH);

        double scale =
            std::min(scaleX, scaleY);

        if (scale < 0.01)
            scale = 0.01;

        if (maxUpscale > 0 &&
            scale > static_cast<double>(maxUpscale))
        {
            scale =
                static_cast<double>(maxUpscale);
        }

        outW =
            static_cast<int>(
                std::lround(gameW * scale));

        outH =
            static_cast<int>(
                std::lround(gameH * scale));

        outX =
            (panelW - outW) / 2;

        outY =
            (panelH - outH) / 2;

        return;
    }

    // --------------------------------------------------------
    // LetterBox (default): INTEGER scale only (x1, x2, x3...):
    // the largest integer multiple that fits in the panel is
    // used, so the image is never distorted. The game is
    // centered, with letterbox (black bars) if needed.
    // --------------------------------------------------------

    int scaleX =
        panelW / gameW;

    int scaleY =
        panelH / gameH;

    int scale =
        std::min(
            scaleX,
            scaleY);

    if (scale < 1)
        scale = 1;

    if (maxUpscale > 0 &&
        scale > maxUpscale)
    {
        scale = maxUpscale;
    }

    outW = gameW * scale;
    outH = gameH * scale;

    outX =
        (panelW - outW) / 2;

    outY =
        (panelH - outH) / 2;
}

void LayoutGame()
{
    if (!g.gameWnd ||
        !IsWindow(g.gameWnd) ||
        !g.panelWnd ||
        !IsWindow(g.panelWnd) ||
        g.layoutInProgress)
    {
        return;
    }

    RECT rc = {};

    if (!GetClientRect(
        g.panelWnd,
        &rc))
    {
        return;
    }

    int panelW =
        rc.right - rc.left;

    int panelH =
        rc.bottom - rc.top;

    if (panelW <= 0 ||
        panelH <= 0)
    {
        return;
    }

    if (g.gameWidth <= 0 ||
        g.gameHeight <= 0)
    {
        return;
    }

    g.layoutInProgress = true;

    int targetX = 0;
    int targetY = 0;
    int targetW = g.gameWidth;
    int targetH = g.gameHeight;

    // --------------------------------------------------------
    // FAKE FULLSCREEN, or windowed/maximized with
    // enableWindowedUpscale active (WinScaler.ini): the game is
    // scaled according to ScaleMode. LetterBox and Fit keep
    // their bars (no forced distortion); Stretch fills the
    // whole space. Fullscreen therefore does NOT force Stretch
    // when the user chose LetterBox or Fit.
    // --------------------------------------------------------

    if (g.isFakeFullscreen ||
        g.enableWindowedUpscale)
    {
        GetScaledRectangle(
            panelW,
            panelH,
            g.gameWidth,
            g.gameHeight,
            g.maxUpscale,
            g.scaleMode,
            targetX,
            targetY,
            targetW,
            targetH);
    }

    // --------------------------------------------------------
    // Default windowed/maximized: native resolution, game
    // simply centered, regardless of ScaleMode (not used
    // here).
    // --------------------------------------------------------

    else
    {
        targetW = g.gameWidth;
        targetH = g.gameHeight;

        targetX =
            (panelW - targetW) / 2;

        targetY =
            (panelH - targetH) / 2;
    }

    if (targetX == g.lastGameX &&
        targetY == g.lastGameY &&
        targetW == g.lastGameWidth &&
        targetH == g.lastGameHeight &&
        panelW == g.lastPanelWidth &&
        panelH == g.lastPanelHeight)
    {
        g.layoutInProgress = false;
        return;
    }

    HWND foreground =
        GetForegroundWindow();

    bool preserveFocus =
        foreground == g.hostWnd ||
        IsChild(g.hostWnd, foreground);

    g.expectedGameWidth =
        targetW;

    g.expectedGameHeight =
        targetH;

    MoveWindow(
        g.gameWnd,
        targetX,
        targetY,
        targetW,
        targetH,
        TRUE);

    g.lastGameX = targetX;
    g.lastGameY = targetY;

    g.lastGameWidth = targetW;
    g.lastGameHeight = targetH;

    g.lastPanelWidth = panelW;
    g.lastPanelHeight = panelH;

    g.layoutInProgress = false;

    if (preserveFocus)
        RequestFocusGame();
}

void ResizeHostToClient(
    HWND hwnd,
    int clientW,
    int clientH)
{
    if (!hwnd ||
        !IsWindow(hwnd) ||
        clientW <= 0 ||
        clientH <= 0)
    {
        return;
    }

    DWORD style =
        static_cast<DWORD>(
            GetWindowLongW(
                hwnd,
                GWL_STYLE));

    DWORD exStyle =
        static_cast<DWORD>(
            GetWindowLongW(
                hwnd,
                GWL_EXSTYLE));

    RECT rc =
    {
        0,
        0,
        clientW,
        clientH
    };

    AdjustWindowRectEx(
        &rc,
        style,
        FALSE,
        exStyle);

    int windowW =
        rc.right - rc.left;

    int windowH =
        rc.bottom - rc.top;

    RECT current = {};

    if (!GetWindowRect(
        hwnd,
        &current))
    {
        return;
    }

    g.hostResizeInProgress = true;

    SetWindowPos(
        hwnd,
        nullptr,
        current.left,
        current.top,
        windowW,
        windowH,
        SWP_NOZORDER |
        SWP_NOACTIVATE |
        SWP_FRAMECHANGED);

    g.hostResizeInProgress = false;
}

void PollGameResolution()
{
    // --------------------------------------------------------
    // Forced native resolution (WinScaler.ini): never trust a
    // live reading that could be thrown off by cross-process
    // DPI virtualization.
    // --------------------------------------------------------

    if (g.configuredNativeWidth > 0 &&
        g.configuredNativeHeight > 0)
    {
        return;
    }

    if (!g.gameWnd ||
        !IsWindow(g.gameWnd))
    {
        return;
    }

    int width = 0;
    int height = 0;

    if (!GetClientSize(
        g.gameWnd,
        width,
        height))
    {
        return;
    }

    if (width < 160 ||
        height < 120)
    {
        return;
    }

    if (width == g.expectedGameWidth &&
        height == g.expectedGameHeight)
    {
        return;
    }

    if (!g.isFakeFullscreen &&
        width == g.gameWidth &&
        height == g.gameHeight)
    {
        return;
    }

    if (width == g.lastDetectedGameWidth &&
        height == g.lastDetectedGameHeight)
    {
        return;
    }

    g.lastDetectedGameWidth =
        width;

    g.lastDetectedGameHeight =
        height;

    bool changed =
        SetGameResolution(
            width,
            height);

    if (!changed)
        return;

    // ----------------------------------------------------
    // With enableWindowedUpscale, the host window's size is
    // driven by the user (manual resize or maximize), not by
    // the game's native resolution.
    // ----------------------------------------------------

    if (!g.enableWindowedUpscale &&
        !g.isFakeFullscreen &&
        !IsZoomed(g.hostWnd))
    {
        ResizeHostToClient(
            g.hostWnd,
            width,
            height);
    }

    g.lastGameX = -1;
    g.lastGameY = -1;
    g.lastGameWidth = 0;
    g.lastGameHeight = 0;

    LayoutGame();

    RequestFocusGame();
}
