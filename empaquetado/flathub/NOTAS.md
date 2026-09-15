# Flathub: lo que falta para poder enviar

Estado a 2026-09-15. El manifiesto de al lado es un borrador honesto: refleja lo
que la aplicación hace hoy, y por eso mismo todavía no se puede enviar.

App-id: **`es.solucionesconscientes.EasyScreenRecorder`** (antes
`org.capturia.Capturia`).

## Bloqueo 1 (resuelto): la licencia

Era `project_license: proprietary` porque el titular no había elegido. **Ya está
decidido: GPL-3.0-or-later**, con `metadata_license: CC0-1.0` en el metainfo, que
es lo que pide AppStream. Ver `docs/LICENSING.md`, sección «Decisión».

## Bloqueo 2 (sigue): lanzar GSR desde dentro del sandbox, verificado

La aplicación empaquetada vive en un sandbox y `gpu-screen-recorder` vive en el
anfitrión (nativo o como su propio flatpak). Hoy `localizar_gsr()` busca en
`PATH` y en `/var/lib/flatpak`, y ninguna de las dos cosas existe dentro de
nuestro sandbox.

La vía es `flatpak-spawn --host` (de ahí el `--talk-name=org.freedesktop.Flatpak`),
con `FLATPAK_ID` como señal de estar dentro. Eso es código en `entorno.cpp` que
**no se escribe hasta poder ejecutarlo de verdad**, porque:

- exige probar las dos variantes del anfitrión (GSR en `PATH` y GSR flatpak);
- el socket IPC en `~/.cache/easy-screen-recorder` tiene que cruzar dos
  sandboxes;
- y hace falta construir el flatpak para probarlo.

`flatpak-builder` **ya está instalado** (2026-09-15), así que el impedimento
que quedaba es bajar el SDK de KDE y dedicarle la construcción. Cuando se haga:
se escribe, se construye local y se verifica grabando.

## Lo que dice el linter de Flathub, hoy

```
$ flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
    manifest empaquetado/flathub/es.solucionesconscientes.EasyScreenRecorder.yml
errors  : ['finish-args-flatpak-spawn-access']
warnings: []
```

**Los avisos están a cero.** El runtime estaba en `6.9`, que está EOL y que el
linter avisaba de que pasaría a error; se subió a `6.11`. **Ese cambio está sin
verificar**: comprobarlo exige bajar el SDK (~2 GB) y construir, y no se ha hecho
desde aquí. Es lo primero que hay que confirmar en la primera construcción real.

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
| Capturas de pantalla en URL pública | **Falta.** Las tres están ya referenciadas en el metainfo; ver `docs/screenshots/README.md` |
| `<releases>` con la versión | **Hecho**: 0.1.0, tipo `development` |
| Repositorio público con tag | **Falta.** El manifiesto de envío cambia `type: dir` por `type: git` + tag + commit |
| Verificación de dominio | **Falta.** Hay que publicar `https://solucionesconscientes.es/.well-known/org.flathub.VerifiedApps.txt` con el app-id dentro, para que la ficha salga como verificada |
| Justificar `flatpak-spawn` | **Redactado arriba**, listo para pegar en el envío |
| Construir y grabar dentro del sandbox | **Falta**, y es el bloqueo 2 |

## El .deb (sí X11)

Verificado 2026-09-15: la UI corre como cliente X11 (`QT_QPA_PLATFORM=xcb` sobre
XWayland) y graba igual; `libesr` no toca el display. Una sesión X11 pura no se
ha probado desde aquí (esta máquina corre Wayland), pero la captura es de GSR,
que soporta X11 upstream. Conclusión: el `.deb` es viable. Su preparación está
en `empaquetado/debian/`.
