# Post-proceso: qué puede ser y qué se ha comprobado

**Estado: INVESTIGADO, pendiente de decisión del titular.** No hay ni una
línea de código de post-proceso, y no la habrá hasta que el titular lea esto
y decida el alcance.

Máquina de la comprobación: Ubuntu 26.04, KDE Plasma sobre Wayland, KWin
Wayland, GSR 6.0.0 flatpak. Fecha: 2026-09-15. Todo lo marcado "ejecutado"
se corrió en esta máquina; lo demás dice "sin verificar".

## La pregunta que decide todo

CLAUDE.md vende Easy Screen Recorder contra Screen Studio y AutoZoom, y su gracia es el
**auto-zoom**: acercarse a donde pasa la acción. Para eso hace falta saber
**dónde estaba el puntero en cada instante** de la grabación.

GSR no lo da. Su `-cursor` solo decide si el cursor se dibuja o no
(`args_parser.c:269`), y no exporta posiciones por ningún sitio. Así que la
telemetría hay que capturarla nosotros, en paralelo, y en Wayland eso tenía
fama de imposible.

**Resultado de la investigación: se puede, y todas las piezas están
comprobadas ejecutándolas en esta máquina.**

## Las cuatro piezas, ejecutadas

### 1. KWin da la posición del puntero a un script

KWin (el compositor de Plasma) expone `workspace.cursorPos` a sus scripts, y
los scripts se cargan por DBus sin diálogo ni permiso especial:

```
$ qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript cursor.js easy-screen-recorder-prueba
$ qdbus6 org.kde.KWin /Scripting/Script0 org.kde.kwin.Script.run
journal: js: easy-screen-recorder-prueba cursorPos: 566,405
```

### 2. El muestreo periódico funciona

Un `QTimer` dentro del script de KWin, a 100 ms:

```
js: esr-muestra 1 t=1789423738816 pos=566,405
js: esr-muestra 2 t=1789423738920 pos=566,405
js: esr-muestra 3 t=1789423739025 pos=566,405
js: esr-muestra 4 t=1789423739131 pos=566,405
js: esr-muestra 5 t=1789423739230 pos=566,405
```

Intervalos reales de 99-106 ms. `Date.now()` da tiempo de pared en
milisegundos. **A 60 Hz (16 ms) no se ha probado**; para un zoom suave
tampoco hace falta: el zoom es una curva lenta y 20-30 muestras por segundo
sobran. Sin verificar, igualmente.

### 3. El script saca los datos por DBus a un proceso nuestro

`callDBus()` está disponible dentro de los scripts de KWin. Comprobado con
`dbus-monitor` delante:

```
method call sender=:1.16 -> destination=es.solucionesconscientes.esr.Telemetria
  path=/telemetria; interface=es.solucionesconscientes.esr.Telemetria; member=muestra
   string "566,405"
```

O sea: Easy Screen Recorder levanta un servicio DBus de sesión, el script de KWin le
manda `(t, x, y)` en cada tick, y Easy Screen Recorder lo escribe a un fichero de
telemetría junto al vídeo. El journal del paso 2 era solo para la prueba; el
transporte real es este.

### 4. GSR ya trae el ancla de sincronización

`-write-first-frame-ts yes` hace que GSR escriba `<salida>.ts` en cuanto
codifica el primer frame (`src/recorder/recorder.c:279-283`), con este
contenido (`src/encoder/encoder.c:24-40`):

```
monotonic_microsec	realtime_microsec
```

CLOCK_MONOTONIC y CLOCK_REALTIME del primer frame, en microsegundos. La
variable interna se llama `evdev_compatible_ts`: **el upstream la puso
exactamente para esto**, correlacionar eventos de entrada externos con el
vídeo. El instante de vídeo de una muestra es
`t_video = t_muestra_realtime - realtime_del_primer_frame`.

Nuestro `argumentos_gsr()` no pasa esta opción hoy; se añade el día que el
post-proceso exista.

## La arquitectura que sale de esto

```
easy-screen-recorder-cli grabar --con-telemetria
  ├── GSR graba, con -write-first-frame-ts yes    → video.mkv + video.mkv.ts
  ├── script KWin (lo carga y descarga easy-screen-recorder-cli)  → muestras (t, x, y) por DBus
  └── easy-screen-recorder-cli las recibe y escribe                → video.mkv.puntero (nuestro)

easy-screen-recorder-cli pulir video.mkv   (nombre por decidir)
  └── lee .ts y .puntero, calcula la curva de zoom, y re-renderiza
```

El re-render es la pieza **no investigada**: ffmpeg puede hacer crop+zoom
dinámico por expresiones o `sendcmd`, pero si eso alcanza calidad de
Screen Studio (curvas suaves, easing) está **sin verificar**. Es la mitad
del trabajo que queda y puede acabar pidiendo un renderizador propio sobre
ffmpeg como proceso.

## Los límites, sin adornos

- **Esto es KDE.** `workspace.cursorPos` es API de scripting de KWin. En
  GNOME y otros compositores no existe. La vía estándar allí sería el portal
  ScreenCast con `cursor_mode=metadata` (bit 4; en esta máquina
  `AvailableCursorModes` devuelve 7, o sea que está), pero consumir esa
  metadata exige abrir un stream PipeWire propio: segunda sesión de captura y
  segundo diálogo de permiso. **Sin verificar.** Easy Screen Recorder es KDE primero
  (CLAUDE.md), así que no bloquea, pero el auto-zoom nacería siendo solo-KDE
  y hay que decirlo en la UI.
- **La API de KWin puede cambiar.** `cursorPos` existe en la serie 6.x que
  hay aquí. Un cambio de KWin rompería la telemetría, no la grabación: el
  fallo queda contenido.
- **Pantallas con escala.** `cursorPos` da coordenadas lógicas; el vídeo va
  en píxeles físicos. Con escala 1 coinciden (caso de esta máquina); con
  escala fraccionaria hay que multiplicar y **no se ha probado**.
- **El re-render cuesta CPU.** Un portátil sin AV1 recodificando minutos de
  1080p tardará. El post-proceso es opcional por diseño: la grabación
  original nunca se toca.
- **GSR en modo portal ya pide el cursor como metadata** para pintarlo él
  (`src/capture/portal.c:205`). No lo exporta, y no lo vamos a tocar. Se
  cita para dejar claro que la telemetría no choca con GSR: son dos
  consumidores distintos.

## Qué NO es el post-proceso de Easy Screen Recorder

Para mantener "ligera e intuitiva" por delante:

- No es un editor de vídeo. Sin línea de tiempo, sin clips, sin transiciones.
- No es un pipeline de filtros configurable. Una operación, un botón.
- Nada de efectos que necesiten más telemetría de la descrita (clicks
  visuales exigirían capturar botones del ratón: otra investigación).

## La decisión del titular

Tres alcances posibles, de más a menos:

| Alcance | Qué incluye | Qué exige |
|---|---|---|
| A. Auto-zoom | telemetría KWin + `pulir` que re-renderiza con zoom | investigar el render con ffmpeg (la mitad no hecha) |
| B. Solo cortes | recortar principio/final y trocear, sin telemetría | solo ffmpeg, sin re-render completo (`-c copy`) |
| C. Posponer | nada hasta después de la UI | nada |

B es casi gratis y útil ya. A es lo que diferencia a Easy Screen Recorder de Kooha y
compañía, y esta investigación deja las piezas listas. A incluye B.

**Nada de esto se escribe hasta que el titular elija.**
