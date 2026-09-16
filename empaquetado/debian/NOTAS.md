<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

# El .deb: estado y lo que falta

Estado a 2026-09-15. **Los ficheros de empaquetado están escritos y el
contenido del paquete está verificado.** Falta una construcción completa con
`dpkg-buildpackage`, que necesita `debhelper` instalado.

El directorio de empaquetado vive en **`debian/` en la raíz**, que es donde lo
exige la herramienta de Debian y no se pelea con eso. Este documento es la nota
del proyecto; `debian/copyright` se movió allí con el resto.

| Fichero | Qué es |
|---|---|
| `debian/control` | Fuente y binario, con `${shlibs:Depends}` |
| `debian/rules` | `dh` con `--buildsystem=cmake+ninja` |
| `debian/changelog` | 0.1.0-1 |
| `debian/copyright` | DEP-5, `GPL-3+` |
| `debian/source/format` | `3.0 (native)`: somos el upstream, no hay tarball aparte |

## Lo verificado

Un `.deb` construido a mano con `dpkg-deb` a partir de `cmake --install`, para
comprobar el contenido sin esperar a `debhelper`:

```
$ dpkg --contents easy-screen-recorder_0.1.0-1_amd64.deb
/usr/bin/easy-screen-recorder
/usr/bin/easy-screen-recorder-cli
/usr/share/applications/es.solucionesconscientes.EasyScreenRecorder.desktop
/usr/share/metainfo/es.solucionesconscientes.EasyScreenRecorder.metainfo.xml
/usr/share/icons/hicolor/scalable/apps/es.solucionesconscientes.EasyScreenRecorder.svg
/usr/share/icons/hicolor/symbolic/apps/…-symbolic.svg
/usr/share/icons/hicolor/symbolic/apps/…-recording-symbolic.svg
```

Los dos binarios y los cinco ficheros de datos, cada uno en su sitio. El
binario del paquete responde: `easy-screen-recorder-cli 0.1.0`.

Y las dependencias no se escribieron a mano: las calculó `dpkg-shlibdeps`, que
es lo que hará `dh_shlibdeps` en la construcción real.

```
libc6 (>= 2.38), libgcc-s1 (>= 3.0), libqt6core6t64 (>= 6.10.2),
libqt6dbus6 (>= 6.4), libqt6gui6 (>= 6.4), libqt6qml6 (>= 6.10.2),
libqt6quick6 (>= 6.6.0), libqt6quickcontrols2-6 (>= 6.6.0),
libqt6widgets6 (>= 6.4), libstdc++6 (>= 13),
qt6-declarative-private-abi (= 6.10.2)
```

## El hallazgo que condiciona la distribución

Fíjate en el último: **`qt6-declarative-private-abi (= 6.10.2)`, con igual
exacto.** Sale de que el módulo de QML usa ABI privada de Qt Declarative, y
significa que **este `.deb` solo instala en máquinas con exactamente Qt
6.10.2.**

Consecuencia práctica: no hay un `.deb` único que sirva para Ubuntu 26.04 y
para Debian 13. **Hay que construir uno por versión de distribución.** Eso no
es un fallo del empaquetado, es cómo funciona el QML compilado, y es una razón
más para que Flathub sea la vía principal: ahí el runtime va con la aplicación.

Cuando haya que distribuir varios, el sitio natural es un PPA de Launchpad o un
repositorio propio con un `.deb` por serie, no un fichero suelto en la web.

## Decisiones tomadas, con su motivo

**`gpu-screen-recorder` va en `Recommends`, no en `Depends`.** Comprobado el
2026-09-15 en Ubuntu 26.04: `apt-cache policy gpu-screen-recorder` y
`apt-cache search gpu-screen` no devuelven nada. No está en los repositorios.
Un `Depends` sobre un paquete que no existe en ningún archivo deja el paquete
sin instalar. Con `Recommends`, se instala y apt lo propondrá el día que exista.

La instalación de GSR se documenta en el README y la dice `--check`. Las dos
vías reales:

| Vía | Comando |
|---|---|
| Flathub | `flatpak install flathub com.dec05eba.gpu_screen_recorder` |
| Desde fuente | `git.dec05eba.com/gpu-screen-recorder` |

**Instalar a nivel de sistema, no `--user`:** `localizar_gsr()` mira
`/var/lib/flatpak`, y un `--user` cae en `~/.local/share/flatpak`.

