# Estado del proyecto

Fecha: 2026-09-15. Última tanda ejecutada: **Tanda 5, completa.**

## Resumen en una línea

**El modo audio-only funciona sin GSR** (ffmpeg + PipeWire, opus y flac,
verificado con ffprobe) y **la detección baja de 1,3 s a 0,7-0,9 s** con las
sondas en paralelo: por debajo del segundo que exige CLAUDE.md.

## Hecho y verificado

Cada línea con su comando, ejecutado el 2026-09-15 en la máquina de
desarrollo.

| Entregable | Cómo se comprobó |
|---|---|
| Backend audio-only (`src/core/audio.cpp`) | grabaciones reales: **opus del audio del sistema (3,59 s)** y **flac del micrófono**, ffprobe conforme |
| Orden `capturia audio` | ejecutada; `--dispositivo`, `--formato`, `--salida`, defaults incluidos |
| `capturia parar` universal | para la pantalla por IPC o el audio por SIGINT, lo que esté en marcha |
| `capturia estado` con las dos sesiones | «grabando», «grabando audio», «grabando pantalla y audio», «sin grabacion» |
| `capturia dispositivos` sin GSR | cae a `pactl list short sources`: el audio-only no depende de GSR |
| Parada limpia de ffmpeg | SIGINT y espera a que cierre la cola del contenedor; ffprobe lee el fichero entero |
| Sondas en paralelo (`volcar()`) | `--check`: **0,68-0,90 s** en 4 medidas, antes 1,26-1,44 s. `/usr/bin/time` |
| Arnés con audio | `verify-recording.sh` graba pantalla (2,95 s) **y** audio (2,47 s, opus) y verifica ambos. En verde |
| Todo el build | sin un warning; `ctest`: **11/11** |

Totales de test: version 22, proceso 17, capacidades 111, entorno 38,
json_ipc 20, ajustes 37, grabacion 21, audio 15, más los 3 del CLI.

## Decisiones tomadas en esta tanda

1. **Formatos del audio-only: opus y flac. MP3 no.** Opus cubre el caso
   general y flac el de calidad; los dos salen del ffmpeg del sistema. MP3
   obligaría a depender de otro codificador y no hay un caso que opus no
   cubra. Si algún día alguien lo pide con una razón real, se reabre.
2. **pactl como herramienta de apoyo del audio-only.** Es la interfaz pulse
   de PipeWire (paquete pipewire-pulse, presente en cualquier escritorio
   PipeWire) y es la misma API que GSR usa por dentro. Solo se usa para
   resolver «default_output»/«default_input» y para listar fuentes sin GSR;
   un nombre explícito de fuente funciona sin pactl. Proceso externo, como
   todo aquí.
3. **El audio se para con SIGINT, no por IPC.** ffmpeg no tiene IPC; SIGINT
   es su parada limpia documentada (escribe la cola del contenedor y sale).
   Se espera a que el proceso muera antes de dar la ruta, o el fichero
   podría estar a medio cerrar.
4. **Sesiones separadas**: `~/.cache/capturia/sesion` (pantalla, socket IPC)
   y `~/.cache/capturia/sesion-audio` (ffmpeg, fichero pid). Protocolos
   distintos, cerrojos distintos. Pueden convivir.
5. **El arranque se arregló con paralelismo, no con caché.** Un caché de
   capacidades habría mentido: el volcado depende del estado de la sesión
   (pantalla apagada = capacidades distintas, ESTADO.md de la Tanda 2). Las
   sondas son procesos independientes; en paralelo el coste es el de la más
   lenta. Medido: 0,68-0,90 s. Si una máquina más lenta vuelve a superar el
   segundo, lo siguiente es sondear en segundo plano desde la UI, no cachear.
6. **El audio por defecto va a la carpeta Música** (`XDG_MUSIC_DIR`), no a
   Vídeos: una nota de voz en Vídeos despistaría. Mismo mecanismo de
   `user-dirs.dirs`, con el home como último recurso.

## Sin hacer

| Qué | Por qué |
|---|---|
| Verificar portal, ventana y `focused` | siguen modeladas y **sin verificar**: el portal exige un diálogo interactivo |
| Documento de post-proceso | **Tanda 6, lo siguiente.** Requiere lectura del titular antes de escribir código |
| UI (Tanda 7) y empaquetado (Tanda 8) | por orden; la 8 depende de la licencia |
| Replay buffer | sin decidir; se decidirá con la UI delante |
| Licencia de Capturia | **decisión del titular**, sigue pendiente |

## Bloqueos

Ninguno abierto.

## Dependencias

- **ffmpeg** pasa de "dependencia del futuro audio-only" a dependencia real
  en uso. Proceso externo, jamás enlazado.
- **pactl** entra como apoyo opcional del audio-only (decisión 2 de arriba).
  Sin él, los nombres cómodos no se resuelven pero uno explícito funciona.
- `libcapturia` sigue en C++20 y POSIX, ahora con `std::thread` de la
  biblioteca estándar para las sondas.

## Lo primero de la Tanda 6

Escribir `docs/post-proceso.md`: qué puede ser el post-proceso de Capturia y
qué no. La pregunta dura es la telemetría del puntero para un auto-zoom: GSR
no la exporta (`-cursor` solo decide si lo dibuja) y en Wayland capturarla en
paralelo no es trivial (¿KWin? ¿libinput? ¿portal?). Investigación con citas,
"sin verificar" donde no se llegue, y **parada obligatoria**: el titular lo
lee antes de que exista una línea de código de post-proceso.
