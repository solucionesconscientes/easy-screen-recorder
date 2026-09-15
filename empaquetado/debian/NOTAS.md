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

## Lo que falta

**Una sola cosa bloquea la construcción completa:**

```bash
sudo apt install debhelper lintian
```

Con eso:

```bash
cd ~/Desktop/app-audio/capturia/capturia
dpkg-buildpackage -us -uc -b
lintian ../easy-screen-recorder_0.1.0-1_amd64.deb
```

`dpkg-buildpackage` falla hoy con `unmet build dependencies:
debhelper-compat (= 13)`, y eso es lo único que le falta: el resto de
`Build-Depends` —cmake, ninja-build, qt6-base-dev, qt6-declarative-dev— ya está.

Y después de esa construcción quedan dos cosas por mirar:

- **Los binarios sin `strip`.** El `.deb` de prueba pesa 4,6 MB porque lleva la
  información de depuración (`RelWithDebInfo`, `not stripped`: 12 MB la UI y
  6,4 MB el CLI). `dh_strip` lo arregla solo y además genera el paquete
  `-dbgsym`, así que esto se resuelve con la construcción real, no a mano.
- **Lo que diga `lintian`.** Hay un candidato conocido: un `Recommends` sobre
  un paquete que no está en el archivo probablemente genere un aviso. Está
  razonado arriba y se documenta como excepción; no se cambia por silenciar un
  aviso.

## Sobre X11

Verificado 2026-09-15: la UI corre como cliente X11 (`QT_QPA_PLATFORM=xcb`
sobre XWayland) y graba igual; `libesr` no toca el display. Una sesión X11 pura
no se ha probado desde aquí —esta máquina corre Wayland—, pero la captura es de
GSR, que soporta X11 upstream. Conclusión: el `.deb` es viable para usuarios de
X11 y de Wayland.