**`ESR_WERROR=OFF` solo en `debian/rules`.** En el repositorio los warnings son
errores, y así sigue en el CI y en la verificación local. Pero un paquete se
compila en la máquina de otro, con el compilador de esa distribución, y un gcc
más nuevo saca avisos que no existían. Con `-Werror`, ese aviso nuevo no es un
aviso: es un paquete que no compila para alguien que no puede arreglarlo. Debian
lo desaconseja explícitamente. La garantía no se pierde, se aplica donde hay
alguien que puede corregir.

**Sin estrofa de GSR en `copyright`.** El paquete no lo incluye ni lo
redistribuye. Si algún día se empaquetara, habría que añadirla con `GPL-3` —sin
el `+`, es *only*— y ofrecer su fuente exacta.

## La construcción completa: hecha

`dpkg-buildpackage -us -uc -b` el 2026-09-16. Resultado:

| | |
|---|---|
| `easy-screen-recorder_0.1.0-1_amd64.deb` | **210 KB** |
| `easy-screen-recorder-dbgsym_0.1.0-1_amd64.ddeb` | 3,1 MB |

Los 210 KB frente a los 4,6 MB del `.deb` que se hizo a mano confirman lo que
estaba anotado: `dh_strip` quita la información de depuración y **genera el
paquete `-dbgsym` por su cuenta**. Los binarios salen `stripped`, comprobado
extrayéndolos del paquete.

### `lintian`: cero errores

```
$ lintian easy-screen-recorder_0.1.0-1_amd64.deb
W: initial-upload-closes-no-bugs
W: no-manual-page [usr/bin/easy-screen-recorder-cli]
W: no-manual-page [usr/bin/easy-screen-recorder]
```

**Y una predicción que falló.** Este documento decía que el `Recommends` sobre
un paquete que no está en el archivo «probablemente dé aviso». **No lo dio.**
`lintian` no comprueba los `Recommends` contra el archivo, así que la decisión
de usar `Recommends` en vez de `Depends` no cuesta ni un aviso.

- `initial-upload-closes-no-bugs`: **silenciado con un override**, en
  `debian/easy-screen-recorder.lintian-overrides` y con el motivo dentro. Pide
  que el changelog cierre un bug de ITP, que es el trámite de entrar en Debian
  y aquí no existe. Si algún día se envía a Debian de verdad, se quita esa línea
  y se abre el ITP.
- `no-manual-page` ×2: **pendiente, y a propósito.** Las páginas de manual
  documentan las opciones del CLI, y esas opciones cambian con la rama
  `audio-formatos` —cinco formatos de audio en vez de dos, y `--bitrate`—.
  Escribirlas aquí garantizaría que nacen desfasadas, así que van en esa rama,
  instaladas por el CMake para que las tenga cualquier empaquetado y también
  quien compile desde el código.

### Lo que sigue sin comprobarse

**La instalación.** `sudo dpkg -i` necesita privilegios que esta sesión no
tiene. El contenido, las dependencias y `lintian` sí están verificados; lo que
falta es meterlo en un sistema y arrancarlo desde `/usr/bin`.

Y un detalle del entorno de esta máquina: `debhelper` y `lintian` estaban en
estado `iU` —desempaquetados y **sin configurar**—, así que `/usr/bin/dh`
existía pero `dpkg-checkbuilddeps` fallaba porque un `debhelper` sin configurar
no provee el virtual `debhelper-compat`. La construcción de arriba se hizo con
`-d` para saltar esa comprobación. Se arregla con:

```bash
sudo dpkg --configure -a
```

## Lo que falta## Lo que falta

1. **Las dos páginas de manual**, que llegan con la rama `audio-formatos`.
2. **Probar la instalación** en un sistema: `sudo dpkg -i` y arrancar desde
   `/usr/bin`.
3. **Un `.deb` por serie de distribución**, por el pin exacto de
   `qt6-declarative-private-abi`. Cuando haya varios, un PPA de Launchpad o un
   repositorio propio, no ficheros sueltos en la web.

## Sobre X11

Verificado 2026-09-15: la UI corre como cliente X11 (`QT_QPA_PLATFORM=xcb`
sobre XWayland) y graba igual; `libesr` no toca el display. Una sesión X11 pura
no se ha probado desde aquí —esta máquina corre Wayland—, pero la captura es de
GSR, que soporta X11 upstream. Conclusión: el `.deb` es viable para usuarios de
X11 y de Wayland.
