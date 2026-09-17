<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Easy Screen Recorder frente a lo que ya hay en Linux

Base para el material de marketing. Escrito para **no** poder ser desmontado por
el primer comentario de alguien que conozca el terreno: si una ventaja no está
comprobada, aquí lleva «SIN VERIFICAR» y no debe salir a la web como afirmación.

Consultado el 2026-09-17.

## Veredicto en diez líneas

Easy Screen Recorder es la opción adecuada para **una persona que graba su
pantalla para explicar algo y quiere terminar en dos clics**: un tutorial, una
demo para un cliente, un informe de error, una clase. Con su cara en una esquina
si hace falta, y ahora también emitiendo en directo sin montar escenas.

**No es la opción para quien hace streaming como actividad**: ahí OBS gana y no
hay discusión, porque lo que se necesita son escenas, superposiciones, chat y
alertas, y eso es precisamente lo que aquí se ha decidido no construir.

Y hay un competidor incómodo que no es OBS: **gpu-screen-recorder, el motor que
usamos, trae su propia interfaz gráfica**. Cualquiera que instale nuestra
aplicación tiene que instalar GSR primero, y con eso ya recibe una interfaz que
funciona. El marketing tiene que responder a eso, no esquivarlo.

## El elefante: GSR ya trae interfaz

**Comprobado en esta máquina.** El flatpak `com.dec05eba.gpu_screen_recorder`,
que es dependencia nuestra, trae estos binarios:

```
gpu-screen-recorder      el motor, que es lo que usamos
gpu-screen-recorder-gtk  una interfaz GTK  ← y es su comando POR DEFECTO
gsr-ui                   una interfaz nueva, superpuesta, estilo ShadowPlay
gsr-cli, gsr-kms-server, gsr-global-hotkeys, ...
```

Su ficha de Flathub lo confirma: dos interfaces, la GTK y una «ShadowPlay-like
fullscreen overlay UI» experimental, y la GTK se va a retirar en favor de la
nueva. Versión 6.1.2, publicada cinco días antes de esta consulta. Nosotros
fijamos la 6.0.0.

**Consecuencia honesta:** no somos «la interfaz que le falta a GSR». GSR tiene
interfaz. Somos *otra* interfaz, con otro criterio de diseño. Eso puede seguir
valiendo —y creo que vale— pero el argumento tiene que ser el criterio, no la
ausencia.

**Y aquí está el agujero de esta comparativa, que hay que tapar antes de
publicar nada:** las ventajas que más peso tienen en la tabla de abajo están
medidas contra **el motor**, no contra **su interfaz**. No sé si
`gpu-screen-recorder-gtk` fuerza el volcado a disco, si valida las parejas de
códec y contenedor, o si expone la cámara superpuesta. Intenté abrirla y
capturarla y no lo conseguí en el tiempo que le dediqué. **Hasta que se use de
verdad, esas tres filas son «sin verificar» y no pueden salir como ventaja.**

## Lo que sí está medido, y es nuestro

Estas tres cosas se descubrieron midiendo, y valen como argumento porque son
fallos reales que se arreglaron:

**1. Una grabación cortada de golpe no se perdía: se perdía ENTERA.** Medido con
un A/B limpio —10 segundos grabados y un SIGKILL al grabador, que es lo más
parecido a un apagón que se puede provocar a mano—: sin forzar el volcado a
disco se recuperan **0 segundos**; forzándolo, **8,1 de los 10**. Y en el modo de
solo audio era peor: opus, flac y mp3 dejaban un fichero de **0 bytes**.

El motor de GSR **no fuerza ese volcado por defecto** (eso está medido). Si su
interfaz tampoco lo hace, esto es una ventaja real. **SIN VERIFICAR.**

**2. Tres parejas de códec y formato que la interfaz permitía no grababan
nada.** `h264` y `hevc` en webm, `vp8` en mp4: arrancan, mueren al escribir la
cabecera y no dejan fichero. Ahora ni se pueden pedir. **SIN VERIFICAR** si otra
interfaz las previene.

**3. La aplicación repara sola lo que quedó a medias.** Un mkv sin cerrar trae el
vídeo dentro pero muchos reproductores no lo abren. Al arrancar lo detecta y
ofrece rehacerlo, sin recodificar. Verificado: de «sin duración» a 8,07 s con
vídeo y audio.

## Tabla comparativa

Lo que dice cada ficha oficial o su propia documentación. Donde no hay dato, se
dice.

