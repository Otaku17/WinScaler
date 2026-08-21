#include "ProcessDiscovery.h"
#include "Utils.h"

#include <tlhelp32.h>
#include <set>
#include <cwchar>

void PrepareChildWindow(
    HWND hwnd)
{
    if (!hwnd ||
        !IsWindow(hwnd))
    {
        return;
    }

    LONG style =
        GetWindowLongW(
            hwnd,
            GWL_STYLE);

    style &= ~(
        WS_POPUP |
        WS_CAPTION |
        WS_THICKFRAME |
        WS_MINIMIZEBOX |
        WS_MAXIMIZEBOX |
        WS_SYSMENU |
        WS_BORDER |
        WS_DLGFRAME
        );

    style |=
        WS_CHILD |
        WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN;

    SetWindowLongW(
        hwnd,
        GWL_STYLE,
        style);

    LONG exStyle =
        GetWindowLongW(
            hwnd,
            GWL_EXSTYLE);

    exStyle &= ~(
        WS_EX_APPWINDOW |
        WS_EX_TOOLWINDOW |
        WS_EX_DLGMODALFRAME |
        WS_EX_WINDOWEDGE |
        WS_EX_CLIENTEDGE |
        WS_EX_STATICEDGE
        );

    SetWindowLongW(
        hwnd,
        GWL_EXSTYLE,
        exStyle);

    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_FRAMECHANGED);
}

struct EnumData
{
    DWORD pid;
    HWND best;
    int bestScore;
};

static BOOL CALLBACK EnumWindowsProc(
    HWND hwnd,
    LPARAM lParam)
{
    EnumData* data =
        reinterpret_cast<EnumData*>(
            lParam);

    DWORD pid = 0;

    GetWindowThreadProcessId(
        hwnd,
        &pid);

    if (pid != data->pid)
        return TRUE;

    if (GetWindow(
        hwnd,
        GW_OWNER) != nullptr)
    {
        return TRUE;
    }

    if (!IsWindowVisible(hwnd))
        return TRUE;

    RECT rc = {};

    GetClientRect(
        hwnd,
        &rc);

    int w =
        rc.right - rc.left;

    int h =
        rc.bottom - rc.top;

    if (w < 160 ||
        h < 120)
    {
        return TRUE;
    }

    wchar_t title[256] = {};

    GetWindowTextW(
        hwnd,
        title,
        256);

    int score =
        w * h;

    if (title[0] != L'\0')
        score += 1000000;

    if (score > data->bestScore)
    {
        data->bestScore = score;
        data->best = hwnd;
    }

    return TRUE;
}

HWND FindMainWindow(
    DWORD pid)
{
    EnumData data =
    {
        pid,
        nullptr,
        -1
    };

    EnumWindows(
        EnumWindowsProc,
        reinterpret_cast<LPARAM>(
            &data));

    return data.best;
}

bool IsRubyProcess(
    const wchar_t* exeName)
{
    if (!exeName)
        return false;

    return
        _wcsicmp(
            exeName,
            L"rubyw.exe") == 0 ||
        _wcsicmp(
            exeName,
            L"ruby.exe") == 0;
}

static std::set<DWORD> GetProcessTree(
    DWORD rootPid)
{
    std::set<DWORD> knownPids;

    knownPids.insert(
        rootPid);

    bool changed = true;

    while (changed)
    {
        changed = false;

        HANDLE snap =
            CreateToolhelp32Snapshot(
                TH32CS_SNAPPROCESS,
                0);

        if (snap ==
            INVALID_HANDLE_VALUE)
        {
            break;
        }

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
                    knownPids.count(
                        pe.th32ParentProcessID) &&
                    !knownPids.count(
                        pe.th32ProcessID))
                {
                    knownPids.insert(
                        pe.th32ProcessID);

                    changed = true;
                }

            } while (
                Process32NextW(
                    snap,
                    &pe));
        }

        CloseHandle(
            snap);
    }

    return knownPids;
}

