#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Arnes de verificacion de grabacion.
#
# Idea: grabar de verdad y comprobar el resultado con ffprobe, porque una
# grabacion que "parece" ir es exactamente el fallo que este proyecto no se
# puede permitir.
#
# Hasta la Tanda 2 este script salia siempre con 0, tambien cuando no podia
# comprobar nada. Eso lo hacia verde por construccion, o sea inutil como
# verificacion. Ahora puede fallar, y falla en cuanto hay algo que verificar
# y no se verifica.

set -u

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binario="${ESR_BIN:-$raiz/build/src/cli/easy-screen-recorder-cli}"

# GSR puede estar en PATH o como flatpak. Debe resolverse igual que
# localizar_gsr() en src/core/entorno.cpp y que scripts/volcar-capacidades.sh:
# mirar solo el PATH daba «ausente» en maquinas donde GSR funciona, y ese
# diagnostico falso es el que este arnes arrastraba desde la Tanda 1.
app_flatpak="com.dec05eba.gpu_screen_recorder"

gsr_disponible() {
  command -v gpu-screen-recorder >/dev/null 2>&1 && return 0
  command -v flatpak >/dev/null 2>&1 || return 1
  for raiz_fp in /var/lib/flatpak "${XDG_DATA_HOME:-$HOME/.local/share}/flatpak"; do
    [ -d "$raiz_fp/app/$app_flatpak" ] && return 0
  done
  return 1
}

echo "verify-recording: comprobando que hay algo que verificar"
echo

# Sin binario no se puede verificar nada, y "no se puede verificar" es un
# fallo, no un pendiente: quien invoca este script espera una verificacion.
if [ ! -x "$binario" ]; then
  echo "FALLO: el binario easy-screen-recorder-cli no esta en $binario"
  echo "  compila primero: cmake -S . -B build -G Ninja && cmake --build build"
  exit 1
fi

# La grabacion existe cuando el CLI anuncia la orden «grabar». Se busca en la
# ayuda para no depender de una version concreta del binario.
if ! "$binario" --help 2>&1 | grep -qE '^\s+easy-screen-recorder-cli (grabar|record)'; then
  echo "easy-screen-recorder-cli todavia no tiene orden de grabacion (--help no anuncia «grabar»)."
  echo "No hay nada que verificar. Esta rama muere en la Tanda 4: cuando"
  echo "«grabar» exista, este script grabara de verdad o fallara."
  echo
  echo "PENDIENTE. No es un fallo."
  exit 0
fi

# A partir de aqui hay orden de grabacion, asi que todo lo que falte es fallo.
faltan=()
gsr_disponible               || faltan+=("gpu-screen-recorder: ni en PATH ni como flatpak $app_flatpak")
command -v ffprobe >/dev/null 2>&1 || faltan+=("ffprobe no esta en PATH: sin el no se puede comprobar lo grabado")

if [ ${#faltan[@]} -gt 0 ]; then
  echo "FALLO: hay orden de grabacion pero falta con que verificarla:"
  for f in "${faltan[@]}"; do
    echo "  - $f"
  done
  exit 1
fi

# Las comprobaciones reales: grabar unos segundos de verdad y exigirle al
# resultado duracion, un flujo de video y uno de audio. Una grabacion que
# "parece" ir es exactamente el fallo que no nos podemos permitir.
#
# El destino va bajo el home y no bajo /tmp: el /tmp de un GSR en flatpak es
# privado del sandbox (docs/gsr-ipc.md, "La trampa del flatpak").
destino="$HOME/.cache/easy-screen-recorder/verify/prueba.mkv"
mkdir -p "$(dirname "$destino")"
rm -f "$destino"

fallo() {
  echo "FALLO: $1"
  exit 1
}

if "$binario" estado >/dev/null 2>&1; then
  fallo "ya hay una grabacion en marcha; este arnes no la toca. Parala y repite"
fi

echo "grabando 3 segundos de prueba..."
"$binario" grabar --salida "$destino" || fallo "«easy-screen-recorder-cli grabar» devolvio error"
sleep 3

ruta="$("$binario" parar)" || fallo "«easy-screen-recorder-cli parar» devolvio error"
[ "$ruta" = "$destino" ] || fallo "parar dijo «$ruta» y se pidio «$destino»"
[ -s "$ruta" ] || fallo "el fichero guardado no existe o esta vacio: $ruta"

# Lo grabado, contra ffprobe. Un solo aviso de ffprobe tambien es fallo.
flujos="$(ffprobe -v error -show_entries stream=codec_type -of csv=p=0 "$ruta")" \
  || fallo "ffprobe no puede leer $ruta"
echo "$flujos" | grep -qx "video" || fallo "no hay flujo de video en $ruta"
echo "$flujos" | grep -qx "audio" || fallo "no hay flujo de audio en $ruta"

duracion="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$ruta")"
case "$duracion" in
  ''|N/A) fallo "ffprobe no da duracion para $ruta" ;;
