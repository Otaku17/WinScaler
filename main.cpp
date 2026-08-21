// ============================================================
// WinScaler - External Window Host
// Dynamic Resolution / Integer Upscale / Fake Fullscreen
// Windows Desktop Application (Visual Studio)
//
// MODES :
//
// 1. Fenêtré normal
//    - La fenêtre Windows fait exactement la résolution du jeu.
//    - Aucun upscale.
//    - Exemple : Ruby 640x480 -> fenêtre client 640x480.
//
// 2. Fenêtré complet / maximisé
//    - La fenêtre host peut occuper tout l'écran.
//    - Le jeu reste à sa résolution native.
//    - Le jeu est simplement centré, quel que soit ScaleMode
//      (non utilisé dans ce mode).
//    - AUCUN upscale.
//
//    Variante (WindowedUpscale=1 dans WinScaler.ini, actif
//    par défaut) :
//    - Le jeu est mis à l'échelle selon ScaleMode
//      (LetterBox/Stretch/Fit), centré, en suivant
//      dynamiquement la taille de la fenêtre host, sans
//      passer en borderless fullscreen.
//
// 3. Faux plein écran (Alt+Entrée)
//    - La fenêtre host devient borderless fullscreen.
//    - Le jeu est mis à l'échelle selon ScaleMode, comme en
//      upscale fenêtré. LetterBox et Fit gardent leurs bandes
//      (pas de déformation forcée) ; Stretch remplit tout
//      l'écran.
//
// ScaleMode (faux plein écran et WindowedUpscale=1) :
//    - LetterBox : scale entier, bandes noires, pixel-parfait.
//    - Stretch : remplit tout l'espace, déforme l'image.
//    - Fit (défaut) : scale flottant, garde le ratio, bandes
//      minimisées.
//
// IMPORTANT :
// On ne subclass plus la fenêtre Ruby.
// Le jeu appartient à un autre processus.
// La résolution est surveillée par polling.
//
// Voir les autres fichiers pour le détail de chaque module :
// AppState (état global / config), Utils, Config, Focus,
// Layout (scale / positionnement), HostIdentity (titre/icône),
// ProcessDiscovery (recherche Ruby / cleanup), WindowProcs
// (PanelProc / HostProc), DpiAwareness.
// ============================================================

#include "AppState.h"
#include "Utils.h"
#include "Config.h"
#include "Focus.h"
#include "Layout.h"
#include "HostIdentity.h"
#include "ProcessDiscovery.h"
#include "WindowProcs.h"
#include "DpiAwareness.h"

#include <tlhelp32.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")