bool FindRubyProcess()
{
    const int maxAttempts = 400;

    for (
        int attempt = 0;
        attempt < maxAttempts;
        ++attempt)
    {
        {
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
                                pe.szExeFile) &&
                            pe.th32ParentProcessID ==
                            g.launcherPid)
                        {
                            for (
                                int w = 0;
                                w < 300;
                                ++w)
                            {
                                HWND found =
                                    FindMainWindow(
                                        pe.th32ProcessID);

                                if (found)
                                {
                                    int cw = 0;
                                    int ch = 0;

                                    if (
                                        GetClientSize(
                                            found,
                                            cw,
                                            ch) &&
                                        cw >= 160 &&
                                        ch >= 120)
                                    {
                                        HANDLE hProc =
                                            OpenProcess(
                                                PROCESS_QUERY_INFORMATION |
                                                PROCESS_VM_READ |
                                                PROCESS_TERMINATE |
                                                SYNCHRONIZE,
                                                FALSE,
                                                pe.th32ProcessID);

                                        if (hProc)
                                        {
                                            g.rubyProcess =
                                                hProc;

                                            g.rubyPid =
                                                pe.th32ProcessID;

                                            g.gameWnd =
                                                found;

                                            CloseHandle(
                                                snap);

                                            return true;
                                        }
                                    }
                                }

                                Sleep(50);

                                MSG msg;

                                while (
                                    PeekMessage(
                                        &msg,
                                        nullptr,
                                        0,
                                        0,
                                        PM_REMOVE))
                                {
                                    TranslateMessage(
                                        &msg);

                                    DispatchMessage(
                                        &msg);
                                }
                            }
                        }

                    } while (
                        Process32NextW(
                            snap,
                            &pe));
                }

                CloseHandle(
                    snap);
            }
        }

        {
            std::set<DWORD> tree =
                GetProcessTree(
                    g.launcherPid);

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
                                pe.szExeFile) &&
                            tree.count(
                                pe.th32ProcessID))
                        {
                            for (
                                int w = 0;
                                w < 300;
                                ++w)
                            {
                                HWND found =
                                    FindMainWindow(
                                        pe.th32ProcessID);

                                if (found)
                                {
                                    int cw = 0;
                                    int ch = 0;

                                    if (
                                        GetClientSize(
                                            found,
                                            cw,
                                            ch) &&
                                        cw >= 160 &&
                                        ch >= 120)
                                    {
                                        HANDLE hProc =
                                            OpenProcess(
                                                PROCESS_QUERY_INFORMATION |
                                                PROCESS_VM_READ |
                                                PROCESS_TERMINATE |
                                                SYNCHRONIZE,
                                                FALSE,
                                                pe.th32ProcessID);

                                        if (hProc)
                                        {
                                            g.rubyProcess =
                                                hProc;

                                            g.rubyPid =
                                                pe.th32ProcessID;

                                            g.gameWnd =
                                                found;

                                            CloseHandle(
                                                snap);

                                            return true;
                                        }
                                    }
                                }

                                Sleep(50);

                                MSG msg;

                                while (
                                    PeekMessage(
                                        &msg,
                                        nullptr,
                                        0,
                                        0,
                                        PM_REMOVE))
                                {
                                    TranslateMessage(
                                        &msg);

                                    DispatchMessage(
                                        &msg);
                                }
                            }
                        }

                    } while (
                        Process32NextW(
                            snap,
                            &pe));
                }

                CloseHandle(
                    snap);
            }
        }

        if (
            WaitForSingleObject(
                g.launcherProcess,
                0) == WAIT_OBJECT_0)
        {
            break;
        }

        Sleep(100);

        MSG msg;

        while (
            PeekMessage(
                &msg,
                nullptr,
                0,
                0,
                PM_REMOVE))
        {
            TranslateMessage(
                &msg);

            DispatchMessage(
                &msg);
        }
    }

    return false;
}

bool CreateSecurityJob()
{
    g.jobHandle =
        CreateJobObjectW(
            nullptr,
            nullptr);

    if (!g.jobHandle)
        return false;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info =
    {};

    info.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (!SetInformationJobObject(
        g.jobHandle,
        JobObjectExtendedLimitInformation,
        &info,
        sizeof(info)))
    {
        CloseHandle(
            g.jobHandle);

        g.jobHandle = nullptr;

        return false;
    }

    if (!AssignProcessToJobObject(
        g.jobHandle,
        g.launcherProcess))
    {
        CloseHandle(
            g.jobHandle);

        g.jobHandle = nullptr;

        return false;
    }

    return true;
}

void Cleanup()
{
    if (g.cleanupDone)
        return;

    g.cleanupDone = true;

    if (g.hostWnd)
    {
        KillTimer(
            g.hostWnd,
            TIMER_ID);

        UnregisterHotKey(
            g.hostWnd,
            HOTKEY_ID);
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

    g.hostIconBig = nullptr;
    g.hostIconSmall = nullptr;

    if (g.rubyProcess)
    {
        if (
            WaitForSingleObject(
                g.rubyProcess,
                0) == WAIT_TIMEOUT)
        {
            TerminateProcess(
                g.rubyProcess,
                0);

            WaitForSingleObject(
                g.rubyProcess,
                3000);
        }

        CloseHandle(
            g.rubyProcess);

        g.rubyProcess = nullptr;
    }

    if (g.launcherProcess)
    {
        if (
            WaitForSingleObject(
                g.launcherProcess,
                0) == WAIT_TIMEOUT)
        {
            TerminateProcess(
                g.launcherProcess,
                0);

            WaitForSingleObject(
                g.launcherProcess,
                3000);
        }

        CloseHandle(
            g.launcherProcess);

        g.launcherProcess = nullptr;
    }

    if (g.jobHandle)
    {
        CloseHandle(
            g.jobHandle);

        g.jobHandle = nullptr;
    }
}
