# Flathub: lo que falta para poder enviar

Estado a 2026-09-15. El manifiesto de al lado es un borrador honesto: refleja lo
que la aplicación hace hoy, y por eso mismo todavía no se puede enviar.

App-id: **`es.solucionesconscientes.EasyScreenRecorder`** (antes
`org.capturia.Capturia`).

## Bloqueo 1 (resuelto): la licencia

Era `project_license: proprietary` porque el titular no había elegido. **Ya está
decidido: GPL-3.0-or-later**, con `metadata_license: CC0-1.0` en el metainfo, que
es lo que pide AppStream. Ver `docs/LICENSING.md`, sección «Decisión».

## Bloqueo 2 (resuelto): lanzar GSR desde dentro del sandbox

**Resuelto y verificado el 15 de septiembre de 2026, grabando de verdad.**

La vía es `flatpak-spawn --host`, que habla con el portal
`org.freedesktop.Flatpak` —de ahí el `--talk-name`— y ejecuta en el anfitrión.
Está en `localizar_gsr()` (`src/core/entorno.cpp`), y encaja en la abstracción
que ya existía: una `Invocacion` es `programa` + `prefijo` + argumentos, así que
la vía del anfitrión solo añade un prefijo nuevo y nada de lo que hay debajo
cambió.

Dentro de un sandbox **no se mira PATH**. No es que falle: es que podría
acertar por accidente si algún día el runtime trae un binario con ese nombre, y
estaríamos grabando con otro programa sin saberlo. Se prueba el anfitrión y solo
el anfitrión, con dos rutas y en este orden:

| Orden | Sonda | `origen` que queda |
|---|---|---|
| 1 | `flatpak-spawn --host sh -c 'command -v gpu-screen-recorder'` | `anfitrion` |
| 2 | `flatpak-spawn --host flatpak info com.dec05eba.gpu_screen_recorder` | `anfitrion/flatpak` |

La sonda le pregunta al shell del anfitrión si el binario está en su `PATH`, no
lo ejecuta. Preguntárselo a `gpu-screen-recorder --version` habría confundido
«no está» con «está y no arranca», que son dos diagnósticos distintos y el
usuario necesita saber cuál le ha tocado.

Y se sondea **una sola vez** por proceso, cacheado en una estática local:
`localizar_gsr()` se llama desde cinco sondas en paralelo, más `detectar()`, más
cada grabación, y un salto al anfitrión cuesta más que un `exec` local porque
pasa por el portal.

### Lo verificado, con las cifras

Construido con `flatpak-builder` contra `org.kde.Sdk//6.11` e instalado como
flatpak de usuario:

```
$ flatpak run --command=easy-screen-recorder-cli <app-id> --check
  gpu-screen-recorder   presente   6.0.0  com.dec05eba.gpu_screen_recorder (en el anfitrion)  [anfitrion/flatpak]
  gsr-cli               presente   com.dec05eba.gpu_screen_recorder (en el anfitrion)  [anfitrion/flatpak]
  fuentes de captura      11
  Resultado: LISTO
```

- **Grabación por CLI desde dentro:** 416 s de vídeo `h264` + audio `opus`,
  27 MB, comprobado con `ffprobe` **desde fuera del sandbox** — que es lo que
  prueba que el fichero salió de verdad y no se quedó dentro.
- **Grabación por la UI desde dentro**, por el mismo camino que el botón (el
  arnés `ESR_AUTOPRUEBA`): 2,79 s de `h264` + `opus` en `~/Vídeos/`, vía el
  permiso `xdg-videos`.
- **El socket IPC cruza los dos sandboxes.** Era la duda principal y no hubo
  que hacer nada: el `--filesystem=~/.cache/easy-screen-recorder:create` de
  nuestro lado y el `filesystems=host` del flatpak de GSR hacen que los dos
  vean la misma ruta real. `easy-screen-recorder-cli estado` desde dentro
  responde «grabando», y `parar` devuelve la ruta del fichero.

### Lo que hay que saber: `flatpak-spawn` es un proxy

No es un `exec`. Su proceso **se queda vivo dentro de nuestro sandbox** toda la
grabación, haciendo de puente.

Para la UI es invisible y correcto: la UI también vive mientras se graba. Pero
`easy-screen-recorder-cli grabar` promete volver al instante, y **dentro de un
flatpak no vuelve**, porque `flatpak run` no sale hasta que muere el último
proceso del sandbox y el puente es uno de ellos.

