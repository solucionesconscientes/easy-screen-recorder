# Instrucciones para Claude Code en Easy Screen Recorder

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

Un grabador de pantalla puntero para **Linux**, sobre Wayland y sobre X11, y en
cualquier escritorio. En Linux no hay equivalente a Screen Studio ni a AutoZoom:
OBS es potente pero crudo, Spectacle y Kooha son mínimos.

Se desarrolla y se verifica en KDE Plasma, y solo dos cosas están atadas a él:
el atajo global (KGlobalAccel) y el icono de bandeja (StatusNotifierItem). Las
dos se degradan sin ruido fuera. **No escribas «para KDE Plasma» en un texto de
usuario:** deja fuera a quien podría usarlo, y la captura la hace GSR, que
soporta los dos servidores gráficos.

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
  └── libesr (capa 1, C++20, sin GUI, sin Qt)
        ├── GSR por IPC, igual que gsr-cli  → captura de pantalla
        └── backend propio ffmpeg+PipeWire  → audio-only (ver más abajo)
  └── CLI `easy-screen-recorder-cli` (capa 2, sobre libesr)
```

**Regla dura: si algo no funciona por CLI, no se toca la UI.** El CLI se
verifica con ffprobe; la UI no se verifica.

El backend propio de audio está **en el aire**: puede ser innecesario. No
escribas ni una línea hasta leer `docs/gsr-audio-only.md`.

## Prohibido

- Reimplementar la captura de pantalla.
- **Copiar, adaptar o traducir codigo de GSR.** Ni una linea, ni en C++, ni
  pegada en un documento. GSR es GPL-3.0-**only** y no se puede relicenciar: una
  sola linea suya dentro del repositorio rompe el modelo dual, que es lo que
  permite conceder licencias comerciales. Leer su codigo para entender el
  protocolo si; citar `fichero:linea` si; pegar un fragmento no. La auditoria
  del 2026-09-15 encontro nueve lineas de C suyas en `docs/gsr-ipc.md` y
  `docs/gsr-audio-only.md` y se reescribieron como prosa.
- Modificar nada dentro de la copia de referencia de GSR, que vive en
  `~/Documentos/PROJECTES/app-audio/referencia/gpu-screen-recorder/`,
  **fuera del repositorio**. Es GPL-3.0 de terceros, solo lectura, y no se
  compila como parte nuestra.
- Hardcodear listas de códecs, dispositivos o resoluciones. Todo se detecta en
  runtime, en la máquina del usuario.
- Cambiar de backend sin consultar al titular.
- Añadir dependencias sin justificarlas en ESTADO.md.
- Añadir opciones que no sean imprescindibles.

## Principio de diseño

**Ligera e intuitiva. No es negociable y prima sobre añadir opciones.**

- Arranque por debajo de 1 segundo y poca RAM.
- Grabar en dos clics: Fuente, Grabar. Los dos viven en una cabecera que NO se
  desplaza nunca, junto al perfil. Lo demás va debajo, en una sola columna de
  tarjetas con título, ordenadas por el momento en que se piensan: qué grabo,
  vídeo, audio, mientras grabo, al terminar, dónde se guarda.
  - Estuvo plegado tras un botón "Avanzado" hasta la 0.9.0 y se quitó por lo
    que pasó al probarlo: la primera opción que alguien buscó de verdad la
    buscó en la ventana, no la encontró, y dio por hecho que no existía. Nada
    vuelve a esconderse detrás de un botón sin nombre.
  - Estuvo en dos columnas hasta la 0.10.0, y se quitó por lo mismo que se mide,
    no por gusto: la ventana salía de 1114x866 en una pantalla de 1366x768, o
    sea que no cabía en la pantalla donde se diseñaba. Ahora son 540x687.
  - **Las secciones se pliegan, y su título enseña lo que hay dentro**: «Vídeo ·
    Muy alta · 60 fps · h264». Eso es lo que lo separa del "Avanzado" que se
    quitó, y no es opinable: desplegado eran 2567 px de contenido en un hueco de
    445 —5,8 pantallas de bajar— y plegado son 332, o sea ninguna. Un título que
    no diga su contenido vuelve a ser "Avanzado"; no lo quites.
- **Cada opción lleva su explicación escrita debajo, siempre a la vista.**
  Vivieron en botones «i» hasta la 0.10.0 y llegó a haber diecinueve en la
  misma ventana: eran la textura más visible de la página. Una explicación que
  hay que descubrir y perseguir con el ratón no está puesta.
- **El color solo significa, nunca decora**, y su contraste se mide contra el
  fondo donde cae. Las proporciones y el ritmo salen de una serie, no del ojo.
  Todo eso está en `docs/diseno.md`, con los números y de dónde salen.
- Defaults sensatos sin tocar nada: mkv, h264 cuando la máquina lo codifica por
  hardware (por compatibilidad, no por calidad), sistema y micrófono mezclados si
  hay micrófono (solo sistema si no lo hay), carpeta Vídeos.
- **Solo exponer lo que la máquina soporta de verdad.** `libesr` hace esa
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
./build/src/cli/easy-screen-recorder-cli --check          # qué falta en esta máquina
scripts/volcar-capacidades.sh             # regenera docs/gsr-capabilities.txt
scripts/verify-recording.sh               # arnés de grabación (se activa en la Tanda 2)
```

