# PulseText

Windows x64 desktop controller and two native Resolume FFGL plugins: **PulseText Source** generates transparent animated Unicode text; **PulseText Effect** composites it over one input video. No Wire dependency.

Minimum supported host: **Arena 7.3.0**, based on the official [FFGL 2.2 release compatibility statement](https://github.com/resolume/ffgl/releases/tag/v2.2). The SDK is pinned to commit `fda8d4a5904eaf09dd97ac0f95c246bd7f9f9a46`. Plugins use its real `CFFGLPlugin`, `InitGL`, `Resize`, `ProcessOpenGL`, `DeInitGL`, shader and screen-quad interfaces, checked against the [official source example](https://github.com/resolume/ffgl/blob/v2.2/source/plugins/Gradients/FFGLGradients.cpp). OpenGL 4.1 is required, matching that SDK's shaders. Compatibility at the minimum version still needs an Arena 7.3.0 acceptance run.

Built artifacts from this workspace: launch `dist/PulseText/PulseText.exe` (keep its entire folder together); install the two DLLs from `dist/FFGL` using the Arena steps below. The executable passed a packaged startup/painting/shutdown smoke test. See the verification record for the exact scope and remaining Arena checks.

## Run the desktop app

Install **Python 3.12 x64**, then run from this folder in PowerShell:

```powershell
py -3.12 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-lock.txt
.\.venv\Scripts\python.exe run.py
```

`requirements.txt` pins direct dependencies; `requirements-lock.txt` pins the complete tested Python environment, including packaging/build tools. Python 3.12.10 was used for local checks. The local `.tools` directory is development tooling, not a deployment dependency.

To build a standalone desktop folder:

```powershell
.\.venv\Scripts\python.exe -m PyInstaller --clean --noconfirm PulseText.spec
```

Distribute the **whole** `dist/PulseText` folder. Launch `PulseText.exe`; Python is not needed on the target computer. Include the license notices described in [THIRD_PARTY.md](THIRD_PARTY.md).

## Build the plugins with MSVC

Install Visual Studio 2022 Build Tools with **Desktop development with C++**, MSVC v143, Windows 10/11 SDK, and CMake 3.31.6. Git must be on PATH. Open Developer PowerShell:

```powershell
.\scripts\get-sdk.ps1
cmake -S . -B build/msvc -G "Visual Studio 17 2022" -A x64
cmake --build build/msvc --config Release --parallel
ctest --test-dir build/msvc -C Release --output-on-failure
```

Output: `build/msvc/Release/PulseTextSource.dll` and `PulseTextEffect.dll`. SDK acquisition is pinned; nlohmann/json 3.11.3 is downloaded by CMake with a SHA-256 check. No Qt dependency is required by the native DLLs. Windows DirectWrite, Direct2D and WIC rasterize real shaped text to an alpha mask; OpenGL uploads it only on the rendering thread and performs compositing, outline, glow, transformations and flashes.

The supplied `scripts/build-plugins.ps1` automates the same MSVC steps and optionally installs to `-PluginFolder 'C:/Users/you/Documents/PulseText/FFGL'`. MSVC build instructions are supplied but that compiler was unavailable during implementation. A MinGW build path and actual verification results are in [docs/VERIFICATION.md](docs/VERIFICATION.md).

## Install and use in Arena

1. Create a dedicated FFGL folder, for example `$HOME/Documents/PulseText/FFGL`.
2. Copy **both DLLs** into it. For a CMake build, run `cmake --install build/msvc --config Release --prefix "$HOME/Documents/PulseText/FFGL"`.
3. In Arena **Preferences → Video**, add this directory to the FFGL plugin search folders. Restart Arena. This follows Resolume's [documented extra FFGL folder workflow](https://resolume.com/blog/category/tools?page=1).
4. Drag **PulseText Source** from Sources into an empty clip slot, or add **PulseText Effect** from Effects to an existing video clip. Play the clip.
5. Set its **Control ID** numeric parameter to a unique ID. Default is 1. Its display shows the ID even though the underlying FFGL value is normalized.
6. Open PulseText. Use host `127.0.0.1`, port **12000 for Source** or **12001 for Effect**, and the matching ID. Click **Connect target**. Only an actual plugin status reply can show Connected.
7. Type text. Edits apply after 180 ms; continuous settings update at most 20 times/sec. **Apply** sends a full snapshot. **Restart animation** resets elapsed animation and BPM phase.

Choose a microphone/line input or a device marked **[System loopback]**, then **Start input**. Set audio mapping amounts above zero: their defaults are off. Bass/mid/treble bands, a level meter, attack/release and noise threshold are provided. Loopback devices are enumerated through [PyAudioWPatch's WASAPI support](https://github.com/s0d3s/PyAudioWPatch). Availability depends on the Windows endpoint/driver. Bass transients trigger flashes; this is **not accurate beat tracking**. BPM mode is a separate entered-BPM clock with subdivisions, synchronized by Restart.

Strobe starts **OFF**. The persistent red **Stop Strobe** button and **Escape** stop flashes while preserving text. **Blackout** makes the source transparent and the effect pass through original video. Connection loss latches strobe off after two seconds by default. A new enabled state snapshot rearms it; heartbeat alone does not. Presets always load disarmed. The Arena Strobe enable gate must also be open.

Shortcuts: Escape always stops strobe. Ctrl+Enter Apply, Ctrl+R Restart, and Ctrl+B Blackout work when focus is outside typing/editable controls, preventing performance actions while editing text, numbers or selectors. Buttons remain available while typing. Host/port/ID edits take effect with **Connect target**, which stops the previous target's strobe first.

Presets live in the Windows local application-data folder for PulseText and support JSON Save/Load/Import/Export. Three examples ship in `presets`. Preset file recall does not restore strobe or blackout. Arena saves the numeric parameters; desktop-controlled text/appearance must be reapplied from a preset after reopening a composition.

## Design and practical limits

Each module has one exclusive UDP socket and one receiver thread, shared by initialized GL instances. Prototype objects used by the SDK for metadata never register as live instances. Instance registration, ID routing and settings are mutex-protected. Rendering copies a snapshot, then releases its lock. Receiver lifetime ends with the last live instance, outside DLL unload. Port conflicts are explicit and never silently reuse or choose another port. See [OSC protocol and precedence](docs/OSC.md).

Text rasterization uses DirectWrite Unicode shaping, font fallback, multiline wrapping, style, spacing and alignment. Text masks are cached until font/text/layout/resolution changes; typewriter regenerates the mask when its visible character count changes. Animations use a monotonic clock. The GPU engine preserves the host GL state it touches, renders into the host framebuffer and checks current viewport size each frame. Output uses premultiplied alpha, including effect overlay and optional background.

The preview is functional but approximate: Qt text shaping and metrics can wrap differently from DirectWrite; its halo approximates the native shader's outline/glow. It represents a 1280×720 source, does not display Arena's input video or Arena parameter multipliers, and uses its own local clock. Missing installed font glyphs can show fallback boxes. Native emoji are rendered as monochrome glyph masks; typewriter respects UTF-16 surrogate pairs but can reveal combining sequences partway through a grapheme. Outline shares the text colour. Huge output sizes and rapid typewriter updates require CPU rasterization/upload and should be profiled on the show machine; dimensions above 8192 are rejected.

## Check the project

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
.\.venv\Scripts\python.exe tests/smoke_ui.py
```

For native integration tests in an MSVC build:

```powershell
$env:PULSETEXT_RECEIVER_TEST = (Resolve-Path build/msvc/Release/native_receiver.exe).Path
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
ctest --test-dir build/msvc -C Release --output-on-failure
```

The GL tests create a hidden Windows window and real OpenGL context. Run in an interactive desktop session with GPU drivers; Remote Desktop/software OpenGL may not support 4.1. They do not launch or modify Arena. Finish with the [manual Arena acceptance checklist](docs/ARENA-CHECKLIST.md) and [troubleshooting guide](docs/TROUBLESHOOTING.md).
