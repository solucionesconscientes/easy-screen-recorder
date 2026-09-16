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
