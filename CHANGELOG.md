# Cambios

Formato: una entrada por tanda, con lo verificado. Las fechas son de la
máquina de desarrollo.

## Sin publicar

### Lo grabado sobrevive a un apagón, y la interfaz explica lo que hace (2026-09-16)

- **Un corte de luz se llevaba TODA la grabación.** Medido con un A/B limpio —10
  segundos grabados y un SIGKILL al grabador, que es lo más parecido a un apagón
  que se puede provocar a mano—: sin nada se recuperan **0 segundos**; con
  `flush_packets=1` se recuperan **8,1 de los 10**. Se pierde la cola sin cerrar,
  no la grabación entera. Ahora va siempre, y no cuesta nada: la misma grabación
  parada bien da la misma duración y el mismo tamaño con y sin la opción.
- **«Calidad» no son bitrates, y ahora lo dice.** Son valores de QP —calidad
  constante— de 35, 30, 25 y 22 (`video_codec.c:9-17` de GSR), así que el tamaño
  lo decide lo que pase en pantalla. Medido en esta máquina con el escritorio
  poco movido: **1,5 / 2,4 / 3,0 / 4,6 MB por minuto**. Va en una ayuda
  emergente, con la advertencia de que sube con el movimiento.
- **Ayudas emergentes** donde hacía falta explicar algo y no cabía: qué es el
  modo repetición, qué diferencia hay entre una pista y dos, qué hace superponer
  la cámara y por qué la vista previa se apaga al grabar, y qué significa
  codificar solo al cambiar la pantalla.
- **El botón «Cambiar…» estaba lejos de la ruta.** El hueco sobrante se estiraba
  entre los dos y el botón acababa pegado al borde derecho, a media ventana de lo
  que cambia. Ahora el hueco va después del botón.
- **Descoordinación al maximizar.** Abrir «Avanzado» con la ventana maximizada le
  daba un ancho y un alto propios y la dejaba ni maximizada ni del tamaño pedido.
  Ahora, si manda el gestor de ventanas, no se le toca el tamaño. Verificado:
  maximizada a 1366×686, abrir «Avanzado» la deja igual.
- `docs/interno/PROMPT-COMPARATIVA.md`: un prompt para comparar la aplicación con
  lo que ya existe en Windows, escrito para que el modelo busque contraejemplos y
  no para que dé la razón.

Verificado: build sin un warning, `ctest` 12/12 (109 comprobaciones en ajustes),
`verify-recording.sh` con los siete casos en verde, `reuse lint` conforme.

### La cámara se coloca arrastrándola, y «Avanzado» en dos columnas (2026-09-16)

- **Las cuatro esquinas se quedaban cortas.** Sobre una barra de tareas, un
  panel lateral o una ventana fija en una punta, las cuatro fallan a la vez y no
  hay una quinta. GSR admite **posición libre** en porcentaje (`x=50%;y=10%`,
  convertido a píxeles contra el tamaño del vídeo), verificado grabando, así que
  ahora se arrastra donde sea.
- **Y se ve dónde va a quedar antes de grabar**: un mapa con la pantalla a su
  proporción y la cámara dentro, también a la suya, que se mueve con el ratón.
  Al lado, la vista previa con tu cara. Las dos ayudas contestan preguntas
  distintas —cómo salgo y dónde salgo— y por eso están las dos.
- La proporción de la cámara sale de su mejor modo (`1280x720@30hz` → 16:9), no
  de una suposición: un 4:3 no se coloca igual que un 16:9.
- **«Avanzado» pasa a dos columnas y la ventana se ensancha al abrirlo.** Era
  una sola columna y las últimas filas quedaban fuera de la pantalla mientras
  sobraba la mitad derecha. Tres cambios, los tres medidos con la ventana
  delante:
  - Dos columnas en vez de una.
  - **Etiquetas al lado del control, no encima**: Kirigami elegía «encima», que
    dobla el alto de cada fila; con once filas eran casi 200 px de más.
  - Sin igualar el ancho de las etiquetas entre columnas: con «Códec de audio:»
    mandando, las de la izquierda se salían por el borde y «Imágenes por
    segundo» se leía «ágenes por segundo».
  - Resultado medido: de no caber en 768 px a **828×667**, con todo visible
    hasta «Guardar en».
