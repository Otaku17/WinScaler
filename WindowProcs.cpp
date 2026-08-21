#include "WindowProcs.h"
#include "Layout.h"
#include "Focus.h"
#include "HostIdentity.h"
#include "ProcessDiscovery.h"
#include "Utils.h"

// ============================================================
// Panel
// ============================================================

LRESULT CALLBACK PanelProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:

        if (!g.hostResizeInProgress)
        {
            g.lastGameX = -1;
            g.lastGameY = -1;
            g.lastGameWidth = 0;
            g.lastGameHeight = 0;

            g.lastPanelWidth = 0;
            g.lastPanelHeight = 0;

            LayoutGame();
        }

        return 0;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:

        RequestFocusGame();

        return 0;

    case WM_SETFOCUS:

        RequestFocusGame();

        return 0;

    case WM_ERASEBKGND:

        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        HDC hdc =
            BeginPaint(
                hwnd,
                &ps);

        RECT rc;

        GetClientRect(
            hwnd,
            &rc);

        FillRect(
            hdc,
            &rc,
            static_cast<HBRUSH>(
                GetStockObject(
                    BLACK_BRUSH)));

        EndPaint(
            hwnd,
            &ps);

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam);
}

// ============================================================
// Fake fullscreen
// ============================================================

static void ToggleFullscreen()
{
    if (!g.hostWnd ||
        !IsWindow(g.hostWnd))
    {
        return;
    }

    if (g.isFakeFullscreen)
    {
        // ====================================================
        // Back to windowed mode
        // ====================================================

        g.isFakeFullscreen = false;

        SetWindowLongW(
            g.hostWnd,
            GWL_STYLE,
            g.savedStyle);

        SetWindowPlacement(
            g.hostWnd,
            &g.savedPlacement);

        SetWindowPos(
            g.hostWnd,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOZORDER |
            SWP_FRAMECHANGED |
            SWP_SHOWWINDOW);

        if (!IsZoomed(g.hostWnd))
        {
            ResizeHostToClient(
                g.hostWnd,
                g.gameWidth,
                g.gameHeight);
        }

        g.expectedGameWidth =
            g.gameWidth;

        g.expectedGameHeight =
            g.gameHeight;

        g.lastGameX = -1;
        g.lastGameY = -1;
        g.lastGameWidth = 0;
        g.lastGameHeight = 0;

        g.lastPanelWidth = 0;
        g.lastPanelHeight = 0;

        LayoutGame();

        RequestFocusGame();

        return;
    }

    // ========================================================
    // Entering fake fullscreen
    // ========================================================

    GetWindowPlacement(
        g.hostWnd,
        &g.savedPlacement);

    g.savedStyle =
        GetWindowLongW(
            g.hostWnd,
            GWL_STYLE);

    SetWindowLongW(
        g.hostWnd,
        GWL_STYLE,
        WS_POPUP | WS_VISIBLE);

    HMONITOR mon =
        MonitorFromWindow(
            g.hostWnd,
            MONITOR_DEFAULTTONEAREST);

    MONITORINFO mi =
    {
        sizeof(mi)
    };

    if (!GetMonitorInfoW(
        mon,
        &mi))
    {
        SetWindowLongW(
            g.hostWnd,
            GWL_STYLE,
            g.savedStyle);

        return;
    }

    SetWindowPos(
        g.hostWnd,
        HWND_TOP,
        mi.rcMonitor.left,
        mi.rcMonitor.top,
        mi.rcMonitor.right -
        mi.rcMonitor.left,
        mi.rcMonitor.bottom -
        mi.rcMonitor.top,
        SWP_FRAMECHANGED |
        SWP_SHOWWINDOW);

    g.isFakeFullscreen = true;

    g.lastGameX = -1;
    g.lastGameY = -1;
    g.lastGameWidth = 0;
    g.lastGameHeight = 0;

    g.lastPanelWidth = 0;
    g.lastPanelHeight = 0;

    LayoutGame();

    RequestFocusGame();
}

// ============================================================
// Host Window Procedure
// ============================================================

