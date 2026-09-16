# Cambios

Formato: una entrada por tanda, con lo verificado. Las fechas son de la
máquina de desarrollo.

## Sin publicar

### Distribución propia (2026-09-16)
- `scripts/publicar-flatpak.sh`: repositorio Flatpak propio y firmado, con
  `.flatpakref`, `.flatpakrepo`, bundle y una página de instalación. Las
  actualizaciones llegan con `flatpak update`.
- **Verificado de punta a punta**: repositorio firmado (`summary.sig`),
  instalado desde el `.flatpakref` como lo haría un usuario, el remoto queda
  configurado, `flatpak update` opera y la app arranca.
- Flathub queda para más adelante, con el motivo y el texto de su política
  escritos en `empaquetado/flathub/NOTAS.md`.

### Diseño de pantalla + webcam (2026-09-16)
- `docs/ROADMAP.md` gana la Parte 2 con el diseño medido, **sin implementar**.
- Lo medido: dos grabaciones de GSR a la vez funcionan sin contención, y la
  composición con `ffmpeg` sale a **0,17× del tiempo grabado** mezclando en CPU
  y codificando en GPU, frente a 0,48× haciéndolo todo en CPU.
- `overlay_vaapi` **no funciona** en esta GPU: «Function not implemented»
  incluso en el caso mínimo. La composición entera en GPU no es una opción aquí.
- Se descarta el plugin de GSR, que sería la vía para superponer en vivo: se
  carga dentro de su proceso y rompería el modelo dual.
- Capturas de pantalla rehechas en inglés.

### Interfaz en inglés (2026-09-16)
- **La interfaz sigue el idioma del sistema**: castellano si el sistema está en
  castellano, inglés en cualquier otro caso. 45 cadenas traducidas, con el
  catálogo empotrado en el binario.
- Y el cambio que lo hizo posible: **el texto visible deja de ser el
  identificador.** El desplegable de fuentes pasa a `{texto, valor}`; antes el
  QML decidía el modo con `currentText.indexOf("Solo audio")` y el atajo global
  comparaba igual, así que traducir habría roto el formulario en silencio.
- Las fuentes especiales dejan de enseñar su nombre interno: `portal` pasa a
  «Preguntar al empezar», `region` a «Elegir una región arrastrando» y
  `focused` a «La ventana que tenga el foco».
- Sigue en castellano lo que viene del núcleo: ~47 mensajes de error de
  `libesr` y la línea de comandos. El porqué y la vía, en `docs/ROADMAP.md`.

### Alcance y recursos, medidos (2026-09-16)
- Los README dicen dónde funciona de verdad —cualquier escritorio, Wayland y
  X11— y qué es lo único nativo de KDE.
- **Cifras de consumo medidas**, no adjetivos: CLI 0,00 s y 4 MB; detección
  completa 0,6–1,2 s y 49 MB; interfaz 1,0–1,6 s y 94 MB. En un i5-6200U de
  2015 con gráficos integrados.
- Y el aviso que va con eso: si la GPU no tiene codificador por hardware, GSR
  cae a CPU y la ventaja desaparece. `--check` dice en qué caso estás.
- `ESR_MEDIR_ARRANQUE`: el renombrado había dejado esa variable con espacios
  dentro del nombre, y `medir-arranque.sh` no la ponía, así que medir la
  interfaz se colgaba para siempre.

### Empaquetado y alcance corregidos (2026-09-16)
- **El `.deb` declara los 14 módulos QML** que necesita en tiempo de ejecución,
  Kirigami incluido. `dpkg-shlibdeps` no los ve: un `import` de QML no es
  enlazado ELF. Sin ellos el paquete se instalaba y la interfaz no arrancaba.
- **Deja de decirse «para KDE Plasma»**: funciona en cualquier escritorio y
  sobre Wayland y X11. Lo único atado a KDE es el atajo global y el icono de
  bandeja, y los dos se degradan sin ruido.

### Páginas de manual (2026-09-16)
- `easy-screen-recorder(1)` y `easy-screen-recorder-cli(1)`, instaladas por el
  CMake en `share/man/man1`. Van con el proyecto y no con el empaquetado:
  documentan el programa, no cómo se empaqueta.

### Formatos de solo audio (2026-09-15)
- **Tres formatos nuevos**: AAC en `.m4a` (compatibilidad), WAV (sin pérdida,
  para editar) y MP3 (solo compatibilidad heredada). Con Opus y FLAC, cinco.
- **Calidad del audio** elegible (96/128/192 kbps) en los formatos con
  pérdida. En FLAC y WAV el control **se esconde**, no se deshabilita: ahí ese
  ajuste no existe.
- «Lo que suena» pasa a llamarse **«Audio del sistema»** en la interfaz y en la
  ayuda del CLI.
- `--bitrate` en el CLI.
- La tabla de formatos vive en un solo sitio, y de ella salen la lista de la
  interfaz, la extensión por defecto y el codificador de ffmpeg. Antes la
  extensión era un `formato == "flac" ? "flac" : "opus"` repetido en dos
  ficheros, que con cinco formatos habría dado el fichero mal nombrado.

### Flatpak funcionando (2026-09-15)
- **La aplicación empaquetada ya graba.** Dentro de un sandbox, GSR se lanza en
  el anfitrión con `flatpak-spawn --host`. Verificado grabando: 416 s por CLI y
  2,79 s por la UI, comprobados con `ffprobe` desde fuera del sandbox.
- Dentro de un sandbox no se busca en `PATH`, solo en el anfitrión, por las dos
  vías (binario nativo o flatpak de GSR). Fuera, el comportamiento no cambia.
- El manifiesto declara `--device=dri`: sin él la interfaz se dibujaba en
  software.
- Runtime `org.kde.Platform` 6.11 confirmado construyendo y ejecutando.

### Empaquetado Debian (2026-09-15)
- `debian/` con `control`, `rules`, `changelog`, `copyright` y
  `source/format`. Construcción con `dh --buildsystem=cmake+ninja`.
- `gpu-screen-recorder` va en `Recommends`, no en `Depends`: no está en los
  repositorios y un `Depends` dejaría el paquete sin instalar.
- Contenido del paquete verificado con un `.deb` hecho a mano: los dos
  binarios y los cinco ficheros de datos en su sitio.

### Renombrado (2026-09-15)
- **Proyecto renombrado desde Capturia a Easy Screen Recorder.** El nombre es
  siempre en inglés y sin abreviar, en todos los idiomas.
- Identificador de aplicación: `org.capturia.Capturia` pasa a
  `es.solucionesconscientes.EasyScreenRecorder`.
- Binarios: la UI es `easy-screen-recorder` (antes `capturia-ui`) y la consola
  `easy-screen-recorder-cli` (antes `capturia`).
- Biblioteca: `libcapturia` pasa a `libesr`, con las cabeceras en `include/esr/`
  y el namespace `esr`. El URI de QML pasa a `es.solucionesconscientes.esr`.
- Configuración en `~/.config/easy-screen-recorder/` y sesión en
  `~/.cache/easy-screen-recorder/`. **Sin migración**: no hay usuarios todavía.
- Sin cambios de funcionalidad.

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
- **Easy Screen Recorder graba.** `grabar`, `parar`, `pausar`, `reanudar`, `estado`,
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