No se arregla: es cómo funciona el portal. Y no afecta al camino real del
producto, que es la UI. Si alguien quiere el CLI dentro del flatpak, tiene que
lanzarlo en segundo plano. Queda anotado en el código, donde se construye la
vía del anfitrión.

### Un arreglo que solo apareció al ejecutarlo

La primera ejecución de la UI dentro del sandbox escupió esto:

```
MESA-EGL: warning: failed to get driver name for fd -1
MESA: error: ZINK: failed to choose pdev
MESA-EGL: warning: egl: failed to create dri2 screen
```

Faltaba `--device=dri`, así que **la interfaz de QML se estaba dibujando en
software**. La captura no se veía afectada —la hace GSR en el anfitrión, con su
propio acceso— pero una interfaz que se dibuja en CPU, en una aplicación cuyo
argumento de venta es no gastar CPU, es absurda. Añadido al manifiesto; con él,
los avisos desaparecen.

Esto es exactamente lo que no se puede saber sin construir y ejecutar, y el
motivo por el que `CLAUDE.md` prohíbe escribir esta parte antes de poder
probarla.

## Lo que dice el linter de Flathub, hoy

```
$ flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
    manifest empaquetado/flathub/es.solucionesconscientes.EasyScreenRecorder.yml
errors  : ['finish-args-flatpak-spawn-access']
warnings: []
```

**Los avisos están a cero.** El runtime estaba en `6.9`, que está EOL y que el
linter avisaba de que pasaría a error; se subió a `6.11`. **Verificado el
2026-09-15**: con `org.kde.Sdk//6.11` el flatpak construye, se instala y graba.
Ya no es un cambio a ciegas.

El error que queda **no se arregla, se justifica**. `finish-args-flatpak-spawn-access`
es el permiso de hablar con `org.freedesktop.Flatpak`, y los revisores lo miran
con lupa, con razón: es acceso al anfitrión. Flathub lo trata como algo que
necesita **excepción**, no como un fallo del manifiesto. La justificación, tal
cual hay que escribirla en la descripción del envío:

> La captura la hace un proceso del anfitrión a propósito: `gpu-screen-recorder`
> (GPL-3.0-only), invocado por argumentos y controlado por un socket unix. No se
> enlaza ni se redistribuye. Sin acceso al anfitrión no hay captura por GPU, que
> es la única razón de ser de la aplicación.

Y el metainfo, con `appstreamcli`:

```
$ appstreamcli validate --no-net src/ui/datos/….metainfo.xml
✔ Se realizó la validación correctamente
```

Con red da 7 avisos, y **los 7 son del mismo tipo**: `url-not-reachable` y
`screenshot-image-not-found`, porque el repositorio todavía se llama `capturia`
y no tiene capturas. Se resuelven al renombrar el repositorio, empujar la rama y
añadir las imágenes. No hay ni un problema estructural.

## Lo que falta antes del envío

| Qué | Estado |
|---|---|
| Capturas de pantalla en URL pública | **Falta, y es OBLIGATORIO**: «All graphical applications must have one or more screenshots in the MetaInfo». Referenciadas al tag `v0.1.0`, no a `main`, porque Flathub exige tag o commit. Hay que tomarlas, commitearlas y MOVER el tag; ver `docs/screenshots/README.md` |
| `<releases>` con la versión | **Hecho**: 0.1.0, tipo `development` |
| Repositorio público con tag | **Hecho.** Público desde el 2026-09-16, tag `v0.1.0`, y el manifiesto ya usa `type: git` con tag y commit |
| Verificación de dominio | **Fichero escrito, sin desplegar.** Está en `websc`, en `public/.well-known/org.flathub.VerifiedApps.txt`. No bloquea el envío: la marca de verificada se resuelve cuando se despliegue el sitio |
| Justificar `flatpak-spawn` | **Redactado abajo**, listo para pegar en el envío |
| Construir y grabar dentro del sandbox | **Hecho y verificado** (bloqueo 2) |
| Runtime 6.11 | **Confirmado**: construye y ejecuta. Ya no está a ciegas |

## El .deb (sí X11)

Verificado 2026-09-15: la UI corre como cliente X11 (`QT_QPA_PLATFORM=xcb` sobre
XWayland) y graba igual; `libesr` no toca el display. Una sesión X11 pura no se
ha probado desde aquí (esta máquina corre Wayland), pero la captura es de GSR,
que soporta X11 upstream. Conclusión: el `.deb` es viable. Su preparación está
en `empaquetado/debian/`.
