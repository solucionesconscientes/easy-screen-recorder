# Contributing to Easy Screen Recorder

Thanks for considering it.

## Right now: issues yes, code not yet

**Bug reports and issues are very welcome. Pull requests with code cannot be
merged yet** — the contribution agreement is still pending (see below).

That is not a brush-off, and there is something better you can do: **open an
issue and describe the problem.** A good description of what broke, on what
hardware, with what command, is worth as much as the patch — often more. Tell me
what line is wrong and what it should be, and I will write it. That way you are
not blocked on paperwork and the fix still happens.

Same for a fix you have already written locally: describe it in an issue. Do not
paste the diff — if I read your code I cannot cleanly write my own version of
it, and that is exactly the problem the agreement exists to avoid.

Nothing stops you forking it, either. It is GPL-3.0-or-later; that is your
right and it always will be.

---

Two things are non-negotiable, so they come first.

## 1. Sign the CLA

Easy Screen Recorder is **GPL-3.0-or-later**, and the copyright holder —
**Dalmau Romaní (Soluciones Conscientes)** — also grants commercial licences to
clients who need a closed build. For that to stay possible, every line in the
repository must either belong to the holder or be covered by a Contributor
License Agreement.

- **You keep your copyright.** The CLA is a *licence grant*, not an assignment.
- You grant the holder the right to **relicense your contribution**, including
  under a commercial licence.
- Signing will happen through a GitHub Action
  ([CLA Assistant Lite](https://github.com/contributor-assistant/github-action)):
  it comments on your pull request and blocks merging until you sign. One
  comment, and the signature record lives in this repository — no third-party
  service holds it.

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

## Ahora mismo: los issues sí, el código todavía no

**Los reportes de fallo son muy bienvenidos. Los pull requests con código
todavía no se pueden fusionar** — el acuerdo de contribución está pendiente de
redactar (ver abajo).

No es un «no» seco, y hay algo mejor que puedes hacer: **abre un issue y
describe el problema.** Una buena descripción de qué se rompió, en qué hardware
y con qué comando vale tanto como el parche, y muchas veces más. Dime qué línea
está mal y qué debería decir, y la escribo yo. Así no te bloquea el papeleo y el
arreglo se hace igual.

Lo mismo si ya lo has arreglado en tu copia: descríbelo en un issue. **No pegues
el diff** — si leo tu código ya no puedo escribir limpiamente mi propia versión,
y ese es justo el problema que el acuerdo existe para evitar.

Y nada te impide hacer un fork. Es GPL-3.0-or-later: es tu derecho y lo seguirá
siendo.

---

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
