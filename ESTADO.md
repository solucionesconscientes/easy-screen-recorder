# Estado del proyecto

Fecha: 2026-09-10. Última tanda ejecutada: **Tanda 2, completa salvo la
verificación con CMake.**

## Resumen en una línea

**El bloqueo B1 está cerrado**: los tres entregables de investigación sobre GSR
están hechos con el código y los binarios delante, y el protocolo IPC no solo
está documentado sino ejecutado de punta a punta.

## Hecho y verificado

Cada línea se ha comprobado ejecutando un comando.

| Entregable | Estado | Cómo se comprobó |
|---|---|---|
| 1. Volcado real `docs/gsr-capabilities.txt` | hecho | `scripts/volcar-capacidades.sh` en esta máquina. Las **seis sondas salen con código 0**: ninguna falló |
| 2. Parser contra la realidad | hecho | tres fallos reales corregidos, ver abajo. `prueba_capacidades`: 92 comprobaciones, 0 fallos |
| 3. `docs/gsr-ipc.md` | hecho | leído de GSR 6.0.0 con citas `fichero:línea`, y **ejecutado**: transcripción real en el documento |
| 4. `docs/gsr-audio-only.md` | hecho | las tres preguntas respondidas. La (a) ejecutando GSR |
| 5. Versión mínima de GSR | hecho | fijada en **6.0.0**. `version_minima_gsr()` ya no devuelve `nullopt` |
| 6. `docs/LICENSING.md` | hecho | escrito, con las zonas grises y cuatro reglas accionables |
| `capturia --check` | hecho | dice **LISTO** en esta máquina, código de salida 0 |

### Las seis sondas: ninguna falló

Era la duda que dejó la Tanda 1, porque salían del encargo y no del código de
GSR. Están todas y todas responden con código 0:

| Sonda | Código | Qué devuelve |
|---|---|---|
| `--version` | 0 | `6.0.0` |
| `--info` | 0 | secciones `section=nombre` y líneas `clave|valor` |
| `--list-capture-options` | 0 | 11 fuentes |
| `--list-audio-devices` | 0 | 4 dispositivos |
| `--list-application-audio` | 0 | vacío, porque no sonaba nada |
| `gsr-cli --help` | 0 | el uso completo, con los comandos del IPC |

## Lo que se aprendió y no estaba previsto

Cuatro cosas que cambian decisiones, no curiosidades.

### 1. GSR está aquí como flatpak, no en PATH

`com.dec05eba.gpu_screen_recorder` 6.0.0. `capturia --check` lo daba por
ausente, que era un diagnóstico falso: GSR funciona perfectamente en esta
máquina.

Se añadió la detección del flatpak a `localizar_gsr()` en `src/core/entorno.cpp`
y la misma lógica al script de volcado. `--check` enseña ahora por qué vía se
encontró cada herramienta.

### 2. El `/tmp` del flatpak es privado

Comprobado: un fichero creado en el `/tmp` del sistema no se ve desde dentro del
sandbox. La primera prueba del IPC falló por esto durante un rato largo, porque
GSR creaba el socket **y el vídeo** dentro de su propio `/tmp`.

Consecuencia dura para la Fase 1: **con GSR en flatpak, el socket de `-ipc` y el
fichero de salida no pueden ir en `/tmp`.** Bajo el home del usuario sí. Está en
`docs/gsr-ipc.md`, sección "La trampa del flatpak".

### 3. Con la pantalla apagada, GSR miente sobre sus capacidades

Si el monitor está en DPMS off, `--info` y `--list-capture-options` no
encuentran ningún plano de scanout activo, así que **no listan el monitor y
dicen que solo hay `h264_software`**. Con la pantalla encendida aparecen el
monitor, `region`, y `h264`, `hevc` y `vp8` por hardware.

El primer volcado de esta tanda salió así de pobre y hubo que rehacerlo. No es
un fallo de GSR: `try_card_has_valid_plane` (`src/utils.c:408-438`) exige un
plano con framebuffer, y apagada no lo hay.

**Regla para el futuro: el volcado se hace con la pantalla encendida.** Y
Capturia no puede cachear un volcado como si fuera permanente, porque depende
del estado de la sesión.

### 4. La captura de monitor no funciona sin plano activo

Corolario del anterior. Con la pantalla apagada, `-w region` y `-w eDP-1` fallan
con "no /dev/dri/cardX device found" (`src/cli/main.c:615-619`). Queda `-w
portal`, que abre un diálogo de permiso.

## Los tres fallos del parser, corregidos

La heurística `id|detalle` era una suposición de la Tanda 1. El separador `|`
resultó ser **correcto**; lo que estaba mal era todo lo demás.

