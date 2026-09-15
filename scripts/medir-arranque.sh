#!/usr/bin/env bash
# Mide arranque y RAM de un binario de easy-screen-recorder-cli. El principio de diseño
# manda: arranque por debajo de 1 segundo y poca RAM. Esto lo convierte en
# numero medido en vez de sensacion.
#
# Uso: scripts/medir-arranque.sh [binario] [argumentos...]
#      Sin argumentos mide el CLI: --version (coste minimo del proceso)
#      y --check (deteccion completa, el techo del arranque de la UI).

set -u

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binario="${1:-$raiz/build/src/cli/easy-screen-recorder-cli}"
shift 2>/dev/null || true

if [ ! -x "$binario" ]; then
  echo "FALLO: no hay binario en $binario"
  exit 1
fi

medir() {
  # Tres pasadas: la primera paga caches frios y se enseña aparte.
  local etiqueta="$1"; shift
  echo "== $etiqueta =="
  local i
  for i in 1 2 3; do
    /usr/bin/time -f "  pasada $i: %e s, RAM maxima %M KiB" "$binario" "$@" > /dev/null 2> >(grep -E "pasada" >&2)
  done
}

if [ $# -gt 0 ]; then
  medir "$binario $*" "$@"
else
  medir "easy-screen-recorder-cli --version" --version
  medir "easy-screen-recorder-cli --check (deteccion completa)" --check
fi

echo
echo "Umbral del proyecto: 1 segundo de arranque (CLAUDE.md)."
