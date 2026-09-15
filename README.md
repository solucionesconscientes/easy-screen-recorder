# Easy Screen Recorder

Grabador de pantalla ligero para Linux, pensado para KDE Plasma sobre
Wayland. La captura la hace [gpu-screen-recorder][gsr] (GSR), 100 % en GPU;
Easy Screen Recorder pone la capa que le falta: una interfaz de dos pasos, modo de solo
audio y defaults sensatos.

[gsr]: https://git.dec05eba.com/gpu-screen-recorder/about/

## Qué hace hoy

Por línea de comandos, verificado con grabaciones reales:

```
easy-screen-recorder-cli grabar              # graba el monitor a Vídeos/easy-screen-recorder-FECHA.mkv
easy-screen-recorder-cli parar               # para, guarda e imprime la ruta
easy-screen-recorder-cli pausar / reanudar
easy-screen-recorder-cli audio               # solo audio (opus), sin GSR de por medio
easy-screen-recorder-cli fuentes             # qué puede grabar esta máquina
easy-screen-recorder-cli dispositivos        # qué audio hay
easy-screen-recorder-cli --check             # qué falta en esta máquina y por qué
```

Sin tocar nada: mkv, el mejor códec de hardware que tenga la GPU, el audio
del sistema y 60 fps. Todo lo demás son opciones (`easy-screen-recorder-cli --help`).

La interfaz gráfica (Qt6/Kirigami) está en desarrollo.

## Qué necesita

- **gpu-screen-recorder 6.0.0 o más nuevo**, nativo o como flatpak
  (`flatpak install com.dec05eba.gpu_screen_recorder`). Easy Screen Recorder lo
  encuentra por las dos vías.
- **ffmpeg y ffprobe** para el modo de solo audio y la verificación.
- PipeWire con `pipewire-pulse`, que es lo normal en cualquier escritorio
  actual.

`easy-screen-recorder-cli --check` dice exactamente qué falta y qué deja de funcionar por
ello, sin adornos.

## Compilar

```
cmake -S . -B build -G Ninja
cmake --build build            # -Werror: sin un solo warning
ctest --test-dir build --output-on-failure
./build/src/cli/easy-screen-recorder-cli --check
```

Sin dependencias de biblioteca: C++20 y POSIX. GSR y ffmpeg son procesos
externos, nunca enlazados; el porqué está en `docs/LICENSING.md`.

## Cómo está hecho

```
UI Qt6/QML + Kirigami         (en desarrollo)
  └── libesr             C++20, sin GUI, sin Qt
        ├── GSR por IPC       la pantalla; el mismo protocolo que gsr-cli
        └── ffmpeg + PipeWire el modo solo-audio
  └── CLI easy-screen-recorder-cli            todo lo de arriba, por consola
```

La regla de la casa: si algo no funciona por CLI, no se toca la UI. Y la
verificación es grabar de verdad y pasar ffprobe
(`scripts/verify-recording.sh`), no mirar que "parece" que va.

Los documentos técnicos viven en `docs/`: el protocolo IPC de GSR ejecutado
y transcrito, por qué el modo audio-only no puede ir por GSR, la
investigación del post-proceso y el razonamiento de licencias. El estado
real del proyecto, tanda a tanda, en `ESTADO.md`.

## Licencia

Sin decidir todavía. Hasta entonces este repositorio no concede permisos de
uso ni redistribución. GSR es GPL-3.0-only y Easy Screen Recorder lo usa como proceso
externo, sin enlazar ni redistribuir nada suyo (`docs/LICENSING.md`).
