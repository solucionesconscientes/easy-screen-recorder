# Contributing to Easy Screen Recorder

Thanks for considering it. Two things are non-negotiable, so they come first.

## 1. Sign the CLA

Easy Screen Recorder is **GPL-3.0-or-later**, and the copyright holder —
**Dalmau Romaní (Soluciones Conscientes)** — also grants commercial licences to
clients who need a closed build. For that to stay possible, every line in the
repository must either belong to the holder or be covered by a Contributor
License Agreement.

- **You keep your copyright.** The CLA is a *licence grant*, not an assignment.
- You grant the holder the right to **relicense your contribution**, including
  under a commercial licence.
- Signing happens through [CLA Assistant](https://cla-assistant.io/): it
  comments on your pull request and blocks merging until you sign. One click.

The agreement lives in [`docs/CLA.md`](docs/CLA.md). If that file still shows a
TODO, the project is not accepting external contributions yet — open an issue
instead and say what you had in mind.

## 2. Never copy code from gpu-screen-recorder

Easy Screen Recorder drives `gpu-screen-recorder` as a **separate external
process**: command-line arguments out, JSON over a unix socket back. It is not
linked, not vendored and not redistributed. That boundary is what keeps the two
licences apart, and it is load-bearing.

So:

- **Do not copy, paste, adapt or translate** code from `gpu-screen-recorder`,
  not even into another language. It is GPL-3.0-**only** and cannot be
  relicensed.
- The same goes for any other project.
- Reading its source to understand the protocol is fine, and citing
  `file:line` in a comment or a document is fine. Pasting a fragment is not.
- `docs/gsr-ipc.md` and `docs/gsr-audio-only.md` are the reference: they
  describe the protocol in our own words, on purpose.

## Architecture, in three lines

```
Qt/QML/Kirigami UI  →  libesr  →  gpu-screen-recorder (external process)
```

`libesr` has no Qt and no GUI. Anything the UI or the CLI needs to know about
the environment lives there, so the UI never offers something that is going to
fail. **If it does not work from the CLI, do not touch the UI.**

## Build and test

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires a C++20 compiler and CMake 3.25+. Qt 6.4+ is optional: without it the
UI is skipped and the CLI and the tests still build.

Warnings are errors by default (`-Werror`). You can turn that off with
`-DESR_WERROR=OFF` on an unusual compiler, but then fix the warnings.

For a real end-to-end check, with `gpu-screen-recorder` installed:

```bash
./scripts/verify-recording.sh
```

It records for real and checks the result with `ffprobe`, because a recording
that *looks* fine is exactly the failure this project cannot afford.

## Commits

- **In Spanish**, like the rest of the repository's comments and history.
- One logical change per commit.
- The subject line says what changes, in the imperative.
- The body says **why**, and what was verified — with the numbers. "Builds
  clean" is not verification on its own.
- New files need an SPDX header. `reuse lint` has to pass.

---

# Contribuir — resumen en castellano

**Dos reglas que no se negocian:**

1. **Firma el CLA** (`docs/CLA.md`), vía CLA Assistant, antes de que se acepte
   un PR. Conservas tu copyright; lo que concedes es el derecho a relicenciar,
   que es lo que sostiene el modelo dual. Si el fichero sigue con el TODO,
   todavía no se aceptan contribuciones externas: abre un issue.
2. **No copies ni adaptes código de `gpu-screen-recorder`** ni de ningún otro
   proyecto, ni traducido a otro lenguaje. GSR es GPL-3.0-**only** y no se puede
   relicenciar. Leer su código para entender el protocolo sí; pegar un fragmento
   no. Citar `fichero:línea` sí.

**Arquitectura:** UI Qt/QML → `libesr` → `gpu-screen-recorder` como proceso
externo. `libesr` no tiene Qt. Si algo no funciona por CLI, no se toca la UI.

**Compilar y probar:** `cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build`.
Los warnings son errores; se puede bajar con `-DESR_WERROR=OFF`, pero entonces
se arreglan. Para la prueba de verdad, `./scripts/verify-recording.sh`, que graba
y comprueba con `ffprobe`.

**Commits:** en español, uno por cambio lógico, el cuerpo dice el **por qué** y
qué se verificó con su cifra. Fichero nuevo, cabecera SPDX: `reuse lint` tiene
que pasar.