LRESULT CALLBACK HostProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g.hostWnd = hwnd;

        g.panelWnd =
            CreateWindowExW(
                0,
                L"WinScalerPanel",
                nullptr,
                WS_CHILD |
                WS_VISIBLE |
                WS_CLIPSIBLINGS |
                WS_CLIPCHILDREN,
                0,
                0,
                0,
                0,
                hwnd,
                nullptr,
                GetModuleHandleW(nullptr),
                nullptr);

        return 0;
    }

    // ========================================================
    // KEYBOARD INPUT
    //
    // If, during a focus transition, the host directly receives
    // a key while Ruby should be active, it is forwarded to
    // Ruby.
    //
    // Alt+Enter is still handled by RegisterHotKey / WM_HOTKEY.
    // ========================================================

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:

        if (ForwardKeyboardMessageToGame(
            msg,
            wParam,
            lParam))
        {
            return 0;
        }

        break;

        // ========================================================
        // Host size
        // ========================================================

    case WM_SIZE:
    {
        if (g.panelWnd)
        {
            RECT rc = {};

            GetClientRect(
                hwnd,
                &rc);

            g.hostResizeInProgress = true;

            MoveWindow(
                g.panelWnd,
                0,
                0,
                rc.right,
                rc.bottom,
                TRUE);

            g.hostResizeInProgress = false;

            g.lastGameX = -1;
            g.lastGameY = -1;
            g.lastGameWidth = 0;
            g.lastGameHeight = 0;

            g.lastPanelWidth = 0;
            g.lastPanelHeight = 0;

            LayoutGame();

            // ------------------------------------------------
            // IMPORTANT INPUT:
            // a size change must not leave the keyboard on the
            // host.
            // ------------------------------------------------

            RequestFocusGame();
        }

        return 0;
    }

    // ========================================================
    // Focus / mouse
    // ========================================================

    case WM_MOUSEACTIVATE:

        RequestFocusGame();

        return MA_ACTIVATE;

    case WM_SETFOCUS:

        RequestFocusGame();

        return 0;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:

        RequestFocusGame();

        break;

    case WM_HOTKEY:

        if (wParam == HOTKEY_ID)
        {
            ToggleFullscreen();
            return 0;
        }

        return 0;

    case WM_ECSCALER_REFROCUS:

        if (g.focusRestorePending)
            FocusGame();

        return 0;

        // ========================================================
        // Timer
        // ========================================================

    case WM_TIMER:
    {
        if (wParam != TIMER_ID)
            return 0;

        // ----------------------------------------------------
        // Lost Ruby window
        // ----------------------------------------------------

        if (!g.gameWnd ||
            !IsWindow(g.gameWnd))
        {
            if (g.rubyPid != 0)
            {
                HWND found =
                    FindMainWindow(
                        g.rubyPid);

                if (found)
                {
                    PrepareChildWindow(
                        found);

                    if (SetParent(
                        found,
                        g.panelWnd))
                    {
                        g.gameWnd =
                            found;

                        ResetHostIcon();

                        int w = 0;
                        int h = 0;

                        if (GetClientSize(
                            found,
                            w,
                            h))
                        {
                            if (w >= 160 &&
                                h >= 120)
                            {
                                SetGameResolution(
                                    w,
                                    h);

                                g.expectedGameWidth =
                                    w;

                                g.expectedGameHeight =
                                    h;
                            }
                        }

                        g.lastGameX = -1;
                        g.lastGameY = -1;
                        g.lastGameWidth = 0;
                        g.lastGameHeight = 0;

                        g.lastPanelWidth = 0;
                        g.lastPanelHeight = 0;

                        LayoutGame();

                        RequestFocusGame();
                    }
                }
            }

            if (!g.gameWnd ||
                !IsWindow(g.gameWnd))
            {
                DestroyWindow(
                    hwnd);

                return 0;
            }
        }

        // ----------------------------------------------------
        // Possible new Ruby window
        // ----------------------------------------------------

        if (g.rubyPid != 0)
        {
            HWND better =
                FindMainWindow(
                    g.rubyPid);

            if (
                better &&
                better != g.gameWnd &&
                IsWindow(better))
            {
                int w = 0;
                int h = 0;

                if (
                    GetClientSize(
                        better,
                        w,
                        h) &&
                    w >= 160 &&
                    h >= 120)
                {
                    PrepareChildWindow(
                        better);

                    if (SetParent(
                        better,
                        g.panelWnd))
                    {
                        g.gameWnd =
                            better;

                        ResetHostIcon();

                        SetGameResolution(
                            w,
                            h);

                        g.expectedGameWidth =
                            w;

                        g.expectedGameHeight =
                            h;

                        g.lastGameX = -1;
                        g.lastGameY = -1;
                        g.lastGameWidth = 0;
                        g.lastGameHeight = 0;

                        g.lastPanelWidth = 0;
                        g.lastPanelHeight = 0;

                        LayoutGame();

                        RequestFocusGame();
                    }
                }
            }
        }

        // ----------------------------------------------------
        // Resolution monitoring
        // ----------------------------------------------------

        PollGameResolution();

        // ----------------------------------------------------
        // Safety layout
        // ----------------------------------------------------

        LayoutGame();

        // ----------------------------------------------------
        // Host title / icon, kept in sync with the game window
        // ----------------------------------------------------

        SyncHostTitle();

        SyncHostIcon();

        // ----------------------------------------------------
        // Focus
        //
        // If the host is still the active application but Ruby
        // has lost focus, restore it.
        // ----------------------------------------------------

        HWND foreground =
            GetForegroundWindow();

        if (
            foreground == g.hostWnd ||
            IsChild(
                g.hostWnd,
                foreground))
        {
            if (g.focusRestorePending)
                FocusGame();
        }

        return 0;
    }

    case WM_ACTIVATE:

        if (LOWORD(wParam) != WA_INACTIVE)
            RequestFocusGame();

        return 0;

    case WM_ACTIVATEAPP:

        if (wParam)
            RequestFocusGame();

        return 0;

    case WM_CLOSE:

        Cleanup();

        DestroyWindow(
            hwnd);

        return 0;

    case WM_DESTROY:

        Cleanup();

        PostQuitMessage(0);

        return 0;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam);
}
