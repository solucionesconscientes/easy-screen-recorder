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

# GSR puede estar en PATH o empaquetado como flatpak. En KDE Plasma el flatpak
# es la via habitual, asi que buscarlo solo en PATH daria «ausente» en maquinas
# donde GSR funciona perfectamente. Debe coincidir con localizar_gsr() de
# src/core/entorno.cpp: si aqui y alli se resuelve distinto, el volcado y la
# deteccion en runtime dejan de hablar de lo mismo.
app_flatpak="com.dec05eba.gpu_screen_recorder"

flatpak_tiene_la_app() {
  command -v flatpak >/dev/null 2>&1 || return 1
  for raiz_fp in /var/lib/flatpak "${XDG_DATA_HOME:-$HOME/.local/share}/flatpak"; do
    [ -d "$raiz_fp/app/$app_flatpak" ] && return 0
  done
  return 1
}

# Escribe la linea de comando con la que se invoca $1, o nada si no esta.
resolver() {
  local binario="$1"
  if command -v "$binario" >/dev/null 2>&1; then
    printf '%s' "$binario"
  elif flatpak_tiene_la_app; then
    printf 'flatpak run --command=%s %s' "$binario" "$app_flatpak"
  fi
}

gsr="$(resolver gpu-screen-recorder)"
gsr_cli="$(resolver gsr-cli)"

# Si no se encuentra, se deja el nombre pelado: asi el bloque queda en el
# volcado con su «command not found», que es el dato honesto.
: "${gsr:=gpu-screen-recorder}"
: "${gsr_cli:=gsr-cli}"

# Cada elemento es la linea de comando completa, tal cual se ejecuta.
comandos=(
  "$gsr --version"
  "$gsr --info"
  "$gsr --list-capture-options"
  "$gsr --list-audio-devices"
  "$gsr --list-application-audio"
  "$gsr_cli --help"
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
  if flatpak_tiene_la_app; then
    echo "# gsr:      flatpak $app_flatpak $(flatpak info --show-commit "$app_flatpak" 2>/dev/null | head -c 12)"
  else
    echo "# gsr:      no empaquetado como flatpak en esta maquina"
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
