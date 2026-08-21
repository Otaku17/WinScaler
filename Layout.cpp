#include "Layout.h"
#include "Utils.h"
#include "Focus.h"

#include <algorithm>
#include <cmath>

// ============================================================
// Résolution native du jeu
// ============================================================

bool SetGameResolution(
    int width,
    int height)
{
    // --------------------------------------------------------
    // Résolution native forcée (NativeWidth/NativeHeight dans
    // WinScaler.ini) : remplace toute valeur détectée, qui peut
    // être faussée par la virtualisation DPI cross-process si
    // le jeu lui-même n'est pas DPI-aware.
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
// Calcule le rectangle de mise à l'échelle du jeu
//
// Utilisé en faux plein écran, et aussi en fenêtré/maximisé
// si enableWindowedUpscale est actif (WinScaler.ini), avec le
// ScaleMode configuré.
//
// IMPORTANT :
// La résolution native du jeu reste gameW x gameH.
// Seule la taille de sa fenêtre est agrandie.
//
// Voir ScaleMode pour le détail des modes. Le jeu est
// toujours centré dans le panel (le panel, en fond noir,
// occupe déjà tout l'espace disponible).
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
    // Stretch : remplit tout le panel exactement, sans jamais
    // dépasser ses limites, déforme si besoin.
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
    // Fit : scale FLOTTANT, garde le ratio d'aspect, remplit
    // au maximum sans dépasser le panel (bandes minimisées
    // mais possibles).
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
    // LetterBox (par défaut) : scale ENTIER uniquement
    // (x1, x2, x3...) : le plus grand multiple entier qui
    // tient dans le panel est utilisé, afin de ne jamais
    // déformer l'image. Le jeu est centré, avec letterbox
    // (bandes noires) si nécessaire.
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
    // FAUX PLEIN ÉCRAN, ou fenêtré/maximisé avec
    // enableWindowedUpscale actif (WinScaler.ini) : le jeu est
    // mis à l'échelle selon ScaleMode. LetterBox et Fit gardent
    // leurs bandes (pas de déformation forcée) ; Stretch
    // remplit tout l'espace. Le plein écran ne force donc PAS
    // Stretch quand l'utilisateur a choisi LetterBox ou Fit.
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
    // Fenêtré/maximisé par défaut : résolution native, jeu
    // simplement centré, quel que soit ScaleMode (non utilisé
    // ici).
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
    // Résolution native forcée (WinScaler.ini) : on ne fait
    // jamais confiance à une lecture live potentiellement
    // faussée par la virtualisation DPI cross-process.
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
    // Avec enableWindowedUpscale, la taille de la fenêtre
    // host est pilotée par l'utilisateur (resize manuel ou
    // maximize), pas par la résolution native du jeu.
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
