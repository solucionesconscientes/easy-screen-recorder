# Estado del proyecto

Fecha: 2026-09-09. Última tanda ejecutada: **Tanda 1, parcial.**

## Resumen en una línea

El esqueleto compila, pasa los tests y `--check` diagnostica bien la máquina,
pero **los tres entregables de investigación sobre GSR están sin hacer** porque
en el entorno de esta tanda no había ni código de GSR ni binarios ni GPU.

## Hecho y verificado

Cada línea de aquí abajo se ha comprobado ejecutando un comando.

| Entregable | Estado | Cómo se comprobó |
|---|---|---|
| 1. `CLAUDE.md` | hecho | — |
| 2. `ESTADO.md` | hecho | este fichero |
| 6. Scaffold CMake+Ninja, `libcapturia` y CLI `capturia` | hecho | `cmake --build build`: 13 objetivos, cero warnings con `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror` |
| 6. `--version` y `--check` | hecho | ejecutados, salida abajo |
| 7. Tests del parser de capacidades | hecho | `ctest`: 6 de 6 en verde |
| 8. `scripts/verify-recording.sh` | hecho | ejecutado: sale 0 y explica qué falta |
| 9. Commit y push | hecho | rama `main` |

Extra no pedido pero necesario para el punto 5:
`scripts/volcar-capacidades.sh`, que genera el volcado en el formato que lee el
parser. Sin él, el fixture y la detección en runtime usarían formatos distintos.

Arranque medido: **3 ms** para `capturia --version`. El objetivo del proyecto es
por debajo de 1 s, así que sobra margen; conviene volver a medirlo cuando entre
Qt.

### Lo que `--check` detecta en esta máquina

```
gpu-screen-recorder   ausente    no se encuentra en PATH
gsr-cli               ausente    no se encuentra en PATH
ffmpeg                ausente    no se encuentra en PATH
ffprobe               ausente    no se encuentra en PATH
Resultado: NO LISTO            (código de salida 1)
```

Es el resultado correcto para este contenedor. En la máquina de desarrollo debe
dar otra cosa, y esa es la primera comprobación de la Tanda 2.

## Sin hacer

| Entregable | Por qué |
|---|---|
| 3. `docs/gsr-ipc.md`: protocolo IPC real | bloqueo B1. El fichero existe y explica el bloqueo y cómo desbloquearlo, pero no documenta el protocolo |
| 4. `docs/gsr-audio-only.md`: puede GSR grabar audio sin vídeo | bloqueo B1. El fichero existe con el método y el árbol de decisión, pero **sin la respuesta** |
| 5. `docs/gsr-capabilities.txt`: volcado real | hecho a medias. El volcado es real y honesto, pero de la máquina equivocada: registra seis `command not found` |
| `docs/LICENSING.md` | fuera de la Tanda 1. Se puede escribir sin GSR delante: candidato a lo primero de la Tanda 2 |

## Bloqueos

### B1: no hay acceso a GSR (bloquea 3, 4 y 5)

La Tanda 1 se ejecutó en un contenedor remoto, no en la máquina de desarrollo.
Comprobado, no supuesto:

- `~/Desktop/app-audio/capturia` no existe. El repo se clonó de GitHub.
- `third_party/gpu-screen-recorder/` no está en el remoto: lo excluye
  `.gitignore`, tal como estaba previsto.
- `gpu-screen-recorder` y `gsr-cli` no están instalados.
- `/dev/dri` no existe: sin GPU. `XDG_SESSION_TYPE`, `WAYLAND_DISPLAY` y
  `DISPLAY` vacíos: sin sesión gráfica.
- `git.dec05eba.com` y `repo.dec05eba.com` los bloquea la política de salida del
  proxy (403 al CONNECT). `github.com/dec05eba/gpu-screen-recorder` da 404.

Sin árbol local, sin binarios y sin upstream no queda ninguna fuente primaria.

**Quién lo desbloquea:** el titular, ejecutando la Tanda 2 en la máquina de
desarrollo, o dando acceso al código de GSR desde el entorno remoto.

**Qué se decidió mientras tanto:** no escribir los documentos de memoria. Un
documento que describe un protocolo inventado es peor que no tener documento,
porque el de la Tanda 3 se lo cree.

### B2: la versión mínima de GSR sigue sin fijar

Consecuencia de B1: hace falta `project.conf` y `gpu-screen-recorder --version`.
`capturia::version_minima_gsr()` devuelve `nullopt` y `--check` informa de la
versión detectada sin dar nada por fallado. El criterio para decidirla está en
CLAUDE.md.

## Qué está sin verificar en el código escrito

Conviene tenerlo a mano, porque son las costuras que hay que revisar en cuanto
haya un volcado real:

- **Las seis sondas de `sondas()`** (`--info`, `--list-capture-options`,
  `--list-audio-devices`, `--list-application-audio`) salen del encargo, no del
  código de GSR. Si alguna opción no existe, la sonda devolverá error y `--check`
  lo dirá; no se romperá nada, pero faltará información.
- **La heurística `id|detalle` de `interpretar_lista()`.** El separador `|` es
  una suposición. Cuando ninguna línea lo trae, el parser se queda con la línea
  entera y deja aviso, en vez de partirla por donde le parezca.
- **La salida de `--info` no se interpreta.** Se vuelca y ahí se queda: su
  formato no está verificado y de ahí saldrán los códecs disponibles.

El envoltorio del volcado (`### comando:` / `### codigo:` / `### fin`) sí es
nuestro y sí está verificado con tests, incluido el caso de volcado truncado.

## Dependencias

Ninguna nueva. `libcapturia` usa solo la biblioteca estándar de C++20 y POSIX
(`fork`, `execv`, `poll`, `waitpid`). Los tests no usan framework: `tests/comprobar.hpp`
son treinta líneas. ffmpeg y GSR se usan como **procesos externos**, nunca
enlazados, que es además lo que mantiene la GPL-3.0 de GSR fuera de nuestro
código.

## Lo primero de la Tanda 2

1. Clonar en la máquina de desarrollo y ejecutar `capturia --check`. Comprobar
   que detecta GSR y su versión de verdad.
2. `scripts/volcar-capacidades.sh` y revisar `interpretar_lista()` contra el
   volcado real. Corregir la heurística y ampliar los tests.
3. Responder la pregunta (a) de `docs/gsr-audio-only.md`. Condiciona si hay
   backend ffmpeg o no.
4. Documentar el IPC de verdad, con citas `fichero:línea`.
5. Fijar la versión mínima de GSR y quitar el `nullopt`.
