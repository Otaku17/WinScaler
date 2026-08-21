#pragma once

// ============================================================
// Etat global de l'application, types et constantes partagées.
//
// Inclus par tous les modules.
// ============================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <string>

// ============================================================
// Constantes
// ============================================================

constexpr int BASE_WIDTH = 320;
constexpr int BASE_HEIGHT = 240;

// --------------------------------------------------------
// Scale entier maximum par défaut (faux plein écran /
// upscale fenêtré). Configurable via WinScaler.ini
// (MaxUpscale). Voir LoadConfig().
// --------------------------------------------------------

constexpr int DEFAULT_MAX_UPSCALE = 0;

// --------------------------------------------------------
// Modes de mise à l'échelle du jeu dans le panel.
//
// Appliqué en faux plein écran et en upscale fenêtré
// (WindowedUpscale=1). Configurable via WinScaler.ini
// (ScaleMode). Voir LoadConfig() / GetScaledRectangle().
//
// LetterBox : scale ENTIER (x1, x2, x3...) le plus grand
//             qui tient, centré, avec bandes noires.
//             Pixel-parfait, aucune déformation.
// Stretch   : remplit tout le panel, étire l'image,
//             ignore le ratio d'aspect (déformation).
// Fit       : scale FLOTTANT, garde le ratio d'aspect,
//             remplit au maximum, bandes minimisées.
// --------------------------------------------------------

enum class ScaleMode
{
    LetterBox,
    Stretch,
    Fit
};

// --------------------------------------------------------
// Fichier de configuration
//
// Placé à côté de l'exe. Voir LoadConfig().
// --------------------------------------------------------

constexpr wchar_t CONFIG_FILE_NAME[] = L"WinScaler.ini";

constexpr UINT_PTR TIMER_ID = 1;
constexpr UINT TIMER_INTERVAL = 50;

constexpr int HOTKEY_ID = 1;

constexpr UINT WM_ECSCALER_REFROCUS =
WM_APP + 10;

// ============================================================
// Etat global
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
    // Titre / icône host synchronisés sur la fenêtre du jeu
    // --------------------------------------------------------

    std::wstring launchedExePath;

    HICON hostIconBig = nullptr;
    HICON hostIconSmall = nullptr;

    bool hostIconBigOwned = false;
    bool hostIconSmallOwned = false;

    bool hostIconLoaded = false;

    // --------------------------------------------------------
    // Résolution native forcée (WinScaler.ini)
    //
    // 0 = auto-détection via GetClientSize (par défaut).
    // Sinon, remplace la détection automatique : utile car la
    // fenêtre du jeu appartient à un autre processus, qui peut
    // ne pas être DPI-aware lui-même. Sa taille lue depuis notre
    // process (DPI-aware) peut alors être virtualisée par
    // Windows et donc légèrement fausse, ce qui fausse ensuite
    // le calcul du scale entier de LetterBox.
    // --------------------------------------------------------

    int configuredNativeWidth = 0;
    int configuredNativeHeight = 0;

    // --------------------------------------------------------
    // Mode faux plein écran
    // --------------------------------------------------------

    bool isFakeFullscreen = false;

    // --------------------------------------------------------
    // Protection layout
    // --------------------------------------------------------

    bool layoutInProgress = false;
    bool hostResizeInProgress = false;
    bool cleanupDone = false;

    bool focusRestorePending = false;

    // --------------------------------------------------------
    // Résolution native réellement utilisée par le jeu
    // --------------------------------------------------------

    int gameWidth = BASE_WIDTH;
    int gameHeight = BASE_HEIGHT;

    double gameAspectRatio =
        static_cast<double>(BASE_WIDTH) /
        static_cast<double>(BASE_HEIGHT);

    // --------------------------------------------------------
    // Résolution actuellement imposée à la fenêtre Ruby
    // --------------------------------------------------------

    int expectedGameWidth = BASE_WIDTH;
    int expectedGameHeight = BASE_HEIGHT;

    // --------------------------------------------------------
    // Scale
    //
    // Utilisé UNIQUEMENT en faux plein écran.
    // --------------------------------------------------------

    int gameScale = 1;

    // --------------------------------------------------------
    // Dernière géométrie du jeu
    // --------------------------------------------------------

    int lastGameX = -1;
    int lastGameY = -1;

    int lastGameWidth = 0;
    int lastGameHeight = 0;

    // --------------------------------------------------------
    // Dernière taille du panel
    // --------------------------------------------------------

    int lastPanelWidth = 0;
    int lastPanelHeight = 0;

    // --------------------------------------------------------
    // Dernière résolution détectée
    // --------------------------------------------------------

    int lastDetectedGameWidth = 0;
    int lastDetectedGameHeight = 0;

    // --------------------------------------------------------
    // Etat fenêtre host
    // --------------------------------------------------------

    LONG savedStyle = 0;

    WINDOWPLACEMENT savedPlacement =
    {
        sizeof(WINDOWPLACEMENT)
    };
};

extern AppState g;
