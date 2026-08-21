// ============================================================
// WinScaler - External Window Host
// Dynamic Resolution / Integer Upscale / Fake Fullscreen
// Windows Desktop Application (Visual Studio)
//
// MODES:
//
// 1. Normal windowed
//    - The Windows window is exactly the game's resolution.
//    - No upscale.
//    - Example: Ruby 640x480 -> 640x480 client window.
//
// 2. Full / maximized windowed
//    - The host window can occupy the whole screen.
//    - The game stays at its native resolution.
//    - The game is simply centered, regardless of ScaleMode
//      (not used in this mode).
//    - NO upscale.
//
//    Variant (WindowedUpscale=1 in WinScaler.ini, active by
//    default):
//    - The game is scaled according to ScaleMode
//      (LetterBox/Stretch/Fit), centered, dynamically
//      following the host window's size, without switching
//      to borderless fullscreen.
//
// 3. Fake fullscreen (Alt+Enter)
//    - The host window becomes borderless fullscreen.
//    - The game is scaled according to ScaleMode, same as in
//      windowed upscale. LetterBox and Fit keep their bars (no
//      forced distortion); Stretch fills the whole screen.
//
// ScaleMode (fake fullscreen and WindowedUpscale=1):
//    - LetterBox: integer scale, black bars, pixel-perfect.
//    - Stretch: fills all space, distorts the image.
//    - Fit (default): floating-point scale, keeps aspect
//      ratio, minimal bars.
//
// IMPORTANT:
// The Ruby window is no longer subclassed.
// The game belongs to another process.
// Resolution is monitored via polling.
//
// See the other files for the details of each module:
// AppState (global state / config), Utils, Config, Focus,
// Layout (scale / positioning), HostIdentity (title/icon),
// ProcessDiscovery (Ruby discovery / cleanup), WindowProcs
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
// Entry point
// ============================================================

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR /*lpCmdLine*/,
    _In_ int nCmdShow)
{
    // ========================================================
    // DPI awareness (before anything else)
    // ========================================================

    EnableDpiAwareness();

    // ========================================================
    // Configuration (WinScaler.ini)
    // ========================================================

    LoadConfig();

    // ========================================================
    // Game executable
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
    // Launching the game executable
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
    // Ruby discovery
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
    // Initial Ruby resolution
    // ========================================================

    int initialW = 0;
    int initialH = 0;

    GetClientSize(
        g.gameWnd,
        initialW,
        initialH);

    // Applies NativeWidth/NativeHeight (WinScaler.ini) if
    // configured, otherwise uses the detected size.

    SetGameResolution(
        initialW,
        initialH);

    g.expectedGameWidth =
        g.gameWidth;

    g.expectedGameHeight =
        g.gameHeight;

    // ========================================================
    // Preparing Ruby
    // ========================================================

    PrepareChildWindow(
        g.gameWnd);

    // ========================================================
    // Panel class
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
    // Host class
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
    // Initial window:
    //
    // EXACTLY the Ruby resolution.
    //
    // No upscale.
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
    // Center the window on the main screen (work area, outside
    // the taskbar).
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
    // Embedding
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
    // No subclassing.
    //
    // Monitoring is performed by
    // PollGameResolution().
    // ========================================================

    g.expectedGameWidth =
        g.gameWidth;

    g.expectedGameHeight =
        g.gameHeight;

    // ========================================================
    // Initial game size
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
    // Alt + Enter
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
    // Display
    // ========================================================

    ShowWindow(
        g.hostWnd,
        nCmdShow);

    UpdateWindow(
        g.hostWnd);

    // ========================================================
    // Initial layout
    // ========================================================

    LayoutGame();

    // ========================================================
    // Host title / icon, kept in sync with the game window
    // ========================================================

    SyncHostTitle();

    SyncHostIcon();

    // --------------------------------------------------------
    // INPUT:
    // explicitly give focus to the game after embedding it and
    // showing the host.
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
