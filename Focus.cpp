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
    // On ne vole pas le focus à une autre application.
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
    // La fenêtre Ruby appartient à un autre thread/processus.
    //
    // AttachThreadInput permet au thread du host de manipuler
    // correctement le focus clavier de la fenêtre Ruby.
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
    // Le host reste la fenêtre active de premier niveau.
    // Ruby est la fenêtre qui reçoit le focus clavier.
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
    // Vérification supplémentaire.
    //
    // Avec certaines applications Ruby/RGSS, le premier
    // SetFocus peut être perdu pendant le changement
    // de foreground.
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
// Forward clavier
//
// Si le host reçoit exceptionnellement un message clavier
// alors que Ruby devrait avoir le focus, on retransmet le
// message directement à la fenêtre du jeu.
//
// Cela ne modifie PAS le comportement graphique.
// ============================================================

bool ForwardKeyboardMessageToGame(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (!IsGameWindowValid())
        return false;

    // --------------------------------------------------------
    // Si Ruby possède déjà le focus, aucune retransmission
    // n'est nécessaire.
    // --------------------------------------------------------

    HWND foreground =
        GetForegroundWindow();

    if (foreground != g.hostWnd &&
        !IsChild(g.hostWnd, foreground))
    {
        return false;
    }

    // --------------------------------------------------------
    // Si le focus de notre thread est déjà Ruby, laisser le
    // système traiter normalement.
    // --------------------------------------------------------

    HWND focus =
        GetFocus();

    if (focus == g.gameWnd)
        return false;

    // --------------------------------------------------------
    // On s'assure d'abord que Ruby est bien la cible.
    // --------------------------------------------------------

    RequestFocusGame();

    // --------------------------------------------------------
    // Retransmission du message clavier.
    //
    // SendMessage est utilisé volontairement ici : la fenêtre
    // Ruby appartient à un autre thread et doit recevoir le
    // message immédiatement.
    // --------------------------------------------------------

    SendMessageW(
        g.gameWnd,
        msg,
        wParam,
        lParam);

    return true;
}
