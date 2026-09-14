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

# Aqui van las comprobaciones reales (Tanda 4): grabar unos segundos, y con
# ffprobe exigir duracion, numero de flujos y codec. Hasta que existan, tener
# la orden «grabar» sin verificarla es exactamente el agujero que este script
# tapa, asi que se falla a proposito.
echo "FALLO: capturia anuncia «grabar» pero las comprobaciones de grabacion"
echo "de este arnes no estan escritas. Se escriben en la Tanda 4, junto a la"
echo "orden. Un exito silencioso aqui seria verde por construccion otra vez."
exit 1