- Por el CLI, `--camara-esquina` se cambia por `--camara-x` y `--camara-y`, en
  porcentaje. La validación rechaza una cámara que se saldría del borde, porque
  ahí GSR la recortaría sin decir nada.

Verificado: build sin un warning, `ctest` 12/12, `verify-recording.sh` con los
siete casos en verde, `reuse lint` conforme. Y grabando por el camino de la
interfaz con la cámara al 5 %, 5 %: aparece exactamente ahí.

### Nueve funciones, y una premisa que estaba mal (2026-09-16)

El hallazgo que ordena toda la tanda: **GSR ya compone pantalla y cámara en
vivo, él solo y en un proceso.** `-w` admite varias fuentes unidas por `|`,
igual que `-a`, y cada una acepta `x`, `y`, `width`, `height`, `halign`,
`valign`, `hflip` y `vflip` detrás de un `;`. Verificado grabando con el CLI del
proyecto **sin escribir una línea de código**.

Eso tira abajo la Parte 2 entera del ROADMAP —dos procesos, dos ficheros,
composición con ffmpeg, estado `componiendo`, 0,21× del tiempo grabado— que se
había diseñado y medido sobre la premisa contraria. El diseño viejo queda en el
documento como registro, con la lección escrita: se leyó el código de GSR para
ver cómo rodearlo y no se leyó entero el manual de la opción que ya se usaba.

Lo implementado:

- **Cámara superpuesta**, con tamaño y esquina elegidos **antes** de grabar. El
  tamaño va en porcentaje del ancho, no en píxeles: un valor fijo es un cuarto
  de pantalla en 1366 y un décimo en 4K. La altura no se pasa, para que GSR
  mantenga la proporción.
- **Espejo**, activado por defecto. Sin él uno se ve al revés de como se ve en
  un espejo y no se reconoce.
- **Vista previa de la cámara** para encuadrarte antes de empezar, con
  QtMultimedia. Se apaga al arrancar la grabación, y no por capricho: **una
  cámara V4L2 admite un solo cliente**, medido (`Device or resource busy`) y
  dicho por el propio manual de GSR. Verte *durante* la grabación no es posible
  con esta arquitectura.
- **Codificar solo cuando la pantalla cambie** (`-fm content`), que se ofrece
  **solo donde funciona**: en Wayland sobre un monitor GSR lo acepta, avisa por
  stderr y lo ignora. Otro caso de «pides una cosa y recibes otra» atajado
  antes de lanzar.
- **Límite de tamaño del vídeo** (`-s`): grabar en la resolución de la pantalla
  y entregar 1080p o 720p. Verificado: pedir 800x600 sobre 1366x768 da 800x450,
  escalado con su proporción.
- **Vúmetro del micrófono** antes de grabar, porque el fallo caro es descubrir
  al reproducir que estaba mudo. Con escala en decibelios, no lineal: el micro
  de la máquina de desarrollo da 0,004 de RMS y en escala lineal la barra no se
  movía.
- **Cuenta atrás de 3 segundos**, cancelable, antes de empezar.
- **Audio de una sola aplicación** (`app:nombre` de GSR), con las que están
  sonando en ese momento añadidas al selector de audio.
- **Modo repetición** (replay buffer): guarda en memoria los últimos 30 s, 1, 5
  o 15 minutos y no escribe nada hasta que se lo pides. Orden nueva
  `easy-screen-recorder-cli guardar`, botón propio y entrada en la bandeja. El
  atajo global **guarda** en vez de parar, que es lo que uno quiere del «se me
  ha escapado eso».

Tres cosas que costaron y conviene no repetir:

- **`find_package(Qt6 COMPONENTS Multimedia)` en una llamada aparte pone
  `Qt6_FOUND` a FALSE** si el componente falta. Con eso, la interfaz entera
  dejó de compilarse **en silencio** durante varios pasos, con el binario viejo
  todavía en `build/` y los `grep -i error` saliendo limpios. Va en
  `OPTIONAL_COMPONENTS`.
- **La marca de modo repetición en la sesión.** El socket no dice de qué modo es
  la grabación, así que una ventana abierta a mitad de un replay ofrecía «Parar
  y guardar», y eso no guarda nada: tirarías el buffer creyendo que lo salvabas.
