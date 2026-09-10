# ¿Puede GSR grabar audio sin vídeo?

**Estado: RESPONDIDO.** Bloqueo B1 cerrado.

**No. GSR exige una fuente de vídeo siempre.** Comprobado ejecutándolo, no
deducido. Eso deja el modo audio-only en manos de un backend propio con ffmpeg,
que es la tercera fila del árbol de decisión que ya estaba escrito.

Versión analizada: GSR 6.0.0, el árbol de `third_party/gpu-screen-recorder/`,
instalado como flatpak en la máquina de desarrollo. Comprobado el 2026-09-10.

## a) ¿Puede GSR grabar audio sin fuente de vídeo, hoy?

**No.**

```
$ gpu-screen-recorder -a default_output -ac opus -o /tmp/solo-audio.opus
gsr error: missing argument '-w'
usage: gpu-screen-recorder -w <window_id|monitor|focused|portal|region|v4l2_device_path> ...
```

Sale con código 1 y no crea ningún fichero. Da igual qué audio se le pida: se
niega antes de mirar nada más.

### Dos erratas del comando que traía el encargo

El encargo proponía esta prueba:

```
gpu-screen-recorder -a default_output -c opus -o /tmp/solo-audio.opus
```

Tiene dos fallos, y conviene dejarlos escritos para que nadie los repita:

1. **`-c` no es el códec de audio, es el contenedor.** El códec de audio es
   `-ac` (`gpu-screen-recorder --help`, y `-ac aac|opus|flac` en el uso). Con
   `-c opus` se le está pidiendo un contenedor llamado "opus".
2. **`.opus` no vale como extensión para el códec opus.** GSR solo acepta opus
   en `.mp4`, `.mkv`, `.webm`, `.ts` y `.whip`; con cualquier otra extensión
   cambia a AAC y avisa (`src/recorder/codec_select.c:168-174`).

La prueba se repitió con `-ac opus`, que es lo correcto, y el resultado es el
mismo: `missing argument '-w'`. O sea que la respuesta no depende de la errata.

## b) ¿Qué lo impide exactamente a nivel de código?

Una sola línea. `-w` está declarado **no opcional**:

`third_party/gpu-screen-recorder/src/args_parser.c:536`

```c
self->args[arg_index++] = (Arg){ .key = "-w", .optional = false, .list = false, .type = ARG_TYPE_STRING };
```

Y el bucle que valida los argumentos rechaza cualquier obligatorio sin valor,
que es de donde sale literalmente el mensaje del error:

`third_party/gpu-screen-recorder/src/args_parser.c:673-676`

```c
for(int i = 0; i < NUM_ARGS; ++i) {
    const Arg *arg = &self->args[i];
    if(!arg->optional && arg->num_values == 0) {
        gsr_log(GSR_LOG_LEVEL_ERROR, "missing argument '%s'", arg->key);
```

No es una comprobación semántica en mitad de la grabación ni un efecto lateral
del pipeline: es el parser de argumentos, lo primero que corre. Cambiarlo sería
tocar GSR, y eso está prohibido.

### El apaño que no vamos a hacer

Se podría pasar una fuente de vídeo mínima, grabar y tirar la pista de vídeo
después. Se descarta:

- Sigue encendiendo la captura y el codificador de vídeo. Para un modo cuya
  gracia es ser ligero, es justo lo contrario.
- Necesita una fuente válida. En Wayland eso significa portal, o sea un diálogo
  de compartir pantalla para grabar un audio. Absurdo de cara al usuario.
- Deja un post-proceso obligatorio para quitar lo que no debió grabarse.

## c) ¿Qué códecs de audio soporta de verdad y en qué contenedores?

**Tres, y los tres están activos en el paquete instalado:** AAC, Opus y FLAC.

El enum no tiene más (`include/defs.h:77-79`):

```c
GSR_AUDIO_CODEC_AAC,
GSR_AUDIO_CODEC_OPUS,
GSR_AUDIO_CODEC_FLAC,
```

