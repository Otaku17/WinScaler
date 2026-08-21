# WinScaler

**WinScaler** is an external window host for Ruby/RGSS games (RPG Maker XP/VX/VX Ace, PSDK, mkxp...). It wraps the game's window inside a "host" window that properly handles resizing, upscaling and fake fullscreen things the game engine itself often handles poorly, or not at all.

## Why this project exists

**PSDK** (Pokémon SDK) projects like most Ruby/RGSS games have very limited window management: fixed resolution, no clean real fullscreen, no screen-scale upscaling, no proper DPI handling on modern scaled displays. On a recent screen or a large monitor, the game stays tiny and centered, with no simple way to enlarge it properly.

WinScaler works around this from the outside: it launches the game, grabs its window, and re-wraps it inside its own display system (pixel-perfect integer upscale, proportional upscale, stretch, borderless fake fullscreen, DPI awareness...) without touching the game's code.

Although it was originally designed for a PSDK project, **WinScaler is not PSDK-specific**: it works with any game whose process is named `ruby.exe` or `rubyw.exe` (RPG Maker XP/VX/VX Ace, mkxp, mkxp-z, other RGSS engines). The executable to launch is configurable in `WinScaler.ini`.

## ⚠️ Antivirus detection (false positive)

Some antivirus software (notably **Windows Defender**, under the name `Trojan:Win32/Wacatac.C!ml`) may flag `WinScaler.exe` as suspicious. **This is a false positive.**

These detections are all based on heuristic **machine learning** (the `!ml` suffix at Microsoft, `ML.Attribute.HighConfidence` at Symantec, a partial confidence score at CrowdStrike), not on a known malware signature. WinScaler triggers these heuristics because it uses Windows APIs that, taken out of context, resemble techniques used by malware:

- `CreateToolhelp32Snapshot` + process enumeration to locate the launched game's window.
- `AttachThreadInput` to properly transfer keyboard focus to the game window (which belongs to another process).
- Cross-process `SetParent` to re-wrap the game window inside the host.
- Keyboard message forwarding (`SendMessage`) so the game reliably receives keystrokes when focus is ambiguous.

Combined with the fact that the executable is **unsigned** and has **no reputation** (few or no downloads known to antivirus vendors), this profile statistically resembles malware to an ML model even though there is **no** actual malicious technique in the code:

- No code injection (`WriteProcessMemory` / `CreateRemoteThread` / `VirtualAllocEx`: absent from the project).
- No networking (no socket / WinInet / WinHTTP calls).
- No persistence (no registry writes, no service, no scheduled task).
- No exfiltration: forwarded keystrokes are neither logged, nor stored, nor sent anywhere.

The full source code is available in this repository: anyone can verify these claims directly.

If your antivirus blocks the executable:
- **Windows Defender**: report the false positive at https://www.microsoft.com/en-us/wdsi/filesubmission
- **Symantec**: https://www.broadcom.com/support/security-center/submit-file
- Or build the executable yourself from source (see below) to remove any doubt.

## Features

- **Normal windowed mode**: window at the game's exact native resolution, no upscale.
- **Maximized windowed mode**: the game stays centered at its native resolution (default), or dynamically follows the host window's size according to `ScaleMode` (`WindowedUpscale=1`).
- **Fake fullscreen** (`Alt+Enter`): switches to borderless fullscreen, scaled according to `ScaleMode`.
- **3 scaling modes** (`ScaleMode`):
  - `LetterBox` integer scale (x1, x2, x3...), pixel-perfect, black bars.
  - `Fit` floating-point scale, keeps aspect ratio, fills as much as possible.
  - `Stretch` fills all available space, distorts the image if needed.
- **DPI-aware** (Per-Monitor V2) avoids blur and incorrect scale calculations on displays scaled by Windows (125%, 150%...).
- **Title and icon** automatically synced to the game window's.
- **Forceable native resolution** (`NativeWidth`/`NativeHeight`) if auto-detection is thrown off by cross-process DPI virtualization.

## Installation / usage

1. Place `WinScaler.exe` next to the game's executable (or configure a full path in `WinScaler.ini`).
2. Run `WinScaler.exe` once: a default `WinScaler.ini` is generated next to the exe.
3. Edit `WinScaler.ini` if needed (see below).
4. Launch `WinScaler.exe` instead of the game's executable.

### Configuration (`WinScaler.ini`)

| Key | Default | Description |
|---|---|---|
| `GameExe` | `game.exe` | Name or full path of the executable to launch. If relative, resolved from `WinScaler.exe`'s folder. |
| `WindowedUpscale` | `1` | `0` = windowed/maximized stays at native resolution, centered. `1` = dynamically follows `ScaleMode` based on the host window's size. |
| `MaxUpscale` | `0` | Maximum scale (fake fullscreen and windowed upscale). `0` = unlimited. |
| `ScaleMode` | `Fit` | `LetterBox`, `Stretch` or `Fit` (see Features). |
| `NativeWidth` / `NativeHeight` | `0` / `0` | Forces the game's native resolution instead of auto-detection. `0` = auto-detect. |

## Building

- Visual Studio 2022, toolset v143, C++20.
- Open `WinScaler.slnx`, select the `Release` / `x64` configuration, build.
- No external dependencies (only `user32`, `kernel32`, `gdi32`, `shell32`).

## Code structure

| File | Role |
|---|---|
| `AppState.h/.cpp` | Global state, `ScaleMode`, shared constants |
| `Utils.h/.cpp` | Paths, aspect ratio, window size |
| `Config.h/.cpp` | Reading / generating `WinScaler.ini` |
| `Focus.h/.cpp` | Cross-process keyboard focus |
| `Layout.h/.cpp` | Resolution, scale calculation, positioning |
| `HostIdentity.h/.cpp` | Host window title / icon sync |
| `ProcessDiscovery.h/.cpp` | Ruby process/window discovery, job object, cleanup |
| `WindowProcs.h/.cpp` | `PanelProc`, `HostProc`, fake fullscreen |
| `DpiAwareness.h/.cpp` | DPI-aware activation |
| `main.cpp` | Entry point (`wWinMain`) |

## Known limitations

- Only detects processes named `ruby.exe` or `rubyw.exe`.
- Executable is currently unsigned (see antivirus section above).
- The game's own internal rendering stays sharp or blurry depending on whether **its own** process is DPI-aware WinScaler cannot fix that from the outside.

## Contributors

| Name | Role |
|---|---|
| Ota | Lead Developer |
| Sukinae | Original Concept |
| Bugfix | Tester |
| Carine | Tester |
| Flo | Tester |
| Joewy | Tester |


## Version

Current version: **1.0.0**
