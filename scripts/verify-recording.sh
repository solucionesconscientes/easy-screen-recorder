#!/usr/bin/env bash
# Arnes de verificacion de grabacion.
#
# Idea: grabar de verdad y comprobar el resultado con ffprobe, porque una
# grabacion que "parece" ir es exactamente el fallo que este proyecto no se
# puede permitir. En la Tanda 1 todavia no hay nada que grabar, asi que el
# script detecta que falta y sale en verde sin romper el build.

set -u

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binario="${CAPTURIA_BIN:-$raiz/build/src/cli/capturia}"

echo "verify-recording: comprobando que hay algo que verificar"
echo

faltan=()

if [ ! -x "$binario" ]; then
  faltan+=("el binario capturia no esta compilado en $binario (cmake -S . -B build -G Ninja && cmake --build build)")
else
  # La grabacion se dara por existente cuando el CLI tenga una orden para ella.
  # Se busca en la ayuda para no depender de una version concreta del binario.
  if ! "$binario" --help 2>&1 | grep -qE '^\s+capturia (grabar|record)'; then
    faltan+=("capturia todavia no tiene orden de grabacion (--help no anuncia «grabar»)")
  fi
fi

command -v gpu-screen-recorder >/dev/null 2>&1 || faltan+=("gpu-screen-recorder no esta en PATH")
command -v ffprobe            >/dev/null 2>&1 || faltan+=("ffprobe no esta en PATH: sin el no se puede comprobar lo grabado")

if [ ${#faltan[@]} -eq 0 ]; then
  echo "Todo lo necesario esta presente, pero las comprobaciones de grabacion"
  echo "aun no estan escritas. Se activan en la Tanda 2."
  echo
  echo "PENDIENTE (Tanda 2). No es un fallo."
  exit 0
fi

echo "Este arnes todavia no puede verificar nada. Falta:"
for f in "${faltan[@]}"; do
  echo "  - $f"
done
echo
echo "PENDIENTE (Tanda 2). No es un fallo: en la Tanda 1 no hay grabacion que probar."
exit 0
