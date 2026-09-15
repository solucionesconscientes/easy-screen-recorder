# El .deb: lo que falta y lo que ya se sabe

Estado a 2026-09-15. Todavía no se empaqueta; esto es la preparación.

## `copyright` ya está

`empaquetado/debian/copyright`, formato DEP-5, con `Files: *` y `Files: debian/*`
a nombre de Dalmau Romaní (Soluciones Conscientes) y licencia `GPL-3+`, con la
referencia a `/usr/share/common-licenses/GPL-3` que exige la política de Debian.

**Sin estrofa de `gpu-screen-recorder`**, y a propósito: el paquete no lo
incluye ni lo redistribuye. Si algún día se empaquetara, habría que añadir su
estrofa con `GPL-3` (sin el `+`: es *only*) y ofrecer su fuente exacta.

## `gpu-screen-recorder` NO está en los repositorios

Comprobado el 2026-09-15 en Ubuntu 26.04:

```
$ apt-cache policy gpu-screen-recorder
(sin salida)
$ apt-cache search gpu-screen
(sin resultados)
```

**Consecuencia para el empaquetado: no se puede usar `Depends`.** Un `Depends`
sobre un paquete que no existe en ningún repositorio hace el `.deb`
instalable-pero-roto, o directamente no instalable según el gestor.

Lo que se hará en su lugar:

- **`Recommends: gpu-screen-recorder`**, no `Depends`. Así el paquete se instala
  y, si algún día entra en los repositorios o el usuario añade uno de terceros,
  apt lo propone.
- Y **documentar la instalación manual** en el README y en el propio paquete.

Las dos vías reales hoy, por orden de facilidad:

| Vía | Comando | Nota |
|---|---|---|
| Flathub | `flatpak install flathub com.dec05eba.gpu_screen_recorder` | Lo que se usa en esta máquina. **Instalar a nivel de sistema, no `--user`**: `localizar_gsr()` mira `/var/lib/flatpak`, y un `--user` cae en `~/.local/share/flatpak`, donde no se busca |
| Desde fuente | `git.dec05eba.com/gpu-screen-recorder` | Necesita sus dependencias de compilación |

Verificado con el flatpak de sistema: `easy-screen-recorder-cli --check` lo
detecta como `6.0.0 [flatpak]` y `scripts/verify-recording.sh` graba y
comprueba con `ffprobe`.

## Lo que falta antes de construir el paquete

- La primera versión etiquetada. `debian/changelog` necesita una versión real,
  y hoy `version.cpp` dice `0.1.0` sin tag.
- `debian/control` con las dependencias exactas: Qt 6.4+ (Core, DBus, Gui, Qml,
  Quick, QuickControls2, Widgets, Concurrent) y CMake 3.25+ para compilar;
  `ffmpeg` en `Recommends` para el modo solo-audio.
- `debian/rules` con `dh` y el `cmake-ninja` que ya usa el proyecto.
- Decidir si el `.deb` instala los simbólicos en `symbolic/apps` como hace el
  `install()` del CMake, que es lo correcto, o si Debian prefiere otra ruta.

## Sobre X11

Verificado el 2026-09-15: la UI corre como cliente X11 (`QT_QPA_PLATFORM=xcb`
sobre XWayland) y graba igual; `libesr` no toca el display. Una sesión X11 pura
no se ha probado desde aquí —esta máquina corre Wayland—, pero la captura la
hace GSR, que soporta X11 upstream. Conclusión: el `.deb` es viable para
usuarios de X11 y de Wayland.
