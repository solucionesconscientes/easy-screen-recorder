# Estado del proyecto

Fecha: 2026-09-15. Última tanda ejecutada: **Tanda 10: carpetas con memoria, atajo global, X11 y el camino a Flathub.**

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
| Empaquetado distribuible (Tanda 8) | **bloqueado por la licencia**, que el titular aplazó. Las reglas de `install()` están escritas y comprobadas con un install de ensayo (los dos binarios y los tres ficheros de datos en sus rutas FHS) |
| Post-proceso | **pospuesto por decisión del titular** (2026-09-15); la investigación está en docs/post-proceso.md |
| Replay buffer | sin decidir; con la UI ya delante, es la siguiente conversación de alcance |
| Atajos globales | falta libkf6globalaccel-dev; una línea de apt el día que se quiera |
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

## Tanda 10: carpetas, atajo global y empaquetado

Lo pedido por el titular tras confirmar la región a mano. Verificado
ejecutando, el 2026-09-15:

| Qué | Cómo se comprobó |
|---|---|
| **Carpeta de destino elegible y con memoria** | "Guardar en" en Avanzado (vídeos y audio por separado, FolderDialog). Persistencia en `~/.config/capturia/capturia.conf`; módulo `configuracion` con 12 comprobaciones. Ciclo entero probado: carpeta elegida por conf → la autoprueba de la UI **y** el CLI grabaron allí |
| Defaults de carpeta | Vídeos para pantalla y Música (la carpeta XDG de audio) para solo-audio; el home como último recurso. Se crean si faltan |
| **Atajo global Meta+Shift+R** | registrado en `org.kde.kglobalaccel` **por DBus, sin dependencia nueva** (firmas comprobadas con busctl contra el servicio vivo). Verificado sin tocar el teclado: `invokeShortcut` arrancó una grabación real y la paró; el fichero quedó en Vídeos. Cambiable en Preferencias del sistema; fuera de KDE no se registra y no pasa nada |
| **X11** | la UI corre como cliente X11 (`QT_QPA_PLATFORM=xcb`, 1,39 s de primer frame vía XWayland) y graba igual. **Sesión X11 pura sin verificar** (esta máquina corre Wayland); la captura ahí es de GSR, que la soporta upstream. El .deb es viable |
| Camino a Flathub | borrador de manifiesto en `empaquetado/flathub/` con los dos bloqueos documentados en NOTAS.md: la licencia, y el lanzamiento de GSR desde dentro del sandbox (`flatpak-spawn --host`), que **no se codifica hasta poder ejecutarse** (no hay flatpak-builder en esta máquina, comprobado) |
| Prompt de licencia | `PROMPT-LICENCIA.md` en la raíz (fuera de git), con los hechos verificados de LICENSING.md y las preguntas que discriminan. Lo usa el titular en una sesión aparte |
| Regresión | ctest **12/12**, arnés pantalla+audio en verde |

Un episodio de esta tanda que vale como lección operativa: en la batería
final el arnés salió ROJO porque **la pantalla se había apagado sola** entre
dos pasadas, y sin plano de scanout GSR no lista el monitor (la trampa
documentada en la Tanda 2, mordiendo por primera vez en real). No era un
fallo del código: el arnés dijo la verdad. La cura: `kscreen-doctor --dpms
on` y repetir. Si el arnés se automatiza algún día (cron, CI local), esa
línea va delante.

## Tanda 9: lo que pidió el titular tras probarla

El titular confirmó a mano que la grabación va, y pidió tres cosas. Las tres
hechas y verificadas ejecutando, el 2026-09-15:

| Qué | Cómo se comprobó |
|---|---|
| **Región desde la UI** | selector propio a pantalla completa (arrastrar elige, Esc cancela), estilo Spectacle. El camino entero por autoprueba: `region 800x600+100+100` → ffprobe dice 800x600 exactos |
| **Formatos y códecs en Avanzado** | formato (mkv/mp4/webm), códec de vídeo (detectado de la máquina, «auto» delante), códec de audio **filtrado por formato**: una pareja que GSR cambiaría por detrás ni se puede pedir. `codecs_audio_para()` vive en libcapturia con test de coherencia contra `validar()` (53 comprobaciones) |
| **Solo-audio con formato** | opus/flac elegible; verificado grabando flac del micrófono por el camino de la UI. La carpeta por defecto (Música/Vídeos) se crea si falta: es la del usuario en XDG, no una inventada |
| webm con códec auto | salió **vp8+opus**, el criterio del upstream aplicado solo |
| La ventana se aparta al grabar pantalla | si no, salía dentro del vídeo. Vuelve sola al guardar; en solo-audio se queda con su reloj |
| La bandeja dice la verdad | punto rojo solo grabando; en reposo, icono neutro. Cerrar la ventana grabando la manda a la bandeja; sin nada en marcha, cierra del todo |
| La ventana crece con el Avanzado | antes cortaba las últimas filas |
| Regresión | ctest 11/11, arnés pantalla+audio en verde, arranque de la UI en ~0,75 s |

Nota sobre el «solo audio» del encargo del titular: el camino ya funcionaba
(la autoprueba grabó a Música a la primera). Lo que faltaba era poder elegir
formato y que la carpeta se cree si falta; si el titular vio un error
concreto, sigue interesando el texto exacto.