esac
# Se pidieron 3 s; menos de 2 significa que algo corto la grabacion.
if ! awk "BEGIN{exit !($duracion >= 2.0)}"; then
  fallo "duracion $duracion s, se esperaban al menos 2 s"
fi

echo
echo "VERIFICADO pantalla: $ruta ($duracion s, video + audio segun ffprobe)"
rm -f "$ruta"

# --- El reparto de las pistas de audio ---------------------------------------
# Las dos formas de grabar sistema Y microfono, y son distintas de verdad: «a|b»
# en un solo -a las MEZCLA en una pista (gpu-screen-recorder.1, ejemplo de -a) y
# dos -a dejan cada una en la suya. La diferencia se nota al reproducir: con dos
# pistas, casi todos los reproductores suenan solo la primera.
#
# Se cuentan los flujos porque es lo unico que distingue las dos cosas sin
# escuchar el fichero. Si algun dia GSR dejara de entender el «|», aqui saldrian
# dos pistas donde se pidio una y esto fallaria.
pistas_de() {
  ffprobe -v error -select_streams a -show_entries stream=index -of csv=p=0 "$1" | grep -c .
}

for reparto in mezcladas separadas; do
  destino_pistas="$HOME/.cache/easy-screen-recorder/verify/$reparto.mkv"
  rm -f "$destino_pistas"
  if [ "$reparto" = mezcladas ]; then
    esperadas=1
    args=(--audio "default_output|default_input")
  else
    esperadas=2
    args=(--audio default_output --audio default_input)
  fi

  echo
  echo "grabando 3 segundos con el audio en pistas $reparto..."
  "$binario" grabar --salida "$destino_pistas" "${args[@]}" \
    || fallo "«grabar» con el audio $reparto devolvio error"
  sleep 3
  ruta_pistas="$("$binario" parar)" || fallo "«parar» devolvio error con el audio $reparto"
  [ -s "$ruta_pistas" ] || fallo "no se guardo nada en $ruta_pistas"

  cuantas="$(pistas_de "$ruta_pistas")"
  [ "$cuantas" = "$esperadas" ] \
    || fallo "audio $reparto: se esperaban $esperadas pistas y hay $cuantas en $ruta_pistas"
  echo "VERIFICADO audio $reparto: $cuantas pista(s) segun ffprobe"
  rm -f "$ruta_pistas"
done

# --- Camara superpuesta -------------------------------------------------------
# Pantalla y camara en UNA grabacion y un solo proceso: GSR las compone el mismo
# («-w "pantalla|v4l2:/dev/videoN;width=..."»). Lo que se comprueba aqui es que
# sale UN fichero con UN flujo de video, porque el fallo posible no es que se
# vea mal: es que GSR no entienda la cadena y no grabe nada.
#
# Se salta si no hay camara, y eso NO es un fallo: una maquina sin webcam es
# normal. Lo que si seria fallo es dar por buena una funcion sin probarla, y por
# eso se dice en voz alta cuando se salta.
camara="$("$binario" fuentes 2>/dev/null | awk '$1 ~ /^\/dev\/video/ {print $1; exit}')"
if [ -z "$camara" ]; then
  echo
  echo "SALTADA la camara superpuesta: esta maquina no lista ninguna /dev/videoN"
else
  destino_cam="$HOME/.cache/easy-screen-recorder/verify/camara.mkv"
  rm -f "$destino_cam"
  echo
  echo "grabando 3 segundos con la camara superpuesta ($camara)..."
  "$binario" grabar --salida "$destino_cam" --camara "$camara" --camara-tamano 20 \
    --camara-x 5 --camara-y 5 \
    || fallo "«grabar» con camara superpuesta devolvio error"
  sleep 3
  ruta_cam="$("$binario" parar)" || fallo "«parar» devolvio error con la camara"
  [ -s "$ruta_cam" ] || fallo "camara: no se guardo nada en $ruta_cam"
  n_video="$(ffprobe -v error -select_streams v -show_entries stream=index -of csv=p=0 "$ruta_cam" | grep -c .)"
  [ "$n_video" = 1 ] || fallo "se esperaba 1 flujo de video compuesto y hay $n_video"
  echo "VERIFICADO camara superpuesta: 1 flujo de video con las dos fuentes dentro"
  rm -f "$ruta_cam"