`-Werror` está puesto por defecto (`ESR_WERROR`). Bajarlo es tapar un
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

`esr::version_minima_gsr()` devuelve `Version{6, 0, 0}`.

El criterio era: la versión más antigua que ya traiga el IPC de `gsr-cli`
completo, porque de eso depende toda nuestra capa de control. **6.0.0 es la más
antigua que se ha podido comprobar que lo trae**, y de ahí sale el número:

- `su project.conf:4` dice `version = "6.0.0"`.
- `gpu-screen-recorder --version` responde `6.0.0` en la máquina de desarrollo.
- Sobre esa versión se ejecutó el protocolo entero, incluida la respuesta
  diferida de `stop` que devuelve la ruta del fichero guardado. Está en
  `docs/gsr-ipc.md`, con la transcripción.

`gsr-cli.1:1` lleva sellado `5.15.3`, así que `gsr-cli` ya existía antes. **No se
baja el mínimo por eso**: que existiera el binario no dice que su IPC estuviera
completo, y sin ese árbol delante sería suponer. Se baja el día que se lea ese
código, no antes.

## Licencia

**Easy Screen Recorder es GPL-3.0-or-later.** Titular: Dalmau Romaní (Soluciones
Conscientes). Decidido el 2026-09-15. Cada fichero lleva cabecera SPDX y lo que
no admite comentario está en `REUSE.toml`; `reuse lint` tiene que pasar.

**Y hay modelo dual:** el titular puede conceder licencias comerciales del mismo
código. Eso impone dos disciplinas que no son opcionales:

1. Todo el código es del titular o está cubierto por un CLA. Ninguna
   contribución externa entra sin firmarlo (`CONTRIBUTING.md`, `docs/CLA.md`).
2. Nada copiado de terceros. Ver «Prohibido» arriba.

GSR es GPL-3.0-**only**. Lo usamos como **proceso externo por IPC**, sin enlazar
su código, y esa distinción es la que mantiene su licencia fuera de la nuestra.
El razonamiento completo, los cuatro requisitos del modelo dual y el resultado
de la auditoría están en `docs/LICENSING.md`, sección «Decisión». No lo enlaces;
el documento explica el riesgo, no abre la puerta.

**Commits:** el autor es el titular y la herramienta va en `Co-Authored-By:`. Un
modelo no puede ser autor ni firmar un CLA, y un `Author:` de un tercero deja un
hueco en la cadena de titularidad que hay que explicar en cada revisión. El
historial se reescribió una vez por esto, mientras el repositorio era privado;
ahora ya no se puede y no hace falta. Ver `docs/LICENSING.md`, «Autoría del
historial».

## Datos y cifras

Ninguna cifra entra en un documento sin decir de dónde sale y de qué fecha es.
Si no hay fuente, se escribe "sin dato". Una capacidad de la máquina se afirma
con el volcado delante, no de memoria.