- **`--device=all` en el flatpak**, y solo por la vista previa: `dri`, `input` y
  `usb` se probaron y ninguno expone `/dev/videoN`. La grabación no lo necesita,
  porque la cámara la captura GSR desde el anfitrión.

Verificación, toda ejecutando: compilación sin un warning, `ctest` **12/12**
(106 comprobaciones en `prueba_ajustes`), `verify-recording.sh` con **siete**
casos en verde —pantalla, pistas mezcladas, pistas separadas, cámara
superpuesta, repetición, webm y audio—, `reuse lint` conforme. El menú de la
bandeja y el volcado del buffer, probados por DBus contra el flatpak instalado.
La vista previa y el vúmetro, verificados dentro del flatpak construido con el
SDK: la aplicación abre `/dev/video0` y aparece como flujo de captura en
`pactl`, con niveles que coinciden con los que mide ffmpeg por su cuenta.

### Lo que se rompía al usarla de verdad (2026-09-16)

Cuatro cosas que salieron de instalar el flatpak y usarlo, no de leer el código.

- **Al grabar, la aplicación desaparecía.** La ventana se ocultaba para no salir
  en el vídeo, y la única vía de vuelta era el icono de la bandeja, que **en el
  flatpak no llegaba a existir**: el manifiesto no pedía
  `--talk-name=org.kde.StatusNotifierWatcher` y el registro fallaba en silencio,
  con una sola línea en stderr («KDE platform plugin is loaded but SNI
  unavailable»). Había que reabrir la aplicación desde el menú de inicio para
  poder pausar.
  - El permiso entra en el manifiesto. Verificado en los dos sentidos: sin él, el
    watcher no lista ningún item nuestro; con él, lo lista y el aviso desaparece.
  - Y la ventana **se minimiza en vez de ocultarse**, así que sigue en la barra
    de tareas aunque no haya bandeja. Medido: `visibility` pasa a `Minimized` y
    `visible` sigue en `true`, que es justo por lo que la comprobación anterior
    (`!visible`) nunca la habría traído de vuelta.
  - La bandeja gana **menú**: mostrar, pausa/reanudar, parar y guardar, salir.
    Pausar sin sacar la ventana, que es lo que hacía falta. Verificado por DBus
    sobre el flatpak instalado: se pausa, se reanuda y se para desde el menú, y
    el fichero queda escrito.
- **El desplegable de fuentes decía `eDP-1` y `/dev/video0`.** Ahora dice
  «Pantalla del portátil · 1366×768» y «Cámara · Integrated Webcam HD». El
  identificador de GSR no se toca: solo cambia el texto.
  - El nombre de la cámara sale de `/sys/class/video4linux/videoN/name`, que es
    donde lo pone el driver; la clase de pantalla, del prefijo del conector DRM.
    Si no se reconoce, «Pantalla» a secas: genérico es honesto, adivinar no.
  - Las fuentes se **agrupan y ordenan**: monitores, luego lo que hay que decidir
    al empezar, luego cámaras. GSR las listaba como le salían y la cámara caía
    entre «region» y «portal».
  - El identificador solo reaparece cuando hace falta para distinguir dos
    fuentes que se llamarían igual.
- **«Preguntar al empezar (lo elige el sistema)»** decía quién decide y no QUÉ
  se decide. Pasa a «Elegir ventana o pantalla al empezar». Y «La ventana que
  tenga el foco», a «La ventana activa».
- **El audio de las dos fuentes solo se podía grabar en pistas separadas**, y
  con dos pistas casi todos los reproductores suenan solo la primera: el
  micrófono parecía no haberse grabado. Se añade «Los dos, en una sola pista»,
  la sintaxis `a|b` de GSR, y va **delante** de la opción de pistas separadas,
  que queda etiquetada «(para editar)».
  - `scripts/verify-recording.sh` lo comprueba de aquí en adelante: graba de las
    dos maneras y exige **una** pista de audio en la mezclada y **dos** en la
    separada. En verde.
  - Mezclando, `flac` desaparece del selector de códec y la validación lo
    rechaza: GSR lo cambiaría a opus por detrás (`codec_select.c:186-191`).