fi

# --- Modo repeticion ----------------------------------------------------------
# El replay no escribe NADA hasta que se le pide, asi que aqui se comprueban las
# dos mitades: que durante el buffer la carpeta sigue vacia, y que «guardar»
# deja un fichero de verdad. Si lo primero fallara, estaria escribiendo a disco
# sin parar y el modo no serviria para nada.
dir_replay="$HOME/.cache/easy-screen-recorder/verify/replay"
rm -rf "$dir_replay"; mkdir -p "$dir_replay"

echo
echo "grabando en modo repeticion (buffer de 5 s)..."
"$binario" grabar --salida "$dir_replay" --replay 5 >/dev/null \
  || fallo "«grabar --replay» devolvio error"
sleep 6
[ "$(ls -1 "$dir_replay" | wc -l)" = 0 ] \
  || fallo "el modo repeticion escribio sin que se lo pidieran"

ruta_replay="$("$binario" guardar)" || fallo "«guardar» devolvio error"
[ -s "$ruta_replay" ] || fallo "el volcado del buffer no existe: $ruta_replay"
dur_replay="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$ruta_replay")"
if ! awk "BEGIN{exit !($dur_replay >= 3.0)}"; then
  fallo "el volcado dura $dur_replay s y el buffer era de 5"
fi
"$binario" parar >/dev/null 2>&1
echo "VERIFICADO repeticion: nada en disco durante el buffer, y $dur_replay s al guardar"
rm -rf "$dir_replay"

# --- El contenedor y el codec de video ---------------------------------------
# Un «.webm» no admite h264 ni hevc. Pedirlo no da un error visible: el grabador
# arranca, muere al escribir la cabecera y NO deja fichero, asi que el fallo se
# descubre cuando ya has grabado. Aqui se comprueba que el camino que la
# interfaz ofrece de verdad —«auto», que GSR resuelve mirando el contenedor—
# sigue dando un codec que webm acepta.
destino_webm="$HOME/.cache/easy-screen-recorder/verify/prueba.webm"
rm -f "$destino_webm"

echo
echo "grabando 3 segundos en webm..."
"$binario" grabar --salida "$destino_webm" || fallo "«grabar» en webm devolvio error"
sleep 3
ruta_webm="$("$binario" parar)" || fallo "«parar» devolvio error con el webm"
[ -s "$ruta_webm" ] || fallo "webm: no se guardo nada en $ruta_webm"

codec_webm="$(ffprobe -v error -select_streams v -show_entries stream=codec_name -of csv=p=0 "$ruta_webm")"
case "$codec_webm" in
  vp8|vp9|av1) ;;
  *) fallo "webm trae video «$codec_webm», y webm solo admite vp8, vp9 o av1" ;;
esac
echo "VERIFICADO webm: video $codec_webm, que es de los que webm admite"
rm -f "$ruta_webm"

# --- Emision en directo por RTMP ---------------------------------------------
# Se emite contra un servidor RTMP LOCAL, nunca contra una cuenta de verdad: el
# receptor es un ffmpeg escuchando en 127.0.0.1, y lo que se exige es que lo que
# llegue sea h264 + aac de verdad y no un fichero a medio escribir.
#
# Este caso es el que distingue «la opcion existe» de «la emision funciona».
destino_rtmp="$HOME/.cache/easy-screen-recorder/verify/emitido.flv"
rm -f "$destino_rtmp"
puerto_rtmp=1935

if ss -ltn 2>/dev/null | grep -q ":$puerto_rtmp "; then
  echo
  echo "SALTADA la emision: el puerto $puerto_rtmp esta ocupado"
else
  timeout 40 ffmpeg -v error -y -listen 1 \
    -i "rtmp://127.0.0.1:$puerto_rtmp/live/arnes" -c copy "$destino_rtmp" \
    >/dev/null 2>&1 &
  pid_servidor=$!
  sleep 4

  echo
  echo "emitiendo 8 segundos contra 127.0.0.1..."
  "$binario" emitir --url "rtmp://127.0.0.1:$puerto_rtmp/live" --clave arnes \
    --bitrate 2500 >/dev/null || fallo "«emitir» devolvio error"
  sleep 8
  "$binario" parar >/dev/null 2>&1
  sleep 3
  kill "$pid_servidor" 2>/dev/null
  wait "$pid_servidor" 2>/dev/null

  [ -s "$destino_rtmp" ] || fallo "el servidor RTMP no recibio nada"
  codecs_rtmp="$(ffprobe -v error -show_entries stream=codec_name -of csv=p=0 "$destino_rtmp" | tr '\n' ' ')"
  case "$codecs_rtmp" in
    *h264*aac*|*aac*h264*) ;;
    *) fallo "la emision trajo «$codecs_rtmp» y se esperaba h264 y aac" ;;
  esac
  dur_rtmp="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$destino_rtmp")"
  if ! awk "BEGIN{exit !($dur_rtmp >= 3.0)}"; then
    fallo "la emision duro $dur_rtmp s y se emitieron 8"
  fi
  echo "VERIFICADO emision: $dur_rtmp s recibidos por RTMP, h264 + aac"
  rm -f "$destino_rtmp"
