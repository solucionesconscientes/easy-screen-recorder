# Instrucciones para Claude Code en Capturia

Léelo entero antes de tocar nada. Lo que decidas que contradiga esto es un
error, aunque parezca una mejora.

## Lo primero

**Comprueba toda afirmación contra el código y contra un comando ejecutado.**
No des por bueno lo que diga un documento, un mensaje de commit ni tu memoria
sobre cómo funciona GSR. Si no lo has ejecutado, no está hecho. Si no puedes
comprobarlo, escríbelo como "sin verificar" en vez de rellenarlo.

Esto no es retórica: la Tanda 1 se quedó sin tres entregables por no tener
acceso al código de GSR, y la alternativa habría sido inventárselos. Se
entregaron en la Tanda 2, ya con el código y los binarios delante, y el bloqueo
B1 quedó cerrado. Esperar salió más barato que inventar.

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

## Versión mínima de GSR: 6.0.0

`capturia::version_minima_gsr()` devuelve `Version{6, 0, 0}`.

El criterio era: la versión más antigua que ya traiga el IPC de `gsr-cli`
completo, porque de eso depende toda nuestra capa de control. **6.0.0 es la más
antigua que se ha podido comprobar que lo trae**, y de ahí sale el número:

- `third_party/gpu-screen-recorder/project.conf:4` dice `version = "6.0.0"`.
- `gpu-screen-recorder --version` responde `6.0.0` en la máquina de desarrollo.
- Sobre esa versión se ejecutó el protocolo entero, incluida la respuesta
  diferida de `stop` que devuelve la ruta del fichero guardado. Está en
  `docs/gsr-ipc.md`, con la transcripción.

`gsr-cli.1:1` lleva sellado `5.15.3`, así que `gsr-cli` ya existía antes. **No se
baja el mínimo por eso**: que existiera el binario no dice que su IPC estuviera
completo, y sin ese árbol delante sería suponer. Se baja el día que se lea ese
código, no antes.

## Licencia

GSR es GPL-3.0-only. Lo usamos como **proceso externo por IPC**, sin enlazar su
código, y esa distinción es la que mantiene nuestra UI fuera de la GPL-3.0.

El razonamiento completo está en `docs/LICENSING.md`, con lo que cambiaría si
algún día enlazáramos su código. No lo enlaces; el documento explica el riesgo,
no abre la puerta. Sus cuatro reglas, en corto: GSR solo como proceso externo,
`third_party/` de lectura, ni un `#include` que apunte ahí, y releerlo antes de
empaquetar GSR con Capturia.

La licencia de **Capturia** sigue sin elegirse. Es decisión del titular.

## Datos y cifras

Ninguna cifra entra en un documento sin decir de dónde sale y de qué fecha es.
Si no hay fuente, se escribe "sin dato". Una capacidad de la máquina se afirma
con el volcado delante, no de memoria.
