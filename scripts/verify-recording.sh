#!/usr/bin/env bash
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
binario="${CAPTURIA_BIN:-$raiz/build/src/cli/capturia}"

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
  echo "FALLO: el binario capturia no esta en $binario"
  echo "  compila primero: cmake -S . -B build -G Ninja && cmake --build build"
  exit 1
fi

# La grabacion existe cuando el CLI anuncia la orden «grabar». Se busca en la
# ayuda para no depender de una version concreta del binario.
if ! "$binario" --help 2>&1 | grep -qE '^\s+capturia (grabar|record)'; then
  echo "capturia todavia no tiene orden de grabacion (--help no anuncia «grabar»)."
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
destino="$HOME/.cache/capturia/verify/prueba.mkv"
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
"$binario" grabar --salida "$destino" || fallo "«capturia grabar» devolvio error"
sleep 3

ruta="$("$binario" parar)" || fallo "«capturia parar» devolvio error"
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
echo "VERIFICADO: $ruta ($duracion s, video + audio segun ffprobe)"
rm -f "$ruta"
exit 0