// ============================================================
// Point d'entrée
// ============================================================

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR /*lpCmdLine*/,
    _In_ int nCmdShow)
{
    // ========================================================
    // DPI awareness (avant toute autre chose)
    // ========================================================

    EnableDpiAwareness();

    // ========================================================
    // Configuration (WinScaler.ini)
    // ========================================================

    LoadConfig();

    // ========================================================
    // Exécutable du jeu
    // ========================================================

    std::wstring root =
        GetExecutableDir();

    std::wstring gamePath =
        IsAbsolutePathW(g.gameExeName)
            ? g.gameExeName
            : root + L"\\" + g.gameExeName;

    g.launchedExePath =
        gamePath;

    if (!FileExists(gamePath))
    {
        MessageBoxW(
            nullptr,
            (L"Executable introuvable :\n" +
                gamePath).c_str(),
            L"WinScaler",
            MB_ICONERROR | MB_OK);

        return 1;
    }

    SetEnvironmentVariableW(
        L"EXTERNAL_WINDOW_HOST",
        L"1");

    // ========================================================
    // Lancement de l'exécutable du jeu
    // ========================================================

    STARTUPINFOW si =
    {
        sizeof(si)
    };

    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine =
        L"\"" + gamePath + L"\"";

    if (!CreateProcessW(
        gamePath.c_str(),
        &cmdLine[0],
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        root.c_str(),
        &si,
        &pi))
    {
        MessageBoxW(
            nullptr,
            (L"Impossible de lancer :\n" +
                gamePath).c_str(),
            L"WinScaler",
            MB_ICONERROR | MB_OK);

        return 1;
    }

    g.launcherProcess =
        pi.hProcess;

    g.launcherPid =
        pi.dwProcessId;

    CloseHandle(
        pi.hThread);

    CreateSecurityJob();

    // ========================================================
    // Recherche Ruby
    // ========================================================

    if (!FindRubyProcess())
    {
        std::wstring debug =
            L"Impossible de trouver le Ruby associe a " +
            g.gameExeName +
            L".\n\n";

        debug +=
            g.gameExeName +
            L" PID : " +
            std::to_wstring(
                g.launcherPid) +
            L"\n\n";

        debug +=
            L"Processus Ruby detectes sur la machine :\n\n";

        HANDLE snap =
            CreateToolhelp32Snapshot(
                TH32CS_SNAPPROCESS,
                0);

        if (snap !=
            INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W pe =
            {
                sizeof(pe)
            };

            if (Process32FirstW(
                snap,
                &pe))
            {
                do
                {
                    if (
                        IsRubyProcess(
                            pe.szExeFile))
                    {
                        debug +=
                            L"PID " +
                            std::to_wstring(
                                pe.th32ProcessID);

                        debug +=
                            L"  Parent " +
                            std::to_wstring(
                                pe.th32ParentProcessID);

                        debug +=
                            L"  " +
                            std::wstring(
                                pe.szExeFile) +
                            L"\n";
                    }

                } while (
                    Process32NextW(
                        snap,
                        &pe));
            }

            CloseHandle(
                snap);
        }

        MessageBoxW(
            nullptr,
            debug.c_str(),
            L"WinScaler",
            MB_ICONERROR | MB_OK);

        Cleanup();

        return 1;
    }

    // ========================================================
    // Résolution initiale Ruby
    // ========================================================

    int initialW = 0;
    int initialH = 0;

    GetClientSize(
        g.gameWnd,
        initialW,
        initialH);

    // Applique NativeWidth/NativeHeight (WinScaler.ini) si
    // configuré, sinon utilise la taille détectée.

    SetGameResolution(
        initialW,
        initialH);

    g.expectedGameWidth =
        g.gameWidth;

    g.expectedGameHeight =
        g.gameHeight;

    // ========================================================
    // Préparer Ruby
    // ========================================================

    PrepareChildWindow(
        g.gameWnd);

    // ========================================================
    // Classe panel
    // ========================================================

    WNDCLASSEXW wcPanel =
    {
        sizeof(wcPanel)
    };

    wcPanel.lpfnWndProc =
        PanelProc;

    wcPanel.hInstance =
        hInstance;

    wcPanel.lpszClassName =
        L"WinScalerPanel";

    wcPanel.hbrBackground =
        static_cast<HBRUSH>(
            GetStockObject(
                BLACK_BRUSH));

    wcPanel.hCursor =
        LoadCursor(
            nullptr,
            IDC_ARROW);

    RegisterClassExW(
        &wcPanel);

    // ========================================================
    // Classe host
    // ========================================================

    WNDCLASSEXW wcHost =
    {
        sizeof(wcHost)
    };

    wcHost.lpfnWndProc =
        HostProc;

    wcHost.hInstance =
        hInstance;

    wcHost.lpszClassName =
        L"WinScalerHost";

    wcHost.hbrBackground =
        static_cast<HBRUSH>(
            GetStockObject(
                BLACK_BRUSH));

    wcHost.hCursor =
        LoadCursor(
            nullptr,
            IDC_ARROW);

    wcHost.hIcon =
        LoadIcon(
            nullptr,
            IDI_APPLICATION);

    RegisterClassExW(
        &wcHost);

    // ========================================================
    // Fenêtre initiale :
    //
    // EXACTEMENT la résolution Ruby.
    //
    // Aucun upscale.
    // ========================================================

    int initClientW =
        g.gameWidth;

    int initClientH =
        g.gameHeight;

    RECT adj =
    {
        0,
        0,
        initClientW,
        initClientH
    };

    AdjustWindowRectEx(
        &adj,
        WS_OVERLAPPEDWINDOW,
        FALSE,
        0);

    int initWindowW =
        adj.right - adj.left;

    int initWindowH =
        adj.bottom - adj.top;

    // --------------------------------------------------------
    // Centre la fenêtre sur l'écran principal (zone de travail,
    // hors barre des tâches).
    // --------------------------------------------------------

    RECT workArea = {};

    SystemParametersInfoW(
        SPI_GETWORKAREA,
        0,
        &workArea,
        0);

    int workW =
        workArea.right - workArea.left;

    int workH =
        workArea.bottom - workArea.top;

    int initWindowX =
        workArea.left +
        (workW - initWindowW) / 2;

    int initWindowY =
        workArea.top +
        (workH - initWindowH) / 2;

    if (initWindowX < workArea.left)
        initWindowX = workArea.left;

    if (initWindowY < workArea.top)
        initWindowY = workArea.top;

    g.hostWnd =
        CreateWindowExW(
            0,
            L"WinScalerHost",
            L"Edelweiss_Chronicles",
            WS_OVERLAPPEDWINDOW,
            initWindowX,
            initWindowY,
            initWindowW,
            initWindowH,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

    if (!g.hostWnd)
    {
        Cleanup();
        return 1;
    }

    if (!g.panelWnd)
    {
        MessageBoxW(
            nullptr,
            L"Panel host introuvable.",
            L"WinScaler",
            MB_ICONERROR | MB_OK);

        Cleanup();

        return 1;
    }

    // ========================================================
    // Encapsulation
    // ========================================================

    if (!SetParent(
        g.gameWnd,
        g.panelWnd))
    {
        DWORD err =
            GetLastError();

        std::wstring msg =
            L"Impossible d'encapsuler la fenetre du jeu.\n\n";

        msg +=
            L"GetLastError = " +
            std::to_wstring(
                err);

        MessageBoxW(
            nullptr,
            msg.c_str(),
            L"WinScaler",
            MB_ICONERROR | MB_OK);

        Cleanup();

        return 1;
    }

    // ========================================================
    // Aucun subclass.
    //
    // La surveillance est effectuée par
    // PollGameResolution().
    // ========================================================

    g.expectedGameWidth =
        g.gameWidth;

    g.expectedGameHeight =
        g.gameHeight;

    // ========================================================
    // Taille initiale du jeu
    // ========================================================

    g.layoutInProgress = true;

    MoveWindow(
        g.gameWnd,
        0,
        0,
        g.gameWidth,
        g.gameHeight,
        TRUE);

    g.layoutInProgress = false;

    g.lastGameX = -1;
    g.lastGameY = -1;
    g.lastGameWidth = 0;
    g.lastGameHeight = 0;

    g.lastPanelWidth = 0;
    g.lastPanelHeight = 0;

    // ========================================================
    // Alt + Entrée
    // ========================================================

    RegisterHotKey(
        g.hostWnd,
        HOTKEY_ID,
        MOD_ALT | MOD_NOREPEAT,
        VK_RETURN);

    // ========================================================
    // Timer
    // ========================================================

    SetTimer(
        g.hostWnd,
        TIMER_ID,
        TIMER_INTERVAL,
        nullptr);

    // ========================================================
    // Affichage
    // ========================================================

    ShowWindow(
        g.hostWnd,
        nCmdShow);

    UpdateWindow(
        g.hostWnd);

    // ========================================================
    // Layout initial
    // ========================================================

    LayoutGame();

    // ========================================================
    // Titre / icône host, alignés sur la fenêtre du jeu
    // ========================================================

    SyncHostTitle();

    SyncHostIcon();

    // --------------------------------------------------------
    // INPUT :
    // donner explicitement le focus au jeu après son
    // encapsulation et l'affichage du host.
    // --------------------------------------------------------

    RequestFocusGame();

    // ========================================================
    // Message loop
    // ========================================================

    MSG msg = {};

    while (
        GetMessage(
            &msg,
            nullptr,
            0,
            0))
    {
        TranslateMessage(
            &msg);

        DispatchMessage(
            &msg);
    }

    Cleanup();

    return static_cast<int>(
        msg.wParam);
}
