#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Rehace las tres capturas de la ficha, sin un solo clic.
#
# Esto era una tarea manual y molesta: en Wayland la aplicacion NO puede ponerse
# delante sola —`requestActivate()` necesita un token de activacion que solo
# concede un clic de verdad— asi que `spectacle -a` capturaba la ventana que
# tuviera el foco, que rara vez era la nuestra. Una vez se guardo por error el
# navegador del titular con su sesion abierta.
#
# La salida es KWin: su interfaz de scripting SI puede activar una ventana y
# quitarle el maximizado. Con eso, las tres capturas salen solas y siempre
# iguales.
#
# Cumple lo que pide Flathub y lo que documenta docs/screenshots/README.md:
# PNG sin transparencia, las tres del mismo tamano, en INGLES y sin maximizar.

set -u

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ui="$raiz/build/src/ui/easy-screen-recorder"
cli="$raiz/build/src/cli/easy-screen-recorder-cli"
destino="$raiz/docs/screenshots"
tmp="$(mktemp -d)"
pid_ui=""

fallo() { echo "FALLO: $*" >&2; limpiar; exit 1; }
limpiar() {
  [ -n "$pid_ui" ] && kill "$pid_ui" 2>/dev/null
  "$cli" parar >/dev/null 2>&1
  rm -rf "$tmp" "$HOME/.cache/easy-screen-recorder/capturas"
}
trap limpiar EXIT

[ -x "$ui" ] || fallo "no encuentro la interfaz en $ui. Compila primero"
command -v spectacle >/dev/null || fallo "falta spectacle"
command -v qdbus6 >/dev/null || fallo "falta qdbus6"
python3 -c "import PIL" 2>/dev/null || fallo "falta python3-pil"

cat > "$tmp/encuadrar.js" <<'JS'
// Trae al frente la ventana de ESR, la desminimiza y le quita el maximizado.
// El identificador es resourceName y NO resourceClass: la clase es
// «es.solucionesconscientes.EasyScreenRecorder» y buscar ahi «easy-screen-recorder»
// no casa por las mayusculas. Costo un rato descubrirlo.
const lista = workspace.windowList();
for (let i = 0; i < lista.length; ++i) {
    const w = lista[i];
    if (String(w.resourceName) !== "easy-screen-recorder") continue;
    if (w.minimized) w.minimized = false;
    w.setMaximize(false, false);
    workspace.activeWindow = w;
}
JS

al_frente() {
  qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript \
    "$tmp/encuadrar.js" "esr-captura-$RANDOM" >/dev/null 2>&1
  qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.start >/dev/null 2>&1
  sleep 2
}

# En INGLES: la ficha la lee todo el mundo. Sin esto sale en el idioma del
# sistema, y ese fue uno de los dos errores de la primera tanda de capturas.
abrir() {
  [ -n "$pid_ui" ] && kill "$pid_ui" 2>/dev/null
  sleep 2
  LANG=C.UTF-8 "$ui" "$@" >/dev/null 2>&1 &
  pid_ui=$!
  sleep 10
}

disparar() {
  al_frente
  spectacle -a -b -n -o "$1" 2>/dev/null
  sleep 3
  [ -s "$1" ] || fallo "spectacle no dejo nada en $1"
}

echo "── La pantalla tiene que estar encendida: sin plano de video activo no hay captura"
kscreen-doctor --dpms on >/dev/null 2>&1

echo "── 1/3 ventana principal"
abrir
disparar "$tmp/principal.png"

echo "── 2/3 panel Avanzado"
abrir --avanzado
disparar "$tmp/avanzado.png"

echo "── 3/3 grabando"
# La grabacion arranca ANTES de la ventana: asi se abre ya en ese estado y no se
# minimiza, porque minimizarse es su reaccion a que el estado CAMBIE.
mkdir -p "$HOME/.cache/easy-screen-recorder/capturas"
"$cli" grabar --fuente "$("$cli" fuentes | awk 'NR==1{print $1}')" \
  --salida "$HOME/.cache/easy-screen-recorder/capturas/x.mkv" >/dev/null 2>&1 \
  || fallo "no se pudo grabar para la captura"
sleep 2
abrir
disparar "$tmp/grabando.png"
"$cli" parar >/dev/null 2>&1

echo "── Componiendo"
python3 - "$tmp" "$destino" <<'PY'
import sys
from PIL import Image
tmp, destino = sys.argv[1], sys.argv[2]
# Flathub pide PNG sin transparencia y todas del mismo tamano. Las capturas
# vienen con sombra (o sea con alfa) y de tamanos distintos, asi que se componen
# sobre un lienzo opaco comun. El lienzo se ajusta a la mayor con margen: en uno
# demasiado grande la ventana queda perdida en un descampado gris, que en una
# ficha de tienda se lee como aplicacion a medio hacer.
LIENZO = (1100, 790)
FONDO = (220, 223, 225)
for nombre in ("principal", "grabando", "avanzado"):
    v = Image.open("%s/%s.png" % (tmp, nombre)).convert("RGBA")
    if v.width > LIENZO[0] or v.height > LIENZO[1]:
        raise SystemExit("la captura %s (%dx%d) no cabe en el lienzo %s" %
                         (nombre, v.width, v.height, LIENZO))
    lienzo = Image.new("RGB", LIENZO, FONDO)
    lienzo.paste(v, ((LIENZO[0] - v.width) // 2, (LIENZO[1] - v.height) // 2), v)
    lienzo.save("%s/%s.png" % (destino, nombre), "PNG", optimize=True)
    print("   %-12s %s" % (nombre, LIENZO))
PY

echo
echo "── Listas en $destino"
echo "   MIRALAS antes de subirlas: se graba el escritorio real y puede salir"
echo "   algo personal. Y acuerdate de apuntar el metainfo al commit que las trae:"
echo "   docs/screenshots/README.md lo explica."
