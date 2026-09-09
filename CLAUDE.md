# Instrucciones para Claude Code en Capturia

Léelo entero antes de tocar nada. Lo que decidas que contradiga esto es un
error, aunque parezca una mejora.

## Lo primero

**Comprueba toda afirmación contra el código y contra un comando ejecutado.**
No des por bueno lo que diga un documento, un mensaje de commit ni tu memoria
sobre cómo funciona GSR. Si no lo has ejecutado, no está hecho. Si no puedes
comprobarlo, escríbelo como "sin verificar" en vez de rellenarlo.

Esto no es retórica: la Tanda 1 se quedó sin tres entregables por no tener
acceso al código de GSR, y la alternativa habría sido inventárselos. Ver
ESTADO.md, bloqueo B1.

## Qué es esto

Un grabador de pantalla puntero para Linux/Wayland (KDE Plasma). En Linux no
hay equivalente a Screen Studio ni a AutoZoom: OBS es potente pero crudo,
Spectacle y Kooha son mínimos.

**No reimplementamos la captura.** gpu-screen-recorder (GSR), de dec05eba, ya
resuelve lo difícil: codificación 100 % en GPU, HEVC/AV1, X11 y Wayland, replay
buffer, audio por aplicación. Lo nuestro es la capa que le falta: UI ligera,
modo audio-only y post-proceso.

GSR ya viene partido en dos binarios, el grabador y `gsr-cli`, que le habla por
IPC. Ese es el patrón que copiamos: nuestra UI controla al grabador igual que
`gsr-cli`, sin tocar la captura.

## Arquitectura

```
UI Qt6/QML + Kirigami (capa 3, fases posteriores)
  └── libcapturia (capa 1, C++20, sin GUI, sin Qt)
        ├── GSR por IPC, igual que gsr-cli  → captura de pantalla
        └── backend propio ffmpeg+PipeWire  → audio-only (ver más abajo)
  └── CLI `capturia` (capa 2, sobre libcapturia)
```

**Regla dura: si algo no funciona por CLI, no se toca la UI.** El CLI se
verifica con ffprobe; la UI no se verifica.

El backend propio de audio está **en el aire**: puede ser innecesario. No
escribas ni una línea hasta leer `docs/gsr-audio-only.md`.

## Prohibido

- Reimplementar la captura de pantalla.
- Modificar nada dentro de `third_party/gpu-screen-recorder/`. Es GPL-3.0 de
  terceros, solo lectura, y no se compila como parte nuestra.
- Hardcodear listas de códecs, dispositivos o resoluciones. Todo se detecta en
  runtime, en la máquina del usuario.
- Cambiar de backend sin consultar al titular.
- Añadir dependencias sin justificarlas en ESTADO.md.
- Añadir opciones que no sean imprescindibles.

## Principio de diseño

**Ligera e intuitiva. No es negociable y prima sobre añadir opciones.**

- Arranque por debajo de 1 segundo y poca RAM.
- Grabar en dos clics: Fuente, Grabar. Todo lo demás en "Avanzado", plegado.
- Defaults sensatos sin tocar nada: mkv, mejor códec de hardware disponible,
  audio de sistema, carpeta Vídeos.
- **Solo exponer lo que la máquina soporta de verdad.** `libcapturia` hace esa
  detección para que la UI nunca ofrezca algo que vaya a fallar. Cuando el
  parser no entiende una salida, deja un aviso; no se inventa una capacidad.
- Estética nativa Plasma (Kirigami/Breeze) cuando llegue la UI.
- Antirreferencia: la UI de OBS. Referencia: la sencillez de Spectacle.

## Cómo se escribe aquí

- **En español, y el código también**: nombres, comentarios, mensajes de commit
  y de error.
- **Los comentarios explican por qué, no qué.** Uno que repite la línea de
  debajo sobra. Uno que dice qué se descartó y por qué vale su peso.
- **Frases cortas.** Una idea por frase. Sin rayas largas en texto visible.
- Los identificadores en C++ van sin tildes; el texto que ve el usuario, con
  ellas.

## Comandos

```
cmake -S . -B build -G Ninja
cmake --build build                       # tiene que salir sin un solo warning
ctest --test-dir build --output-on-failure
./build/src/cli/capturia --check          # qué falta en esta máquina
scripts/volcar-capacidades.sh             # regenera docs/gsr-capabilities.txt
scripts/verify-recording.sh               # arnés de grabación (se activa en la Tanda 2)
```

`-Werror` está puesto por defecto (`CAPTURIA_WERROR`). Bajarlo es tapar un
problema, no resolverlo.

## Verificación obligatoria

Antes de dar una tanda por terminada:

1. Compila sin warnings.
2. `ctest --test-dir build --output-on-failure` en verde.
3. Si toca grabación, `scripts/verify-recording.sh` en verde.

**Nunca marques algo como hecho sin haber ejecutado un comando real que lo
pruebe.** Si un error te bloquea dos veces seguidas, documéntalo en ESTADO.md y
para.

## Versión mínima de GSR: sin decidir

`capturia::version_minima_gsr()` devuelve `nullopt` a propósito. Para fijarla
hace falta leer `third_party/gpu-screen-recorder/project.conf` y la salida real
de `gpu-screen-recorder --version` en la máquina de desarrollo, y ninguna de las
dos ha estado disponible todavía.

El criterio para decidirla, cuando se pueda: la versión más antigua que ya traiga
el IPC de `gsr-cli` completo, porque de eso depende toda nuestra capa de
control. Una versión anterior no nos sirve por mucho que grabe bien. Fíjala aquí
con el número y la razón, y quita el `nullopt`.

## Licencia

GSR es GPL-3.0. Lo usamos como **proceso externo por IPC**, sin enlazar su
código, y esa distinción es la que mantiene nuestra UI fuera de la GPL-3.0.
Pendiente de escribir `docs/LICENSING.md` con el razonamiento completo y con qué
cambiaría si algún día enlazáramos su código. No lo enlaces; el documento es
para explicar el riesgo, no para abrir la puerta.

## Datos y cifras

Ninguna cifra entra en un documento sin decir de dónde sale y de qué fecha es.
Si no hay fuente, se escribe "sin dato". Una capacidad de la máquina se afirma
con el volcado delante, no de memoria.
