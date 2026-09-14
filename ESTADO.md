# Estado del proyecto

Fecha: 2026-09-15. Última tanda ejecutada: **Tanda 4, completa.**

## Resumen en una línea

**Capturia graba.** `capturia grabar` y `capturia parar` funcionan de punta a
punta sobre el IPC de GSR, y `scripts/verify-recording.sh` graba de verdad y
verifica con ffprobe, en verde.

## Hecho y verificado

Cada línea con su comando, ejecutado el 2026-09-15 en la máquina de
desarrollo (KDE Plasma, Wayland, GSR 6.0.0 flatpak).

| Entregable | Cómo se comprobó |
|---|---|
| Parser de `--info` (`InfoGsr`) | fixtures + volcado real regenerado. `prueba_capacidades`: 111 comprobaciones |
| JSON mínimo para el IPC (`json_ipc`) | `prueba_json_ipc`: 20 comprobaciones, con las respuestas literales de la transcripción real |
| Cliente IPC (`ConexionIpc`) | `prueba_grabacion` contra un servidor falso que habla el protocolo, y contra el GSR real al grabar |
| Modelo de grabación + validación contenedor/códec | `prueba_ajustes`: 37 comprobaciones |
| Constructor de argumentos de GSR | ídem, más la grabación real |
| Proceso desatendido (`lanzar_desatendido`) | grabaciones reales; GSR sobrevive a que el CLI vuelva |
| Órdenes `grabar`, `parar`, `pausar`, `reanudar`, `estado`, `fuentes`, `dispositivos`, `--check --volcado` | ejecutadas todas; códigos de salida 0/1/2 |
| **Grabación real: monitor** | eDP-1, h264+opus en mkv, 3,9 s, pausa en medio descontada. ffprobe |
| **Grabación real: región** | `--region 640x480+100+100` → ffprobe dice 640x480 exactos |
| **Grabación real: cámara** | `/dev/video0` → 1280x720 h264+opus. ffprobe |
| Arnés de grabación completo | `scripts/verify-recording.sh` **graba 3 s, exige vídeo+audio y duración ≥ 2 s, y borra la prueba. En verde** |
| Todo el build | `cmake --build build` sin un warning; `ctest`: **10/10** |

Totales de test: version 22, proceso 17, capacidades 111, entorno 38,
json_ipc 20, ajustes 37, grabacion 21, más los 3 del CLI.

## Decisiones tomadas en esta tanda

Con el código de GSR delante, no de memoria:

1. **"Mejor códec de hardware" se delega en `-k auto` de GSR.** Su criterio
   (`codec_select.c`, `select_appropriate_video_codec_automatically`) es
   h264 → hevc → av1, escalando solo por resolución: compatibilidad primero.
   Es el criterio probado del upstream. `mejor_codec_hardware()` copia ese
   orden para poder enseñarlo, y `h264_software` nunca cuenta como hardware
   (`commands.c:75-76`: sale solo porque existe libx264).
2. **JSON propio mínimo, sin dependencia.** Las peticiones son dos formas
   fijas y la respuesta tres campos (`ipc.c:247-268`). Una biblioteca de JSON
   para eso no se justifica; `json_ipc.cpp` cubre el protocolo y se niega
   ante todo lo demás, campos desconocidos incluidos.
3. **`--list-capture-options` ya no se sondea**: `--info` trae la sección
   `capture_options` idéntica (`commands.c:268-269`). El parser sigue
   entendiendo los volcados viejos que la traen. Medido con honestidad: el
   ahorro quedó dentro del ruido (1,29-1,37 s frente a 1,26-1,44 s), porque
   el coste dominante es arrancar el flatpak, no una sonda más. Se queda
   igualmente: menos trabajo y la misma información. El arranque de verdad se
   ataca en la Tanda 5.
4. **Persistencia de ajustes: todavía no.** Recordar la última elección es
   cosa de la UI (Tanda 7); un fichero de configuración que hoy nadie
   escribiría sería código muerto y una opción más que mantener. El CLI se
   gobierna por flags con defaults sensatos.
5. **Una grabación a la vez.** La sesión vive en `~/.cache/capturia/sesion`
   (jamás en `/tmp`: la trampa del flatpak) y el socket fijo hace de cerrojo:
   si responde, ya se está grabando.
6. **Defaults**: mkv (CLAUDE.md), opus (`args_parser.c:262`), 60 fps
   (`args_parser.c:395`), `very_high`, `default_output`, carpeta Vídeos de
   `user-dirs.dirs` con el home como último recurso. Grabar sin audio exige
   `--sin-audio` explícito.
7. **Con `--fuente portal` se pasa `-restore-portal-session yes`**: sin eso,
   diálogo de permiso en cada grabación.

## Sin hacer

| Qué | Por qué |
|---|---|
| Verificar portal, ventana y `focused` | El portal abre un diálogo interactivo que un script no puede aceptar. Están modeladas y `argumentos_gsr` las construye, pero **sin verificar**. Se prueban a mano cuando toquen |
| Backend audio-only (ffmpeg+PipeWire) y `capturia audio` | Tanda 5 |
| Arranque < 1 s | Sigue en 1,3 s por las sondas del flatpak. Tanda 5: sondeo perezoso o caché con invalidación |
| Replay buffer (`-r`) | Sin decidir si Capturia lo expone. Choca con "solo lo imprescindible"; se decidirá con la UI delante |
| Documento de post-proceso | Tanda 6, y requiere lectura del titular antes de escribir código |
| Licencia de Capturia | **Decisión del titular**, sigue pendiente |

## Bloqueos

Ninguno abierto. B1, B2 y B3 cerrados.

## Dependencias

Sin cambios: C++20 y POSIX pelados. ffmpeg sigue siendo dependencia externa
(proceso, nunca enlazado) del futuro audio-only; ffprobe, del arnés.

## Lo primero de la Tanda 5

1. Backend audio-only: enumerar fuentes PipeWire sin GSR, orden
   `capturia audio`, parada limpia, ffprobe.
2. Decidir formatos del audio-only (propuesta: opus y flac; mp3 solo con una
   razón real) y dejarlo escrito aquí.
3. El arranque: medir qué sonda cuesta qué, y caché de capacidades con
   invalidación honesta (el volcado depende del estado de la sesión: pantalla
   apagada = capacidades distintas).