Y de repasar qué ofrece el desplegable de códecs, tres más:

- **`h264_software` no grababa nunca.** Sale de `--info`, que lo imprime solo
  porque la máquina tiene libx264 (`commands.c:75-76` de GSR), pero **no es un
  valor de `-k`** (`args_parser.c:20-38`): el grabador muere al arrancar con
  «-k should either be 'auto', 'h264', …». Se quita del selector. La
  codificación por CPU en GSR es `-encoder cpu`, otra opción y otra
  conversación; queda apuntada, sin implementar, en ESTADO.md.
- **El códec de vídeo y el formato se elegían por separado, y tres de las
  parejas que la interfaz permitía no grababan NADA.** Medido grabando: `h264`
  en webm, `hevc` en webm y `vp8` en mp4 arrancan, mueren al escribir la
  cabecera («Only VP8 or VP9 or AV1 video … are supported for WebM») y no dejan
  fichero. Es el fallo más caro que puede tener esto, porque se descubre cuando
  ya has grabado.
  - El selector de códec pasa a filtrarse por el formato elegido, igual que ya
    hacía el de audio, y `validar()` rechaza la pareja antes de lanzar, así que
    el CLI también queda cubierto.
  - La tabla sale de meter un flujo de cada códec en cada contenedor con ffmpeg:
    webm solo admite vp8, vp9 y av1; mp4 admite todo menos vp8; mkv todo.
  - `auto` vale siempre: GSR mira el contenedor antes de elegir. Verificado —
    `auto` en un `.webm` da vp8, no h264.
  - `scripts/verify-recording.sh` graba ahora también en webm y exige que el
    vídeo sea de los que webm acepta.
- **«Automática (recomendada)» en la calidad del audio no era un nivel de
  calidad.** Medido: es el default del codificador, y coincide con una opción
  que ya estaba en la lista — 96 kbps en opus, 128 en aac. O sea, vaguedad y una
  entrada repetida. Se quita; queda **96 kbps · voz** preseleccionada, que es
  exactamente lo que entregaba antes en opus, así que nadie recibe algo distinto
  de lo de ayer. No se añaden escalones por encima de 192: el núcleo llega a 512
  y el CLI los acepta, pero opus es transparente bastante antes y quien quiere
  más no quiere más kbps, quiere flac, que ya está en el selector de Formato.

Y un quinto, encontrado al ir a tocar el control del audio:

- **El panel «Avanzado» no cabía en la ventana.** Crecía a 32 unidades de
  rejilla fijas y en una pantalla de 1366×768 las tres últimas filas —imágenes
  por segundo, carpeta y **audio**— quedaban fuera del borde inferior, sin barra
  de desplazamiento ni nada que insinuara que seguían ahí. Ahora la ventana se
  ajusta a lo que el formulario mide, hasta donde dé la pantalla. Medido: la
  ventana converge a 711 px con el contenido entero dentro.

Verificación: compilación sin un warning, `ctest` **12/12** (148 comprobaciones
en `prueba_capacidades` y 86 en `prueba_ajustes`), `scripts/verify-recording.sh`
en verde con las tres comprobaciones nuevas —pistas mezcladas, pistas separadas
y webm—, y `reuse lint` conforme.

Lo que esta máquina graba de verdad, medido grabando 3 s con cada códec y
pasando ffprobe el 2026-09-16 (Intel, `/dev/dri/card1`, Wayland, GSR 6.0.0):
**h264**, **hevc** y **vp8**. `vp9` falla y lo dice; `av1` **cae a h264 en
silencio**, con un aviso solo en el log — ninguno de los dos los lista `--info`
aquí, así que la interfaz no los ofrece.

### Distribución propia (2026-09-16)
- `scripts/publicar-flatpak.sh`: repositorio Flatpak propio y firmado, con
  `.flatpakref`, `.flatpakrepo`, bundle y una página de instalación. Las
  actualizaciones llegan con `flatpak update`.
- Se sirve desde `flatpak.solucionesconscientes.es` en Cloudflare Pages, que ya
  está en uso para el sitio. Medido: 148 ficheros y 8,1 MB por versión, con
  4,1 MiB el fichero mayor — los límites del plan gratuito son 20.000 ficheros
  y 25 MiB, así que sobra.
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
