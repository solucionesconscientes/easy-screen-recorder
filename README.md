<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

<p align="center">
  <img src="docs/branding/readme-header.svg" alt="Easy Screen Recorder" width="100%">
</p>

<p align="center">
  <a href="README.md"><strong>English</strong></a> ·
  <a href="README.es.md">Castellano</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-GPL--3.0--or--later-blue" alt="GPL-3.0-or-later">
  <img src="https://img.shields.io/badge/platform-Linux%20%C2%B7%20Wayland%20%26%20X11-1d99f3" alt="Linux · Wayland and X11">
  <img src="https://img.shields.io/badge/C%2B%2B-20-da4453" alt="C++20">
</p>

---

<!-- TODO: demo GIF. See docs/screenshots/README.md for how to record it. -->
<p align="center">
  <em>Demo GIF coming soon — <code>docs/screenshots/demo.gif</code></em>
</p>

## What it does

Recording your screen on Wayland is usually one of two things: a tool that
re-encodes on the CPU and turns your laptop into a heater, or a command line
with fifteen flags. This is the missing layer — capture stays on the GPU and you
start recording in two clicks.

- **Records any monitor, a region you drag, the focused window or a camera**,
  with the encoding done on the GPU.
- **Audio-only mode**, to Opus, AAC, FLAC, WAV or MP3, without turning on video
  capture at all.
- **Pause and resume** mid-recording, plus a **global shortcut** to start and
  stop without leaving what you are doing.
- **Picks the audio device for you** — system audio, your microphone, or
  several tracks at once — and **remembers where your recordings go**.
- **Tells you what is missing before you press record**, instead of failing
  halfway through: `easy-screen-recorder-cli --check`.

Sensible defaults out of the box: `.mkv`, the best hardware codec your GPU has,
system audio and 60 fps. Everything else is an option.

There is a CLI as well as the GUI, and the rule is that the CLI comes first: if
something does not work from the command line, the interface does not get it.

## Install

| | |
|---|---|
| **Flathub** | Coming soon |
| **`.deb`** | Coming soon |
| **From source** | Below |

### From source

Build dependencies, exactly what the CMake asks for:

- **CMake 3.25+** and a **C++20** compiler
- **Ninja** (or any generator you prefer)
- **Qt 6.4+**, components: `Core` `DBus` `Gui` `Qml` `Quick` `QuickControls2`
  `Widgets` `Concurrent`
- **`lrelease`** (Debian: `qt6-l10n-tools`) to build the English translation

Qt is **optional**: without it the GUI is skipped and the CLI and the tests
still build.

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

On Debian and Ubuntu:

```bash
sudo apt install cmake ninja-build g++ qt6-base-dev qt6-declarative-dev \
  qt6-l10n-tools ffmpeg
```

## Language

The interface follows your system language: **Spanish** if your system is in
Spanish, **English** otherwise — including languages that are neither, because
English is more useful to a German speaker than Spanish they cannot read.

**Still in Spanish only:** the error messages that come from the core library
(`libesr`), and the whole command-line interface. Those are plain strings in a
Qt-free library, so translating them properly means returning error codes
instead of text — a refactor in the files that build the recorder's command
line, which is where a mistake does not fail a build, it breaks a recording.
Tracked in `docs/ROADMAP.md`.

## Where it runs

**Any Linux desktop, on Wayland and on X11.** The interface is Qt6/QML with
Kirigami, and the capture is done by gpu-screen-recorder, which supports both
display servers.

Two things are specific to KDE Plasma, and both degrade quietly elsewhere:

| | |
|---|---|
| **Global shortcut** | Registered through KGlobalAccel, so it is configured in *System Settings → Shortcuts* like any other. Outside KDE it simply is not registered — everything else works |
| **Tray icon** | Uses StatusNotifierItem, which is native on Plasma. Most desktops support it; GNOME needs an extension |

Built and verified on KDE Plasma over Wayland, and verified running as an X11
client through XWayland. **A pure X11 session has not been tested here** — this
machine runs Wayland — but the capture is gpu-screen-recorder's, which supports
X11 upstream.

### Modest hardware is the point

The encoding happens on the GPU, so the CPU stays free. That matters most
exactly where a CPU-encoding recorder hurts most: a thin laptop with no thermal
headroom.

Measured on the development machine — an **Intel i5-6200U from 2015 with
integrated HD Graphics 520**, which encodes H.264 and HEVC in hardware:

| | |
|---|---|
| CLI, minimum process cost | **0,00 s · 4 MB** of peak RAM |
| CLI, full environment detection | **0,6–1,2 s · 49 MB** |
| GUI, to the first painted frame | **1,0–1,6 s · 94 MB** |

**The one thing to check first:** your GPU needs a hardware encoder. Without
one, gpu-screen-recorder falls back to CPU encoding (`h264_software`) and the
whole advantage disappears. `easy-screen-recorder-cli --check` prints which case
you are in — look for the `por defecto (hardware)` line.

## Requirements

**[gpu-screen-recorder][gsr] has to be installed.** It does the actual capture.
It is not bundled with this program and it is not a library we link — it runs as
a separate process.

```bash
flatpak install flathub com.dec05eba.gpu_screen_recorder
```

Install it **system-wide, not `--user`**: the lookup checks `/var/lib/flatpak`,
and a user install lands somewhere else.

`ffmpeg` is needed for the audio-only mode, which does not go through GSR at
all.

Not sure what you have? `easy-screen-recorder-cli --check` prints exactly what
is there, what is missing, and what stops working because of it.

[gsr]: https://git.dec05eba.com/gpu-screen-recorder/about/

## Architecture

```
Qt/QML/Kirigami UI  →  libesr  →  gpu-screen-recorder (external process)
```

`libesr` is the core: no Qt, no GUI, everything the UI and the CLI need to know
about the environment. GSR is driven as a separate process — arguments out, JSON
over a unix socket back — which is what keeps the two licences apart and lets
the capture backend be swapped without rewriting the interface.

## Credits

- **[gpu-screen-recorder][gsr]** by **dec05eba** — does the hard part. This is
  an independent frontend, not affiliated with the project.
- **[Qt](https://www.qt.io/)** and **[KDE Frameworks](https://develop.kde.org/products/frameworks/)**
  — the interface.
- **[FFmpeg](https://ffmpeg.org/)** — the audio-only path.

## Need custom software?

Built by **[Soluciones Conscientes](https://solucionesconscientes.es)**, which
develops bespoke applications — desktop, web and the plumbing in between.

If this is close to what you need but not quite, or you want it built for your
own workflow, get in touch. Commercial licensing of this codebase is also
available on request.

## License

**GPL-3.0-or-later**, see [`LICENSE`](LICENSE). Uses gpu-screen-recorder
(GPL-3.0-only) as a separate external program; it is not included or
distributed. Commercial licensing available on request.

Contributions are welcome and require signing a CLA — you keep your copyright.
See [`CONTRIBUTING.md`](CONTRIBUTING.md).