Y ninguno es opcional de compilación. `meson_options.txt` tiene siete opciones
(`systemd`, `capabilities`, `nvidia_suspend_fix`, `portal`, `app_audio`,
`plugin_examples`, `ffmpeg_static`) y **ninguna toca los códecs de audio**. Lo
que sí es opcional es el audio por aplicación (`app_audio`), que aquí está
activo: `project.conf:13` define `GSR_APP_AUDIO` y `--info` responde
`supports_app_audio|yes`.

En el flatpak, ffmpeg va estático y su lista de codificadores se fija en
`extra/build_ffmpeg.sh:117`:

```
encoders=aac,flac,libx264,libopus,h264_nvenc,...
```

Los tres están. Comprobado además a la salida: la grabación de prueba con
`-ac opus` produjo un stream `opus` según ffprobe.

### La restricción real no es el códec, es el contenedor

De `src/recorder/codec_select.c:158-196`. GSR no falla, **cambia el códec por
detrás y avisa**, que para una UI es peor que fallar: el usuario pide una cosa y
recibe otra.

| Códec pedido | Contenedores donde se respeta | Si no |
|---|---|---|
| AAC | todos menos `.webm` | en `.webm` pasa a Opus |
| Opus | `.mp4`, `.mkv`, `.webm`, `.ts`, `.whip` | en cualquier otro pasa a AAC |
| FLAC | `.mp4` y `.mkv` | en `.webm` pasa a Opus, en el resto a AAC |

Y una más: **FLAC no sobrevive a mezclar varias fuentes de audio**. Con `amix`
activo pasa a Opus (`src/recorder/codec_select.c:186-191`).

Consecuencia para la UI, cuando llegue: la pareja contenedor + códec hay que
validarla antes de ofrecerla, o Capturia enseñará "FLAC" y entregará AAC. Encaja
con el principio de solo exponer lo que la máquina soporta de verdad.

## Recomendación sobre el backend ffmpeg

**Hay que escribirlo. No sobra.**

Es la tercera fila del árbol de decisión, la que decía "backend propio
ffmpeg+PipeWire para todo el modo audio-only, tal como estaba previsto en la
arquitectura". No hay reparto posible con GSR: no es que le falten códecs, es
que no arranca sin vídeo.

Por eso **`graba_audio_solo()` se queda** en `src/core/entorno.cpp`. El encargo
pedía borrarla si la conclusión era que sobraba; la conclusión es la contraria.
Lo que sí se ha corregido es su comentario, que daba la pregunta por abierta, y
ahora cita el sitio del código que la cierra.

Lo que esto significa en la práctica:

- ffmpeg pasa de "quizá" a **dependencia real del modo audio-only**. Sigue
  siendo proceso externo, nunca enlazado, igual que GSR.
- `--check` ya lo refleja: sin ffmpeg dice "Audio sin video: NO posible".
- El backend en sí sigue sin escribir. Esto es una tanda de investigación, no de
  features. Que haga falta no es permiso para empezarlo.

Antes de escribirlo hay una decisión que sigue siendo del titular y que esta
investigación no zanja: **qué formatos ofrece el modo audio-only**. Opus cubre
el caso general y FLAC el de calidad, pero MP3 obliga a otro codificador y
puede que no le haga falta a nadie. Es la pregunta que había en la segunda fila
del árbol y sigue viva.

## El árbol de decisión, resuelto

| Si resulta que... | Entonces el backend ffmpeg es... | Y hacemos |
|---|---|---|
| GSR graba audio sin vídeo, con Opus, AAC, MP3 y FLAC | innecesario | — |
| GSR graba audio sin vídeo, pero solo en Opus y AAC | parcial | — |
| **GSR exige fuente de vídeo siempre** | **imprescindible** | **backend propio ffmpeg+PipeWire para todo el modo audio-only. Es lo que toca** |
