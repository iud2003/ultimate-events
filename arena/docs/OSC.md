# PulseText OSC protocol v1

These are custom messages received by PulseText DLLs. They are **not Arena OSC addresses**. Arena's OSC input preferences are unrelated.

Transport: OSC 1.0 messages over IPv4 UDP. No bundles, timetags, broadcast IDs or OSC pattern matching. Source module defaults to UDP 12000; effect module defaults to 12001. Each DLL shares one receiver thread/socket across its live instances. The last instance shuts down and joins its receiver. No OpenGL calls occur on that worker.

Ports can be changed with process environment variables `PULSETEXT_SOURCE_PORT` and `PULSETEXT_EFFECT_PORT` (1024–65535), set **before launching Arena**. Restart Arena after changing them. Binding is exclusive: a port conflict does not select an unpredictable fallback port. It produces a Windows debug message and no status replies; choose an unused port. Use distinct ports for multiple Arena processes.

All control addresses are `/pulsetext/{id}/{command}`, where `id` is a decimal integer 1–999. Configure the matching **Control ID** parameter in Arena; its numeric display shows the mapped integer. Internally FFGL stores `(id - 1) / 998` as a normalized parameter. IDs must be unique within a DLL. Zero matching IDs produce status count 0; duplicate IDs produce count >1 and **reject every control command**. IDs can be reused across Source and Effect because their ports differ. ID changes disarm strobe.

| Command | OSC type tags | Payload / meaning |
|---|---|---|
| `state` | `,s` | One UTF-8 JSON object containing the complete settings snapshot below |
| `audio` | `,s` | JSON object with float `bass`, `mid`, `treble`, `flash`, all 0–1; extra `level` is ignored by plugins |
| `ping` | `,` | Requests status, including absent or duplicate ID counts |
| `heartbeat` | `,` | Refreshes control watchdog |
| `restart` | `,` | Restarts elapsed animation/strobe clock on receipt |
| `stop` | `,` | Latches strobe off, leaves text and video visible |
| `blackout` | `,s` | JSON `{ "enabled": true }`; source transparent, effect passes original video |

Reply: `/pulsetext/status`, tags `,s`, JSON `{ "id": 1, "instances": 1, "port": 12000, "version": "1.0" }`. Sent to the request's source IP and UDP source port. The desktop app binds an ephemeral local port and only accepts replies from its configured plugin endpoint matching its current ID. “Connected” means exactly one registered instance has answered within two seconds; it does not guarantee the clip is playing or its GPU render succeeded. No replies or zero/duplicate counts never show Connected.

For a unique instance, status also reports `received_text`, `rendered_text`, successful `frames`, `render_age` in seconds (-1 before any successful frame), and `render_error`. Use `python -m desktop.diagnose --port 12000 --id 1` to inspect these without changing plugin settings. A received/rendered mismatch or growing render age separates message delivery from a host that is not drawing the clip; an error reports a failed text rasterization.

Maximum accepted datagram 49,152 bytes; controller limit 48,000 bytes. Text limited by controller to 16,000 UTF-8 bytes, multiline. OSC strings include NUL termination and four-byte padding. JSON floats must be finite. Numeric plugin inputs are clamped; invalid JSON is discarded. Use a trusted local network: this protocol has no authentication or encryption. UDP packets can be lost or reordered; Apply resends a complete snapshot. Continuous controls are limited to 20 Hz, text debounced 180 ms, audio to about 30 Hz, ping/heartbeat 2 Hz. Safety Stop is sent three times. Preset recall never arms strobe.

## Settings object

The canonical defaults and validation live in `desktop/core.py`. Unknown keys are ignored by the controller; omitted keys in custom plugin snapshots use engine defaults.

| Key | JSON type | Range / values |
|---|---|---|
| text, font | string | Unicode text; installed Windows font family (default Segoe UI) |
| size | number | 8–500 output pixels |
| bold, italic | boolean | Font styles, subject to font availability |
| colour, background | number[4] | RGBA, each 0–1; background defaults alpha 0 |
| opacity | number | 0–1 |
| horizontal | string | Left, Centre, Right |
| vertical | string | Top, Middle, Bottom |
| x, y | number | -2–2 of output dimension; positive x right, positive y down |
| rotation | number | -360–360 degrees |
| scale | number | 0.05–5 |
| letter_spacing | number | -20–100 pixels added per glyph cluster |
| line_spacing | number | 0.5–3 times font size |
| outline | number | 0–20 pixels, same colour as text |
| glow | number | 0–3 |
| animation | string | Static, Scroll left, Scroll right, Scroll up, Scroll down, Fade in/out, Zoom pulse, Bounce, Typewriter, Shake |
| speed, intensity | number | 0–10, 0–2; intensity affects pulse/bounce/shake |
| audio_scale, audio_brightness, audio_shake | number | 0–2 |
| audio_glow | number | 0–3 |
| strobe, blackout | boolean | Default false |
| strobe_clock | string | Rate, BPM, Audio |
| strobe_target | string | Text, Background, Whole output |
| strobe_rate | number | 0.1–30 Hz (before Arena multiplier) |
| strobe_intensity | number | 0–1 |
| pulse_width | number | 0.01–0.95 fraction of periodic cycle |
| bpm, subdivision | number | 20–300 BPM, 0.25–8 flashes per beat |
| timeout | number | 0.3–10 seconds; default 2 |

Controller-only analysis settings (carried in snapshots but not consumed by plugins): `sensitivity` 0.1–20, `threshold` 0–1, `attack` 0.001–1 seconds, `release` 0.01–3 seconds, `trigger_threshold` 0–1, `trigger_interval` 0.05–3 seconds.

## Ownership and precedence

Appearance, text, animation choice, mapping amounts, BPM and strobe clock/target are OSC-owned. Engine settings persist in memory for each instance; save a desktop preset and reapply after reopening a composition. OSC text/settings are not serialized in Arena's composition. Arena saves its numeric FFGL parameters.

| Arena parameter | Normalized 0–1 interpretation | Combined result |
|---|---|---|
| Opacity | default 1 | OSC opacity × Arena value |
| Anim speed | 0–10, default 1 at slider 0.1 | OSC speed × Arena speed |
| Anim intensity | 0–2, default 1 at 0.5 | OSC intensity × Arena intensity |
| Audio amount | default 1 | Multiplies weighted band envelope |
| Strobe enable | Boolean gate at >0.5, default open | Both OSC enabled and Arena gate required |
| Strobe rate | 0–5 multiplier, default 1 at 0.2 | Multiplies Rate clock only; BPM clock uses explicit BPM/subdivision |
| Strobe intensity | default 1 | OSC intensity × Arena value |
| Control ID | 1–999 mapped to 0–1 | Arena owns routing; not writable via OSC |
| Blackout | Boolean at >0.5, default off | Either Arena or OSC blackout hides generated layer |

OSC never overwrites Arena parameters, so automation cannot fight incoming state. The safety watchdog checks monotonic time on every rendered frame and latches strobe off after loss of state/audio/heartbeat messages; returning heartbeat alone does not rearm it. A new enabled `state` snapshot explicitly rearms. No output is rendered while a clip is suspended; the check runs before its next frame. Stop does not hide text; Blackout does not modify the incoming video. Audio mappings return to zero when control traffic becomes stale.

Timing is monotonic elapsed time, independent of frame count. Restart resets animation and periodic strobe phase. BPM mode is a free-running clock with manually entered BPM, not host transport synchronization. Audio mode uses bass spectral transients with minimum trigger interval; it is not accurate beat tracking. Transient flashes last one controller frame; pulse width applies to Rate/BPM modes.
