#!/usr/bin/env bash
# Vuelca lo que ESTA maquina sabe hacer, en el formato que lee
# capturia::partir_volcado (src/core/include/capturia/capacidades.hpp).
#
# No aborta nunca: un comando que falta o que falla es un dato, no un error del
# script. Por eso no hay `set -e`.
#
# Uso: scripts/volcar-capacidades.sh [fichero-de-salida]
#      Sin argumento escribe en docs/gsr-capabilities.txt

set -u

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
destino="${1:-$raiz/docs/gsr-capabilities.txt}"

# Cada elemento es la linea de comando completa, tal cual se ejecuta.
comandos=(
  "gpu-screen-recorder --version"
  "gpu-screen-recorder --info"
  "gpu-screen-recorder --list-capture-options"
  "gpu-screen-recorder --list-audio-devices"
  "gpu-screen-recorder --list-application-audio"
  "gsr-cli --help"
)

{
  # Todo lo anterior al primer «### comando:» lo ignora el parser a proposito:
  # sirve para saber de que maquina salio el volcado.
  echo "# Volcado de capacidades de Capturia"
  echo "# fecha:    $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "# host:     $(hostname 2>/dev/null || echo desconocido)"
  echo "# kernel:   $(uname -sr 2>/dev/null || echo desconocido)"
  echo "# sistema:  $(. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME" || echo desconocido)"
  echo "# sesion:   XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-<vacio>} WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-<vacio>} DISPLAY=${DISPLAY:-<vacio>}"
  echo "# escritorio: ${XDG_CURRENT_DESKTOP:-<vacio>}"
  if [ -d /dev/dri ]; then
    echo "# /dev/dri: $(ls /dev/dri 2>/dev/null | tr '\n' ' ')"
  else
    echo "# /dev/dri: no existe (sin GPU accesible)"
  fi
  echo "#"

  for cmd in "${comandos[@]}"; do
    echo "### comando: $cmd"
    salida="$(eval "$cmd" 2>&1)"
    codigo=$?
    echo "### codigo: $codigo"
    if [ -n "$salida" ]; then
      printf '%s\n' "$salida"
    fi
    echo "### fin"
  done
} > "$destino"

echo "escrito: $destino"
