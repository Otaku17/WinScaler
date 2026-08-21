#include "HostIdentity.h"
#include "Focus.h"

#include <shellapi.h>
#include <cwchar>

#pragma comment(lib, "shell32.lib")

void SyncHostTitle()
{
    if (!IsHostWindowValid() ||
        !IsGameWindowValid())
    {
        return;
    }

    wchar_t gameTitle[256] = {};

    GetWindowTextW(
        g.gameWnd,
        gameTitle,
        256);

    if (gameTitle[0] == L'\0')
        return;

    wchar_t hostTitle[256] = {};

    GetWindowTextW(
        g.hostWnd,
        hostTitle,
        256);

    if (wcscmp(
        gameTitle,
        hostTitle) != 0)
    {
        SetWindowTextW(
            g.hostWnd,
            gameTitle);
    }
}

void ResetHostIcon()
{
    g.hostIconLoaded = false;
}

void SyncHostIcon()
{
    if (g.hostIconLoaded)
        return;

    if (!IsHostWindowValid() ||
        !IsGameWindowValid())
    {
        return;
    }

    g.hostIconLoaded = true;

    // ----------------------------------------------------
    // Icons borrowed from the game window/class: they belong
    // to the other process, we must never destroy them.
    // ----------------------------------------------------

    HICON iconBig =
        reinterpret_cast<HICON>(
            SendMessageW(
                g.gameWnd,
                WM_GETICON,
                ICON_BIG,
                0));

    if (!iconBig)
    {
        iconBig =
            reinterpret_cast<HICON>(
                GetClassLongPtrW(
                    g.gameWnd,
                    GCLP_HICON));
    }

    HICON iconSmall =
        reinterpret_cast<HICON>(
            SendMessageW(
                g.gameWnd,
                WM_GETICON,
                ICON_SMALL,
                0));

    if (!iconSmall)
    {
        iconSmall =
            reinterpret_cast<HICON>(
                GetClassLongPtrW(
                    g.gameWnd,
                    GCLP_HICONSM));
    }

    bool bigOwned = false;
    bool smallOwned = false;

    // ----------------------------------------------------
    // Fallback: icon extracted from the launched executable.
    // This one belongs to us (ExtractIconExW), we must destroy
    // it ourselves.
    // ----------------------------------------------------

    if ((!iconBig ||
        !iconSmall) &&
        !g.launchedExePath.empty())
    {
        HICON extractedBig = nullptr;
        HICON extractedSmall = nullptr;

        UINT extracted =
            ExtractIconExW(
                g.launchedExePath.c_str(),
                0,
                &extractedBig,
                &extractedSmall,
                1);

        if (extracted > 0)
        {
            if (!iconBig)
            {
                iconBig = extractedBig;
                bigOwned = true;
            }
            else if (extractedBig)
            {
                DestroyIcon(extractedBig);
            }

            if (!iconSmall)
            {
                iconSmall = extractedSmall;
                smallOwned = true;
            }
            else if (extractedSmall)
            {
                DestroyIcon(extractedSmall);
            }
        }
    }

    if (!iconBig &&
        !iconSmall)
    {
        return;
    }

    if (g.hostIconBig &&
        g.hostIconBigOwned)
    {
        DestroyIcon(g.hostIconBig);
    }

    if (g.hostIconSmall &&
        g.hostIconSmallOwned)
    {
        DestroyIcon(g.hostIconSmall);
    }

    g.hostIconBig = iconBig;
    g.hostIconBigOwned = bigOwned;

    g.hostIconSmall = iconSmall;
    g.hostIconSmallOwned = smallOwned;

    if (g.hostIconBig)
    {
        SendMessageW(
            g.hostWnd,
            WM_SETICON,
            ICON_BIG,
            reinterpret_cast<LPARAM>(
                g.hostIconBig));
    }

    if (g.hostIconSmall)
    {
        SendMessageW(
            g.hostWnd,
            WM_SETICON,
            ICON_SMALL,
            reinterpret_cast<LPARAM>(
                g.hostIconSmall));
    }
}
