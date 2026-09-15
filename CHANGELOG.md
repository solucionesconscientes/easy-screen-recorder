# Cambios

Formato: una entrada por tanda, con lo verificado. Las fechas son de la
máquina de desarrollo.

## Sin publicar

### Tandas 9 y 10 (2026-09-15)
- Selector de región propio (arrastrar elige, Esc cancela) y formatos y
  códecs en Avanzado, con el códec de audio filtrado por formato.
- Carpeta de destino elegible y con memoria (vídeos y audio por separado),
  compartida entre la UI y el CLI. Defaults: Vídeos y Música (XDG).
- Atajo global Meta+Shift+R para empezar/parar, registrado en KGlobalAccel
  por DBus y cambiable en Preferencias del sistema.
- La ventana se aparta al grabar pantalla y la bandeja solo enciende el
  punto rojo grabando.
- UI verificada como cliente X11 (xcb); borrador de manifiesto de Flathub
  con sus bloqueos documentados en empaquetado/flathub/NOTAS.md.

### Tanda 7 (2026-09-15)
- La interfaz gráfica: Qt6/QML + Kirigami, dos clics (Fuente, Grabar),
  "Avanzado" plegado con calidad, fps y qué audio. Arranque medido en
  0,73-0,89 s hasta el primer frame.
- Graba de verdad por su propio camino (verificado con ffprobe) y comparte
  estado con el CLI: abierta a mitad de una grabación enseña el reloj real.
- Bandeja con notificación al guardar, .desktop, icono y metainfo validados,
  y reglas de instalación comprobadas con un install de ensayo.

### Tanda 6 (2026-09-15)
- Investigación del post-proceso (`docs/post-proceso.md`): la telemetría del
  puntero para auto-zoom es viable en KDE y las cuatro piezas están
  ejecutadas. Decisión del titular: **pospuesto** hasta después de la UI.

### Tanda 5 (2026-09-15)
- Modo solo-audio real por ffmpeg + PipeWire: `capturia audio`, opus y flac,
  sin depender de GSR.
- Detección en paralelo: `--check` baja de 1,3 s a 0,68-0,90 s.
- El arnés verifica pantalla y audio grabando de verdad.

### Tanda 4 (2026-09-15)
- **Capturia graba.** `grabar`, `parar`, `pausar`, `reanudar`, `estado`,
  `fuentes`, `dispositivos`, `--check --volcado`.
- Cliente IPC propio del protocolo de GSR, con respuestas diferidas sin
  límite de tiempo.
- Validación contenedor+códec antes de lanzar: lo que GSR cambiaría por
  detrás aquí es un error a la cara.
- Verificado con ffprobe: monitor, región y cámara v4l2.

### Tanda 3 (2026-09-14)
- Verificación oficial con CMake y Ninja por primera vez; CI hermético.
- `verify-recording.sh` deja de ser verde por construcción.
- Tests para la detección de entorno y la sincronía de versión.

### Tandas 1 y 2 (2026-09-10)
- Núcleo de detección: qué sabe hacer esta máquina, sin inventar nada.
- Investigación de GSR 6.0.0 con el código delante: IPC documentado y
  ejecutado, audio-only imposible por GSR, licencias razonadas.
