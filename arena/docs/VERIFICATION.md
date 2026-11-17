# Verification record — 30 October 2026

## Verified here

* Windows x64 build with MSYS2 MinGW-w64 **GCC 15.2.0**, CMake **3.31.6**, Ninja **1.11.1.4**; official FFGL **v2.2** commit `fda8d4a5904eaf09dd97ac0f95c246bd7f9f9a46`; nlohmann/json **3.11.3** archive SHA-256 checked by CMake.
* Both Release DLLs compile and link. MinGW compiler runtimes are statically linked; import inspection shows Windows system DLLs rather than extra MinGW runtime DLL requirements.
* Real GPU context: **OpenGL 4.6.0, Intel Graphics, driver 32.0.101.5763**.
* Four CTest checks: native watchdog latch; real DirectWrite Unicode rasterization/transparent empty text; Source FFGL GPU host; Effect FFGL GPU host.
* The host tests load the actual DLL exports and run SDK initialization, instantiation, real shader compilation, OSC status/control, rendering to an FBO, pixel readback, a second instance, ID changes, resize, deinstantiation and module unload. They verify transparent source text, text-over-blue-video compositing, exact blackout pass-through, restoration of deliberately changed host GL state, whole-output flashes, timeout disarming, heartbeat not rearming, and duplicate-ID reporting/control rejection.
* Eight Python unittest checks, including actual native UDP receiver routing, absent-ID reporting, malformed-packet tolerance, stale status, preset Unicode round trips/disarming, invalid preset rejection, spectral band separation, envelope release, transient cooldown and native timeout behavior.
* PySide6 offscreen smoke check: UI construction, preview painting for all ten animation choices, Stop Strobe disarming and no fabricated Connected status. Offscreen Qt requires explicit font loading in the test; the normal Windows platform uses the Windows font database.
* Audio device enumeration through PyAudioWPatch: **6 input endpoints**, including **1 WASAPI loopback**.
* Python **3.12.10**, pinned PySide6 **6.8.3**, NumPy **2.2.4**, PyAudioWPatch **0.2.12.7**, PyInstaller **6.12.0**. Standalone desktop packaging completed; `--smoke-test` exercises packaged imports/widgets/painting without opening a visible window.
* Packaged `dist/PulseText/PulseText.exe --smoke-test` returned **exit 0 in 1.69 seconds** on the measured follow-up run. A diagnostic console build also returned **exit 0 in 14.44 seconds**, without stderr. Initial invocations exceeded 20-second (normal) and 30-second (diagnostic) startup limits; their cause was not established. Cold-start latency should be checked on the deployment machine.

## Still requires acceptance

Arena **7.21.1** is installed on this computer, but interactive Arena plugin scanning, clip playback, composition save/load and its parameter UI were **not exercised**. The minimum Arena **7.3.0** follows upstream compatibility documentation; that exact version has not been tested here. The automated host is an actual FFGL/OpenGL host, not Arena.

Physical microphone/line capture, real system-audio level response, unplug/reconnect behavior, audio-driver hangs, remote network/firewall operation, intended show resolution/performance, and long-duration live playback need testing on the intended machine. No sound recording or microphone capture was performed during development checks. The MSVC build recipe has not been run because MSVC is unavailable here. Follow `ARENA-CHECKLIST.md` before live use.

## Reproduce the verified build

With an MSYS2 MinGW64 GCC 15.2.0 toolchain, activate the Python environment (so `cmake` and `ninja` are on PATH), then:

```powershell
$env:PATH = 'C:/msys64/mingw64/bin;' + $env:PATH
.\scripts\get-sdk.ps1
cmake -S . -B build/mingw -G Ninja `
  -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/mingw --parallel 4
ctest --test-dir build/mingw --output-on-failure
$env:PULSETEXT_RECEIVER_TEST = (Resolve-Path build/mingw/native_receiver.exe).Path
python -m unittest discover -s tests -v
python tests/smoke_ui.py
python -m PyInstaller --clean --noconfirm PulseText.spec
```

The supplied `scripts/build-mingw.ps1` automates configure/build/native tests. DLLs are `build/mingw/PulseTextSource.dll` and `PulseTextEffect.dll`. Development logs remain in `.tools/build.log`, `.tools/package.log`, and `build/mingw/Testing/Temporary/LastTest.log`. The `.tools` Python installation was downloaded locally for verification and is not needed by the packaged application.
