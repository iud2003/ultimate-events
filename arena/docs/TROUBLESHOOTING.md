# Troubleshooting

**Plugin not listed:** ensure the DLLs are x64, directly inside a configured FFGL directory, and Arena was restarted after copying. Check Arena is at least 7.3.0 and the GPU supports OpenGL 4.1. Use Release builds. MSVC builds statically link the C++ runtime; MinGW builds statically link their compiler runtime. Do not mix old and new PulseText DLL copies in multiple search folders.

**No response:** start/initialize a PulseText clip, verify Source port 12000 vs Effect 12001, host/IP and Control ID. PulseText's addresses belong to its DLLs, not Arena's native OSC input. Check Windows Firewall for remote control, and `Get-NetUDPEndpoint -LocalPort 12000` for another listener. A port conflict emits a Windows debugger message and gives no status reply. Set the appropriate `PULSETEXT_*_PORT` environment variable and fully restart Arena. The controller's Connect target button commits destination edits.

**0 instances / duplicate instances:** status count 0 means the requested ID is not initialized on that module. Count >1 means duplicate IDs; the receiver intentionally rejects updates. Change IDs in Arena. FFGL normalized ID value is `(ID-1)/998`; the parameter display gives the actual integer.

**Connected but invisible:** Connected proves a registered instance answers UDP, not that it is playing or successfully drawing. Check clip playback, Blackout in app and Arena, opacity multiplier, scale, position, text colour/alpha, empty text and animation phase. Scroll/typewriter may initially be outside the frame. Click Restart. Inspect host logs for shader/DirectWrite failures. A full white frame can be Whole output strobe; press Escape.

**Fonts look different / missing characters:** install the font on the Arena machine, restart Arena, and use a font with the intended script coverage. DirectWrite uses Windows fallback; monochrome emoji are expected. The Qt preview is explicitly approximate and uses a 1280×720 layout. Compare at the real output size. Typewriter reveals Unicode scalar values rather than complete graphemes. Use Static for exact simultaneous shaping.

**Slow rendering:** static text masks are cached. Typewriter rasterizes and uploads a full frame each revealed step. Try a lower output resolution, slower typewriter, smaller text or Static. Shader outline/glow samples are bounded, but high-resolution fills still require GPU time. Test on the intended machine.

**No audio:** select an actual input or [System loopback] device and click Start input; audio mappings start at zero. Check Windows microphone privacy permissions and the selected endpoint. Loopback requires a working WASAPI output. Close other applications holding the device exclusively. After unplugging/changing an endpoint, Stop input, Refresh, select it and Start input. Input errors are displayed, and stale audio becomes zero within 0.5 seconds. Input capture runs on a worker; shutdown waits for its driver read to finish rather than terminating a live audio thread.

**Audio meter moves but visuals do not:** increase one of Scale/Glow/Brightness/Shake mappings and check Arena Audio amount. Noise threshold or low sensitivity may suppress energy. Attack/release smooth the three spectral bands. Bass transients are only a heuristic flash trigger; sustained bass can keep the meter high without retriggering. Use BPM mode for regularly timed flashes.

**Strobe will not arm:** enable it explicitly in the app, open the Arena Strobe enable gate, check flash intensity, and confirm a unique target is responding. Timeout, Stop, ID changes and preset recall disarm it. A heartbeat does not rearm a timed-out strobe. Rate uses the Arena rate multiplier; BPM uses the entered BPM and subdivision. Audio mode needs transients above threshold and beyond the minimum interval. Pulse width applies to periodic modes.

**Shortcut does nothing:** Ctrl+Enter/Ctrl+R/Ctrl+B are intentionally ignored while a text, number or combo editor has focus. Click elsewhere or use the large button. Escape remains an unconditional Stop Strobe shortcut.

**Build failures:** use Python x64 3.12 and the locked requirements. MSVC needs the C++ workload and Windows SDK. CMake's first configure needs network access to retrieve pinned nlohmann/json. For an offline build prepopulate FetchContent and use `-DFETCHCONTENT_SOURCE_DIR_JSON=<extracted-json-3.11.3>` with the verified archive. The FFGL SDK checkout must be the pinned v2.2 commit, not its changing master branch.

**GL tests fail under RDP:** the Windows software renderer may expose only OpenGL 1.1. Run in the physical GPU desktop session. Preserve failing test output; a build success alone does not prove rendering compatibility.