| Fallo | Qué pasaba | Corrección |
|---|---|---|
| Número de campos fijo | Una cámara imprime `/dev/video0\|1280x720@30hz\|mjpeg`: **tres campos**. El parser metía `1280x720@30hz\|mjpeg` entero en `detalle` | `Opcion::campos` es ahora un vector y se parte por todos los `\|` |
| Aviso falso por falta de separador | `--list-application-audio` imprime el nombre pelado, sin `\|` (`commands.c:296-300`). El parser avisaba de "formato inesperado" ante lo normal | El separador se exige por lista, no en general: `FormatoLista::exige_separador` |
| Aviso falso por lista vacía | Sin ninguna aplicación sonando, esa lista sale vacía con código 0. El parser lo llamaba fallo | `FormatoLista::vacio_normal` |

Y uno más que salió del volcado con la pantalla apagada: las líneas
`gsr error:` que GSR escribe por stderr acababan mezcladas en la lista y podían
llegar a la UI como una fuente de captura. Ahora se descartan.

Los fixtures sintéticos se quedan, como pedía el encargo. El que era "volcado
real" de la Tanda 1 pasó a `tests/fixtures/sin-nada-instalado.txt`, porque el
volcado real de ahora ya no cubre el caso "no hay nada instalado".

## Sin hacer

| Qué | Por qué |
|---|---|
| Verificación con `cmake --build build` y `ctest` | **cmake y ninja no están instalados en esta máquina.** Ver bloqueo B3 |
| Backend ffmpeg de audio-only | Ahora se sabe que hace falta, pero esta tanda era de investigación. No se empieza sin decirlo |
| Parsear `--info` | Su formato ya se conoce y está documentado, pero de ahí salen los códecs y eso es Fase 1 |
| Licencia de Capturia | Decisión del titular. Ver `docs/LICENSING.md` |
| `scripts/verify-recording.sh` | No se ejecutó: esta tanda no tocaba grabación como entregable |

## Bloqueos

### B1: acceso a GSR — **CERRADO**

Cerrado el 2026-09-10. El árbol de GSR 6.0.0 está en
`third_party/gpu-screen-recorder/` y los binarios responden. Los tres
entregables que dependían de él están hechos.

Nota sobre cómo se cerró, porque afecta a la reproducibilidad: el árbol **no
venía en el repo** (lo excluye `.gitignore`, como estaba previsto) ni estaba en
el disco. Se obtuvo del snapshot exacto con el que se construyó el flatpak
instalado, que sale de su propio manifiesto:

```
/var/lib/flatpak/app/com.dec05eba.gpu_screen_recorder/current/active/files/manifest.json
  -> https://dec05eba.com/snapshot/gpu-screen-recorder.git.r1467.d31b698.tar.gz
```

Es decir: el código leído es exactamente el del binario que se ejecutó. Si algún
día hay que rehacerlo, ese es el camino.

### B2: versión mínima de GSR — **CERRADO**

Fijada en 6.0.0, con el razonamiento en CLAUDE.md.

### B3: no hay CMake ni Ninja en la máquina (nuevo)

`cmake` y `ninja` no están instalados. Están en los repos de Ubuntu 26.04 pero
`sudo` pide contraseña, así que no se pueden instalar desde aquí.

**Qué se hizo mientras tanto:** compilar y pasar los tests a mano con `g++ 15`,
replicando exactamente los flags del `CMakeLists.txt`
(`-std=c++20 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror`).
Resultado: **compila sin un solo warning y los tres tests pasan.**

```
prueba_version:     21 comprobaciones, 0 fallos
prueba_proceso:     17 comprobaciones, 0 fallos
prueba_capacidades: 92 comprobaciones, 0 fallos
```

Eso es verificación real y ejecutada, pero **no es el comando que manda
CLAUDE.md**, así que la tanda no se da por verificada del todo. Falta:

```
sudo apt install -y cmake ninja-build
cmake -S . -B build -G Ninja && cmake --build build
ctest --test-dir build --output-on-failure
```

**Quién lo desbloquea:** el titular, con esa primera línea.

## Dependencias

Sin cambios en el código: `libcapturia` sigue usando solo la biblioteca estándar
de C++20 y POSIX. Los tests siguen sin framework.

Lo que sí cambia es el **estatus de ffmpeg**: pasa de "quizá" a dependencia real
del modo audio-only, porque GSR no graba audio sin vídeo. Sigue siendo proceso
externo, nunca enlazado. Justificación completa en `docs/gsr-audio-only.md`.

De herramientas del sistema, `libcapturia` usa ahora `flatpak` si está, solo
para localizar GSR. Si no está, no pasa nada: se busca en PATH y punto.

## Lo primero de la Tanda 3

1. Instalar cmake y ninja, y pasar la verificación oficial. Cerrar B3.
2. Decidir la licencia de Capturia (`docs/LICENSING.md`).
3. Decidir qué formatos ofrece el modo audio-only. Opus y FLAC salen gratis con
   ffmpeg; MP3 obliga a otro codificador y puede que no haga falta.
4. Ya con eso, empezar Fase 1: parsear `--info` para los códecs, y el cliente
   IPC de `libcapturia` siguiendo `docs/gsr-ipc.md`.