| | **Easy Screen Recorder** | **GSR (su GTK)** | **OBS Studio** | **Kooha** | **SimpleScreenRecorder** | **Spectacle** | **vokoscreenNG** |
|---|---|---|---|---|---|---|---|
| Wayland y X11 | Sí | Sí | Sí | Sí (Wayland) | **Su web no menciona Wayland** | Sí (es de KDE) | Sí |
| Codificación en GPU | Sí, es de GSR | Sí | Sí (VA-API, NVENC) | «Experimental» | Su web no lo menciona | Sin dato | Sin dato |
| Grabar en dos clics | **Sí, es el contrato** | Sin verificar | No, hay que montar escena | Sí | Más o menos | Sí | No |
| Cámara superpuesta | **Sí, colocándola con el ratón** | Sin verificar (su ficha no la menciona) | Sí, como fuente de escena | No | No | No | Sí, ventana aparte |
| Vista previa para encuadrarte | **Sí** | Sin verificar | Sí | No | No | No | Sí |
| Emitir en directo | **Sí, RTMP/RTMPS** | Sí | Sí, y a todo | No | «Experimental» | No | No |
| Escenas, filtros, chat, alertas | **No, a propósito** | No | **Sí, es su razón de ser** | No | No | No | No |
| Audio de sistema y micro | Sí, mezclados **o** en pistas separadas | Sí | Sí, mezclador completo | Los dos a la vez; pistas separadas sin dato | Sí | Sin dato | Sí |
| Audio de una sola aplicación | **Sí** | Sí | Sí | No | No | No | No |
| Modo repetición | Sí | Sí | Sí (replay buffer) | No | No | No | No |
| Solo audio, sin vídeo | **Sí** | No | Con apaños | No | No | No | No |
| Vúmetro antes de grabar | **Sí** | Sin verificar | Sí | No | Sí | No | Sin dato |
| Sobrevive a un apagón | **Sí, medido** | Sin verificar | Sin verificar | Sin verificar | Sin verificar | Sin verificar | Sin verificar |
| Repara una grabación cortada | **Sí** | Sin verificar | No | No | No | No | No |
| Interfaz en castellano | Sí | Sin dato | Sí | Sí | Sí | Sí | Sí |
| Licencia | GPL-3.0-or-later | GPL-3.0-only | GPL-2.0 | GPL-3.0 | GPL-3.0 | GPL-2.0 | GPL-2.0 |

**Nota sobre el consumo de recursos.** Es tentador decir «consumimos menos que
OBS» porque GSR captura sin pasar por la CPU. **No está medido y no debe
publicarse.** Medir la comparación cuesta 199 MB de descarga de OBS y media
hora; hasta entonces, la baza es la simplicidad, no el consumo.

## Para quién es la opción adecuada

**Quien graba para explicar algo.** Tutoriales, demos, clases, documentación. Se
abre, se elige la pantalla y se graba. Todo lo demás está plegado y no estorba.

**Quien necesita salir en un rincón de su propia pantalla.** Pantalla y cámara
en un solo fichero, sin montar nada y sin editar después: lo compone el grabador
en vivo. El tamaño y la esquina se eligen arrastrando y viéndose la cara antes de
empezar.

**Quien graba en un portátil y no quiere que el ventilador salga en el audio.**
La codificación va entera en la GPU.

**Quien quiere emitir su pantalla en directo y ya.** Pegar la clave de YouTube y
pulsar un botón, sin escenas.

**Quien graba notas de voz o audio de sistema sin vídeo.** El modo de solo audio
no necesita ni GSR.

**Quien trabaja en KDE Plasma.** Es donde se desarrolla y se verifica, y el
atajo global y el icono de bandeja son nativos ahí.

## Para quién NO es

Y esto tiene que estar en la web, no escondido.

**Para quien hace streaming en serio: usa OBS.** Escenas, superposiciones,
transiciones, chat, alertas, múltiples destinos, mezclador con filtros por
fuente. Nada de eso está aquí ni va a estar. Aquí se emite una pantalla, con la
cámara en una esquina si quieres, y punto.

**Para quien quiere editar lo grabado: usa otra cosa.** Aquí no se corta, no se
recorta el principio, no se añaden rótulos. Para recortar sin recodificar,
LosslessCut; para editar de verdad, Kdenlive o Shotcut.

**Para quien quiere subtítulos o transcripción automática: aquí no hay.** Y la
razón es honesta: YouTube y TikTok ya los generan gratis, y hacerlo en local con
un portátil sin GPU dedicada es lento. Se valoró y se descartó.

**Para quien graba juegos y quiere una superposición al estilo ShadowPlay:** eso
es justo lo que hace `gsr-ui`, la interfaz nueva de GSR. Va por ahí.

**Para quien usa Windows o macOS: no hay versión y no va a haberla.** En Windows
el sistema ya trae dos grabadores y los tres fabricantes de GPU traen el suyo;
ese hueco está tapado. La decisión está razonada en `docs/ROADMAP.md`.

**Para quien quiere zoom automático tipo Screen Studio: no existe en Linux.** Ni
aquí ni en ningún otro. Es el hueco más grande que queda sin tapar, y decirlo es
más útil que insinuar que lo tapamos.

## Lo que hay que hacer antes de publicar

1. **Usar `gpu-screen-recorder-gtk` de verdad** y rellenar las cuatro filas «sin
   verificar» de la tabla. Es el competidor más cercano y ahora mismo es un
   hueco en el argumento.
2. Decidir si se mide el consumo frente a OBS. Sin medición, esa frase no se
   escribe.
3. Elegir dónde vive la página en el sitio: la colección `productos` es para
   recomendaciones con afiliación y su esquema no encaja con software propio.

## Fuentes

- Binarios del flatpak de GSR: comprobado en esta máquina con
  `flatpak run --command=sh com.dec05eba.gpu_screen_recorder -c 'ls /app/bin/'`
  y `flatpak info -m`.
- GSR en Flathub: <https://flathub.org/apps/com.dec05eba.gpu_screen_recorder>
- OBS Studio: <https://obsproject.com/>
- Kooha en Flathub: <https://flathub.org/apps/io.github.seadve.Kooha>
- SimpleScreenRecorder: <https://www.maartenbaert.be/simplescreenrecorder/>
- Mediciones propias de apagón, parejas de códec y reparación: `CHANGELOG.md` y
  `docs/emision.md`, cada una con su comando.