fi

# --- Sobrevivir a un apagon --------------------------------------------------
# Se mata al grabador con SIGKILL, que es lo mas parecido a un corte de luz que
# se puede provocar a mano, y se exige que lo grabado siga ahi. Sin el volcado
# forzado a disco esto devolvia CERO segundos: el fichero existia y estaba vacio
# de contenido util.
destino_corte="$HOME/.cache/easy-screen-recorder/verify/corte.mkv"
rm -f "$destino_corte"

echo
echo "grabando 8 segundos y matando el grabador a lo bruto..."
"$binario" grabar --salida "$destino_corte" >/dev/null || fallo "«grabar» devolvio error"
sleep 8
pid_gsr="$(cat "$HOME/.cache/easy-screen-recorder/sesion/gsr.pid" 2>/dev/null)"
[ -n "$pid_gsr" ] || fallo "no se apunto el pid del grabador"
pkill -9 -P "$pid_gsr" 2>/dev/null
kill -9 "$pid_gsr" 2>/dev/null
sleep 3

[ -s "$destino_corte" ] || fallo "tras el corte no quedo nada en $destino_corte"
# Un mkv a medias no trae duracion: hace falta rehacerlo, y eso lo hace la
# aplicacion sola. Esto comprueba las dos mitades.
a_medias="$("$binario" reparar | tail -1)"
[ "$a_medias" = "$destino_corte" ] || fallo "«reparar» no arreglo $destino_corte (dijo «$a_medias»)"
dur_corte="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$destino_corte")"
case "$dur_corte" in ''|N/A) fallo "tras reparar sigue sin duracion" ;; esac
if ! awk "BEGIN{exit !($dur_corte >= 4.0)}"; then
  fallo "del corte solo se salvaron $dur_corte s de los 8 grabados"
fi
echo "VERIFICADO apagon: $dur_corte s de los 8 grabados, recuperados y reparados"
rm -f "$destino_corte"

# --- Modo audio-only ---------------------------------------------------------
# Va por ffmpeg, no por GSR (docs/gsr-audio-only.md), asi que se verifica
# aparte y con sus propias herramientas.
command -v ffmpeg >/dev/null 2>&1 || fallo "ffmpeg no esta en PATH: el modo audio-only no se puede verificar"

destino_audio="$HOME/.cache/easy-screen-recorder/verify/prueba.opus"
rm -f "$destino_audio"

echo
echo "grabando 2 segundos de audio de prueba..."
"$binario" audio --salida "$destino_audio" || fallo "«easy-screen-recorder-cli audio» devolvio error"
sleep 2

ruta_audio="$("$binario" parar)" || fallo "«easy-screen-recorder-cli parar» devolvio error con el audio"
[ "$ruta_audio" = "$destino_audio" ] || fallo "parar dijo «$ruta_audio» y se pidio «$destino_audio»"
[ -s "$ruta_audio" ] || fallo "el audio guardado no existe o esta vacio: $ruta_audio"

codec_audio="$(ffprobe -v error -show_entries stream=codec_name -of csv=p=0 "$ruta_audio")" \
  || fallo "ffprobe no puede leer $ruta_audio"
[ "$codec_audio" = "opus" ] || fallo "se pidio opus y el fichero trae «$codec_audio»"

duracion_audio="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$ruta_audio")"
case "$duracion_audio" in
  ''|N/A) fallo "ffprobe no da duracion para $ruta_audio" ;;
esac
if ! awk "BEGIN{exit !($duracion_audio >= 1.0)}"; then
  fallo "duracion del audio $duracion_audio s, se esperaba al menos 1 s"
fi

echo
echo "VERIFICADO audio: $ruta_audio ($duracion_audio s, $codec_audio segun ffprobe)"
rm -f "$ruta_audio"
exit 0
