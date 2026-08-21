#include "Focus.h"

bool IsGameWindowValid()
{
    return
        g.gameWnd &&
        IsWindow(g.gameWnd);
}

bool IsHostWindowValid()
{
    return
        g.hostWnd &&
        IsWindow(g.hostWnd);
}

static bool IsFocusInsideHost()
{
    if (!IsHostWindowValid())
        return false;

    HWND foreground =
        GetForegroundWindow();

    if (!foreground)
        return false;

    return
        foreground == g.hostWnd ||
        IsChild(
            g.hostWnd,
            foreground);
}

void FocusGame()
{
    if (!IsGameWindowValid() ||
        !IsHostWindowValid())
    {
        return;
    }

    // --------------------------------------------------------
    // Don't steal focus from another application.
    // --------------------------------------------------------

    HWND foreground =
        GetForegroundWindow();

    if (foreground != g.hostWnd &&
        !IsChild(g.hostWnd, foreground))
    {
        g.focusRestorePending = false;
        return;
    }

    DWORD hostThread =
        GetCurrentThreadId();

    DWORD gameThread =
        GetWindowThreadProcessId(
            g.gameWnd,
            nullptr);

    bool attached = false;

    // --------------------------------------------------------
    // The Ruby window belongs to another thread/process.
    //
    // AttachThreadInput lets the host's thread properly
    // manipulate the Ruby window's keyboard focus.
    // --------------------------------------------------------

    if (gameThread != 0 &&
        gameThread != hostThread)
    {
        attached =
            AttachThreadInput(
                hostThread,
                gameThread,
                TRUE) != FALSE;
    }

    // --------------------------------------------------------
    // The host stays the active top-level window.
    // Ruby is the window that receives keyboard focus.
    // --------------------------------------------------------

    BringWindowToTop(
        g.hostWnd);

    SetForegroundWindow(
        g.hostWnd);

    SetActiveWindow(
        g.hostWnd);

    SetFocus(
        g.gameWnd);

    // --------------------------------------------------------
    // Extra check.
    //
    // With some Ruby/RGSS applications, the first SetFocus can
    // get lost during the foreground change.
    // --------------------------------------------------------

    if (GetFocus() != g.gameWnd)
    {
        SetFocus(
            g.gameWnd);
    }

    if (attached)
    {
        AttachThreadInput(
            hostThread,
            gameThread,
            FALSE);
    }

    g.focusRestorePending = false;
}

void RequestFocusGame()
{
    if (!IsHostWindowValid() ||
        !IsGameWindowValid())
    {
        return;
    }

    g.focusRestorePending = true;

    PostMessageW(
        g.hostWnd,
        WM_ECSCALER_REFROCUS,
        0,
        0);
}

// ============================================================
// Keyboard forwarding
//
// If the host exceptionally receives a keyboard message while
// Ruby should have focus, the message is forwarded directly
// to the game window.
//
// This does NOT change graphical behavior.
// ============================================================

bool ForwardKeyboardMessageToGame(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (!IsGameWindowValid())
        return false;

    // --------------------------------------------------------
    // If Ruby already has focus, no forwarding is needed.
    // --------------------------------------------------------

    HWND foreground =
        GetForegroundWindow();

    if (foreground != g.hostWnd &&
        !IsChild(g.hostWnd, foreground))
    {
        return false;
    }

    // --------------------------------------------------------
    // If our thread's focus is already Ruby, let the system
    // handle it normally.
    // --------------------------------------------------------

    HWND focus =
        GetFocus();

    if (focus == g.gameWnd)
        return false;

    // --------------------------------------------------------
    // First make sure Ruby is indeed the target.
    // --------------------------------------------------------

    RequestFocusGame();

    // --------------------------------------------------------
    // Forward the keyboard message.
    //
    // SendMessage is deliberately used here: the Ruby window
    // belongs to another thread and must receive the message
    // immediately.
    // --------------------------------------------------------

    SendMessageW(
        g.gameWnd,
        msg,
        wParam,
        lParam);

    return true;
}
