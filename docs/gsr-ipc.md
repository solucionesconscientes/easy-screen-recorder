# El protocolo IPC de gpu-screen-recorder

**Estado: SIN DOCUMENTAR. Bloqueado (bloqueo B1 de ESTADO.md).**

Este documento tenía que describir el protocolo IPC real de GSR citando fichero
y línea. No lo hace, y no lo hace a propósito: **el código de GSR no está
disponible en el entorno donde se ejecutó la Tanda 1.** Escribirlo de memoria
sería exactamente el fallo que el proyecto no se puede permitir, así que aquí
queda lo que sí se puede afirmar: qué se intentó, qué falló y qué hay que hacer
para completarlo.

## Por qué está bloqueado

La Tanda 1 se ejecutó en un contenedor remoto, no en la máquina de desarrollo.
Comprobado, no supuesto:

| Comprobación | Resultado |
|---|---|
| `~/Desktop/app-audio/capturia` | no existe |
| `third_party/gpu-screen-recorder/` en el repo | no está: lo excluye `.gitignore`, como estaba previsto |
| `which gpu-screen-recorder gsr-cli` | sin salida: no están instalados |
| `/dev/dri` | no existe: sin GPU accesible |
| `XDG_SESSION_TYPE`, `WAYLAND_DISPLAY`, `DISPLAY` | vacíos: sin sesión gráfica |
| `https://git.dec05eba.com/...` | bloqueado por la política de salida del proxy |
| `https://repo.dec05eba.com/gpu-screen-recorder` | CONNECT rechazado con 403 |
| `https://github.com/dec05eba/gpu-screen-recorder` | 404 |

O sea: ni el árbol local, ni los binarios, ni el origen upstream. No hay ninguna
fuente primaria que leer.

Aunque el upstream fuera accesible, tampoco valdría del todo: lo que importa es
la versión instalada en la máquina de desarrollo y el árbol de
`third_party/`, no el HEAD de upstream. Una cita `fichero:línea` sacada de otro
checkout apunta a líneas que no son las que hay.

## Lo que se afirma en el encargo y sigue sin verificar

Nada de esta lista está comprobado. Son las hipótesis a confirmar, no datos:

- Que GSR viene partido en dos binarios: el grabador y `gsr-cli`, que le habla
  por IPC.
- Que existen `include/cli/ipc.h`, `src/cli/ipc.c`, `src/cli/commands.c` y
  `tools/gsr-cli/main.c`. La distribución de directorios que conozco de GSR es
  otra, así que estas rutas hay que confirmarlas antes de citarlas.
- Que existen las páginas de manual `gsr-cli.1` y `gpu-screen-recorder.1`.
- Que las opciones sondeadas por `scripts/volcar-capacidades.sh` (`--info`,
  `--list-capture-options`, `--list-audio-devices`,
  `--list-application-audio`) son las reales.

El parser de `libcapturia` está escrito dando por hecho **lo mínimo posible**
sobre todo esto. El envoltorio del volcado (`### comando:` / `### codigo:` /
`### fin`) lo generamos nosotros y sí está verificado y cubierto por tests. La
lectura del contenido de cada bloque es una heurística conservadora, marcada
como tal en `src/core/include/capturia/capacidades.hpp`, que ante un formato que
no reconoce deja un aviso en vez de inventarse un campo.

## Qué hay que hacer para desbloquearlo

En la máquina de desarrollo, con `third_party/gpu-screen-recorder/` presente:

1. `gpu-screen-recorder --version` y `cat third_party/gpu-screen-recorder/project.conf`.
   Con esos dos datos se fija la versión mínima soportada, que hoy sigue
   sin decidir (`capturia::version_minima_gsr()` devuelve `nullopt`).
2. `find third_party/gpu-screen-recorder -name '*ipc*'` y `ls third_party/gpu-screen-recorder/tools/`
   para confirmar o corregir las rutas de la lista de arriba.
3. Leer el transporte: ¿socket de dominio unix, FIFO, señales? ¿Dónde se crea la
   ruta del socket y de qué depende (usuario, `XDG_RUNTIME_DIR`, PID)?
4. Leer el formato de mensaje: ¿texto de una línea, struct binario con longitud
   por delante, algún tipo enumerado? ¿Hay respuesta, o es unidireccional?
5. Enumerar los comandos reales y, de cada uno, qué contesta el grabador.
   Interesan sobre todo: arrancar, parar, pausar, reanudar, guardar el replay
   buffer y consultar estado.
6. **Cómo llega la ruta del fichero guardado.** Es el dato que más falta le hace
   a la UI y el que no se puede adivinar. ¿Lo devuelve el IPC, se imprime por
   stdout del grabador, se deduce de la plantilla de nombre?
7. `scripts/volcar-capacidades.sh` para regenerar `docs/gsr-capabilities.txt`
   con datos reales, y después revisar `interpretar_lista()` contra ese volcado.

Cada dato que entre en este documento va con su cita `fichero:línea`. Lo que no
quede claro leyendo el código se escribe como "no queda claro", no se rellena.
