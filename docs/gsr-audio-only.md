# ¿Puede GSR grabar audio sin vídeo?

**Estado: SIN RESPUESTA. Bloqueado (bloqueo B1 de ESTADO.md).**

Esta era la investigación que podía cambiar el alcance del proyecto. No se ha
podido hacer: el código de GSR, sus páginas de manual y sus binarios no están
en el entorno donde se ejecutó la Tanda 1. El detalle de lo comprobado está en
`gsr-ipc.md`, sección "Por qué está bloqueado", y no se repite aquí.

Lo que sí se puede dejar hecho es que la respuesta, cuando llegue, no tenga que
pelearse con el código ya escrito. Eso está resuelto: **hoy no existe ni una
línea de backend de audio con ffmpeg.** La única dependencia de ffmpeg que hay
es `Entorno::graba_audio_solo()`, tres líneas en `src/core/entorno.cpp`, con el
comentario que dice que esa condición cambia según lo que salga de aquí.

## Las tres preguntas, sin responder

**a) ¿Puede GSR grabar audio sin fuente de vídeo, hoy?** Sin respuesta.

**b) Si no, ¿qué lo impide exactamente a nivel de código?** Sin respuesta.

**c) ¿Qué códecs de audio soporta de verdad y en qué contenedores?** Sin
respuesta.

Del encargo salen dos afirmaciones que hay que tratar como hipótesis, no como
datos: que GSR trae Opus y AAC nativos, y que en su historial existen las
opciones `-aa` y `-aai` para audio de aplicaciones seleccionadas. Ninguna está
comprobada.

## Cómo responderlas

En la máquina de desarrollo:

1. `grep -n '"-aa"\|"-aai"\|"-a"' third_party/gpu-screen-recorder/src/args_parser.c`.
   Ahí está qué opciones existen de verdad y cuáles son obligatorias.
2. Buscar en ese mismo fichero la validación que exige una fuente de vídeo.
   Si existe, esa línea es la respuesta a la pregunta (b): es el sitio exacto
   donde GSR se niega. Anotar `fichero:línea`.
3. `man gpu-screen-recorder` y `man gsr-cli`, y contrastarlos con
   `args_parser.c`: cuando el manpage y el código no coinciden, manda el código.
4. `include/recorder/audio_codec.h` y `codec_select.h` para la pregunta (c), más
   `meson_options.txt` para saber qué códecs son opcionales de compilación. Un
   códec que existe en el código pero está desactivado en el paquete instalado
   es, para el usuario, un códec que no existe: por eso `--check` tiene que
   leerlo de la máquina, no de la lista de códecs del proyecto.
5. **La prueba que decide.** Intentar la grabación de verdad y mirar el
   resultado con ffprobe:

   ```
   gpu-screen-recorder -a default_output -c opus -o /tmp/solo-audio.opus
   ffprobe -v error -show_entries stream=codec_type,codec_name /tmp/solo-audio.opus
   ```

   Un fichero con un único stream de tipo `audio` responde a (a) que sí. Un
   error de "falta la fuente de vídeo" responde que no, y el texto del error
   lleva al sitio del código que pide (b). Esto vale más que cualquier lectura:
   la lectura dice qué debería pasar, ffprobe dice qué pasa.

## Qué haremos con cada respuesta

Escrito de antemano para que la Tanda 2 no tenga que discutirlo:

| Si resulta que... | Entonces el backend ffmpeg es... | Y hacemos |
|---|---|---|
| GSR graba audio sin vídeo, con Opus, AAC, MP3 y FLAC | **innecesario** | nada de ffmpeg. Un backend menos que mantener, un proceso menos, arranque más rápido. Se borra `graba_audio_solo()` y su dependencia |
| GSR graba audio sin vídeo, pero solo en Opus y AAC | **parcial** | GSR para Opus y AAC; ffmpeg solo para MP3 y FLAC. Antes de escribirlo, preguntar al titular si MP3 y FLAC merecen un segundo backend, o si con Opus y AAC basta |
| GSR exige fuente de vídeo siempre | **imprescindible** | backend propio ffmpeg+PipeWire para todo el modo audio-only, tal como estaba previsto en la arquitectura |

## Recomendación provisional

**No escribir ni una línea del backend ffmpeg hasta responder (a).** Es la
recomendación clara que pide el encargo, y no es una forma de aplazar la
decisión: en dos de los tres desenlaces ese backend sobra entero o casi entero,
y el proyecto tiene como principio de diseño no añadir lo que no es
imprescindible. Escribirlo antes de saberlo es apostar a la rama menos probable
de las tres y arriesgarse a tirar el trabajo.

Coste de esperar: una tarde de lectura en la máquina buena. Coste de
equivocarse: un backend de audio entero, con su dependencia de libav y su
proceso extra, mantenido para siempre sin hacer falta.
