# Flathub: lo que falta para poder enviar

Estado a 2026-09-15. El manifiesto de al lado es un borrador honesto: refleja
lo que la aplicacion hace hoy, y por eso mismo no se puede enviar todavia.

## Bloqueo 1: la licencia

Flathub exige `project_license` en el metainfo y hoy dice `proprietary`
porque el titular no ha elegido. Sin licencia no hay envio. El prompt para
decidirla esta en PROMPT-LICENCIA.md (raiz del repo, fuera de git).

## Bloqueo 2: lanzar GSR desde dentro del sandbox, verificado

Easy Screen Recorder empaquetada vive en un sandbox y gpu-screen-recorder vive en el
anfitrion (nativo o como su propio flatpak). Hoy `localizar_gsr()` busca en
PATH y en /var/lib/flatpak, y ninguna de las dos cosas existe dentro de
nuestro sandbox.

La via es `flatpak-spawn --host` (de ahi el `--talk-name=org.freedesktop.
Flatpak`), con `FLATPAK_ID` como señal de estar dentro. Eso es codigo en
`entorno.cpp` que NO se escribe hasta poder ejecutarlo de verdad, porque:

- exige probar las dos variantes del anfitrion (GSR en PATH y GSR flatpak);
- el socket IPC en ~/.cache/easy-screen-recorder tiene que cruzar dos sandboxes;
- y esta maquina no tiene flatpak-builder (comprobado 2026-09-15), asi que
  no hay donde construir y ejecutar la prueba.

Escribirlo sin poder ejecutarlo seria exactamente lo que CLAUDE.md prohibe.
Cuando haya flatpak-builder (`sudo apt install flatpak-builder` y el SDK de
KDE), se escribe, se construye local y se verifica grabando.

## Lo que Flathub va a pedir ademas

- Capturas de pantalla en el metainfo (`<screenshots>`), alojadas en URL
  publica.
- Un `<releases>` con la version.
- El repositorio publico accesible, con tag. El manifiesto de envio cambia
  `type: dir` por `type: git` + tag + commit.
- Revision del permiso `--talk-name=org.freedesktop.Flatpak`: los revisores
  lo miran con lupa, y con razon. La justificacion: la captura la hace un
  proceso del anfitrion a proposito (GSR, GPL-3.0, sin enlazar). Conviene
  escribirla en la descripcion del envio tal cual.

## El .deb (si X11)

Verificado 2026-09-15: la UI corre como cliente X11 (QT_QPA_PLATFORM=xcb
sobre XWayland) y graba igual; libesr no toca el display. Una sesion
X11 pura no se ha probado desde aqui (esta maquina corre Wayland), pero la
captura ahi es de GSR, que soporta X11 upstream. Conclusion: el .deb es
viable. Tambien espera a la licencia (debian/copyright) y a la primera
version etiquetada.
