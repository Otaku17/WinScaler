#include "DpiAwareness.h"

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 \
    ((DPI_AWARENESS_CONTEXT)-4)
#endif

void EnableDpiAwareness()
{
    HMODULE user32 =
        GetModuleHandleW(L"user32.dll");

    if (user32)
    {
        using SetProcessDpiAwarenessContextFn =
            BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);

        auto setContext =
            reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                GetProcAddress(
                    user32,
                    "SetProcessDpiAwarenessContext"));

        if (setContext &&
            setContext(
                DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            return;
        }
    }

    // Repli pour les versions de Windows antérieures à la
    // prise en charge du per-monitor V2 (Windows 10 1703+).

    SetProcessDPIAware();
}