Sin verificar, y dicho: el arrastre humano del selector (autoprueba cubre el
camino con la región ya elegida; falta tu arrastre), multimonitor
(virtualX/Y escrito pero con un solo monitor aquí) y escala fraccionaria.

## Tanda 7: la UI

Qt6/QML + Kirigami sobre libcapturia, enlazada directamente (capa 3 sobre
capa 1; el CLI es un hermano, no un intermediario). Verificado ejecutando,
el 2026-09-15:

| Qué | Cómo se comprobó |
|---|---|
| Ventana de dos clics: Fuente, Grabar, "Avanzado" plegado | capturas de pantalla con spectacle; Breeze nativo |
| Detección en segundo plano | la ventana pinta ya y el combo se llena solo; ffmpeg pasa por QtConcurrent |
| **Arranque < 1 s** | **0,73-0,89 s hasta el primer frame pintado**, instrumentado en el binario (`CAPTURIA_MEDIR_ARRANQUE=1`), 4 medidas |
| RAM | 160-172 MiB de pico con la deteccion incluida; es el precio del runtime QML |
| **La UI graba por su camino real** | autoprueba `CAPTURIA_AUTOPRUEBA=eDP-1`: grabó 3,05 s (h264+opus, ffprobe) a la carpeta Vídeos por XDG, con parar por IPC en hilo aparte |
| Estado compartido con el CLI | el CLI grababa y la UI abierta a mitad enseñó "Grabando 00:07" con el reloj real (inicio.txt de la sesión) |
| Avanzado | calidad, fps y qué audio (sistema/micro/ambos en pistas separadas/sin audio). Nada más, a propósito |
| Bandeja y notificación | QSystemTrayIcon (StatusNotifierItem en Plasma); notificación con la ruta al guardar |
| `.desktop`, icono SVG propio y metainfo | desktop-file-validate y appstreamcli validate: 0 errores; reglas de `install()` escritas |
| Build limpio | configuración y compilación sin un aviso; la UI es opcional: sin Qt (el CI) se salta con un mensaje y el resto compila |

Decisiones de la tanda:

1. **La UI enlaza libcapturia, no llama al CLI.** Es lo que dibuja CLAUDE.md
   y evita parsear nuestra propia salida.
2. **Solo-audio como dos entradas más del selector de fuente** ("Solo audio:
   lo que suena" / "micrófono"): dos clics también para una nota de voz.
3. **Sin caché tampoco aquí**: la ventana pinta al instante y la detección
   (0,7-0,9 s) llega por detrás. Estado "detectando" honesto mientras tanto.
4. **Autoprueba dentro del binario** (`CAPTURIA_AUTOPRUEBA`): el clic no se
   puede automatizar sin inyección de entrada; esto verifica el camino real
   del controlador (hilos y señales) grabando de verdad. Es arnés, no feature.

Sin hacer de la tanda, y dicho:

- **Atajos globales**: exigen `libkf6globalaccel-dev`, que no está instalado.
  Una línea de apt el día que se quiera.
- **i18n**: las cadenas están preparadas con qsTr(); la extracción y carga de
  catálogos reales (KI18n) queda para cuando haya una segunda lengua.
- **El clic humano**: la autoprueba cubre el camino del controlador; un
  recorrido a mano del titular (clic en Grabar, pausa, parar, abrir carpeta)
  sigue pendiente y es bienvenido.

## Tanda 6: el post-proceso, investigado y ejecutado

`docs/post-proceso.md` responde la pregunta dura: **la telemetría del puntero
en Wayland/KDE se puede capturar, y las cuatro piezas están comprobadas
ejecutándolas** en esta máquina el 2026-09-15:

1. Un script de KWin lee `workspace.cursorPos` (salió `566,405`).
2. `QTimer` muestrea periódicamente (5 muestras a 99-106 ms).
3. `callDBus` saca cada muestra del compositor a un proceso nuestro
   (capturado con dbus-monitor delante).
4. GSR ancla la sincronización: `-write-first-frame-ts yes` escribe
   `<salida>.ts` con CLOCK_MONOTONIC y CLOCK_REALTIME del primer frame
   (`encoder.c:24-40`), pensado por el upstream justo para esto.

Sin verificar y dicho en el documento: el re-render con ffmpeg (la otra
mitad del trabajo), GNOME (iría por el portal con cursor_mode=metadata),
muestreo a 60 Hz y pantallas con escala fraccionaria.

**Decisión del titular (2026-09-15): alcance C, posponer.** Nada de
post-proceso hasta después de la UI. La investigación queda escrita y no
caduca. La licencia también se decidió aplazar: el repo sigue sin LICENSE
y la Tanda 8 (empaquetado y publicación) queda bloqueada por ella; el
README lo dice sin adornos.

## Lo primero de la Tanda 7 (UI)

1. El titular instala los paquetes dev de Qt6/KF6 (la línea está en
   ENCARGO.md y abajo).
2. `src/ui` entra en el build con extra-cmake-modules.
3. Ventana Kirigami: Fuente y Grabar, dos clics, "Avanzado" plegado.
4. Puente QML sobre libcapturia y medición de arranque y RAM con arnés.

Paquetes que faltan (comprobado con dpkg el 2026-09-14): qt6-base-dev,
qt6-declarative-dev, extra-cmake-modules, libkf6kirigami-dev,
kirigami-addons-dev, libkf6coreaddons-dev, libkf6i18n-dev.
