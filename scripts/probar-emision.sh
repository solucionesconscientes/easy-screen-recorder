#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Prueba la emision en directo SIN tocar ninguna cuenta.
#
# Levanta un receptor RTMP en la propia maquina y abre una ventana con lo que
# llega. Si en esa ventana ves tu pantalla, la emision funciona: lo que estas
# viendo ha salido por RTMP, ha llegado a un servidor y ha vuelto.
#
# Veras el efecto tunel —la ventana dentro de si misma— y eso es justo la
# prueba: si fuera una captura local no habria retardo ni tunel.
#
# Uso:
#   scripts/probar-emision.sh            20 segundos
#   scripts/probar-emision.sh 60         los que digas

set -u

segundos="${1:-20}"
puerto=1935
raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binario="${ESR_BIN:-$raiz/build/src/cli/easy-screen-recorder-cli}"

fallo() { echo "FALLO: $*" >&2; exit 1; }

[ -x "$binario" ] || fallo "no encuentro $binario. Compila primero:
  cmake -S . -B build -G Ninja && cmake --build build"
command -v ffplay >/dev/null || fallo "falta ffplay (paquete ffmpeg)"

if ss -ltn 2>/dev/null | grep -q ":$puerto "; then
  fallo "el puerto $puerto esta ocupado. Cierra lo que lo use y repite"
fi

if "$binario" estado >/dev/null 2>&1; then
  fallo "ya hay una grabacion en marcha. Parala con «$binario parar»"
fi

echo "── Receptor escuchando en rtmp://127.0.0.1:$puerto/live/prueba"
echo "   Se abrira una ventana con lo que llegue. Cierrala o espera a que acabe."
# El receptor tiene que estar antes: RTMP lo abre el que emite.
ffplay -loglevel error -window_title "Recibido por RTMP (esto viene de la red)" \
  -listen 1 -i "rtmp://127.0.0.1:$puerto/live/prueba" &
receptor=$!
sleep 3

echo "── Emitiendo $segundos segundos…"
"$binario" emitir --url "rtmp://127.0.0.1:$puerto/live" --clave prueba --bitrate 2500 \
  || { kill "$receptor" 2>/dev/null; fallo "«emitir» devolvio error"; }

for _ in $(seq "$segundos"); do
  sleep 1
  kill -0 "$receptor" 2>/dev/null || break   # el usuario cerro la ventana
done

"$binario" parar >/dev/null 2>&1
sleep 2
kill "$receptor" 2>/dev/null
wait "$receptor" 2>/dev/null

echo
echo "── Terminado. Si has visto tu pantalla en la ventana, la emision funciona."
echo "   Para emitir de verdad en YouTube: docs/emision.md"
