#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Publica la aplicacion en un repositorio Flatpak propio, firmado.
#
# Por que un repositorio propio y no un .flatpak suelto: con el repositorio, el
# usuario recibe las actualizaciones con `flatpak update` como si viniera de
# Flathub. Con un bundle suelto tiene que volver a descargar a mano cada vez, y
# eso significa que nadie actualiza.
#
# El runtime (org.kde.Platform) sigue viniendo de Flathub: aqui solo se aloja la
# aplicacion, que son ~1 MB.
#
# Uso:
#   scripts/publicar-flatpak.sh                 construye y firma en local
#   scripts/publicar-flatpak.sh --publicar      ademas empuja a GitHub Pages
#
# La primera vez hay que crear el repositorio de GitHub y conectarlo a
# Cloudflare Pages, con la rama de produccion `main`, sin comando de
# construccion y con el directorio de salida `/`. El script avisa si no existe.

set -euo pipefail

# ─── Lo que hay que revisar antes de la primera ejecucion ────────────────
APP_ID="es.solucionesconscientes.EasyScreenRecorder"
APP_TITULO="Easy Screen Recorder"
RAMA="stable"
REMOTO_NOMBRE="easy-screen-recorder"
# El repositorio de GitHub que aloja los ficheros, y la URL desde la que se
# sirven.
#
# Se sirve desde un SUBDOMINIO propio en Cloudflare Pages y no desde
# GitHub Pages, por tres razones:
#
#  - Es el dominio del proyecto. Un usuario que va a instalar software y a
#    anclar una clave de firma se fija en de donde viene.
#  - Cloudflare Pages ya esta en uso para el sitio, asi que no hay una cuenta
#    ni un servicio mas que mantener.
#  - Y cabe: medido, el repositorio de UNA version son 148 ficheros y 8,1 MB,
#    con 4,1 MiB el fichero mas grande. Los limites del plan gratuito son
#    20.000 ficheros y 25 MiB por fichero, y con --prune-depth=5 solo se
#    guardan cinco versiones, asi que no se acerca nunca.
#
# Lo que NO se hace: meter esto en el repositorio del sitio web. Es un artefacto
# binario de 8 MB que crece, y acabaria en el historial de git del sitio para
# siempre, redespleegandose en cada cambio de una pagina.
GH_REPO="git@github.com:solucionesconscientes/flatpak-repo.git"
URL_BASE="https://flatpak.solucionesconscientes.es"
GPG_ID="flatpak@solucionesconscientes.es"
FLATHUB="https://dl.flathub.org/repo/flathub.flatpakrepo"

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# El manifiesto vive en empaquetado/flathub y NO en la raiz: la copia de la raiz
# es lo que se envia a flathub/flathub, aqui se usa el original.
MANIFIESTO="$raiz/empaquetado/flathub/$APP_ID.yml"
salida="$raiz/salida-flatpak"
publicar=false
[ "${1:-}" = "--publicar" ] && publicar=true

fallo() { echo "FALLO: $*" >&2; exit 1; }

[ -f "$MANIFIESTO" ] || fallo "no encuentro el manifiesto en $MANIFIESTO"
command -v flatpak-builder >/dev/null || fallo "falta flatpak-builder"
command -v gpg >/dev/null || fallo "falta gpg"

# ─── 1. La clave de firma ────────────────────────────────────────────────
#
# Sin firma, flatpak avisa al instalar y las actualizaciones no se pueden
# verificar. La clave se crea UNA vez y hay que guardarla fuera de esta maquina:
# si se pierde, los usuarios que ya instalaron no pueden recibir nada mas,
# porque su remoto tiene la clave publica vieja anclada.
if ! gpg --list-secret-keys "$GPG_ID" >/dev/null 2>&1; then
  echo "── No hay clave de firma para $GPG_ID."
  echo "   Se va a crear una. Se te pedira una CONTRASEÑA: ponla y guardala."
  echo "   (Una clave sin contraseña en el portatil es una clave regalada a"
  echo "    cualquiera que se lleve el portatil.)"
  read -r -p "   ¿Crear la clave ahora? [s/N] " r
  [ "$r" = "s" ] || fallo "sin clave no se puede firmar"
  gpg --quick-gen-key "$GPG_ID" rsa4096 sign never
  echo
  echo "── HAZ ESTO AHORA, antes de seguir:"
  echo "   gpg --export-secret-keys --armor $GPG_ID > clave-firma-PRIVADA.asc"
  echo "   y guarda ese fichero FUERA del portatil. Luego borralo de aqui."
  read -r -p "   Pulsa Enter cuando este guardada. " _
fi
CLAVE=$(gpg --list-secret-keys --with-colons "$GPG_ID" | awk -F: '/^fpr/{print $10; exit}')
[ -n "$CLAVE" ] || fallo "no se pudo leer la huella de la clave"
echo "── Firmando con $CLAVE"

# ─── 2. Construir y firmar ───────────────────────────────────────────────
#
# El repositorio ostree se CONSERVA entre versiones a proposito: es lo que
# permite que `flatpak update` sepa que hay algo nuevo. Borrarlo obliga a los
# usuarios a reinstalar.
flatpak remote-add --user --if-not-exists flathub "$FLATHUB" >/dev/null 2>&1 || true
mkdir -p "$salida"
flatpak-builder --user --force-clean --install-deps-from=flathub \
  --repo="$salida/repo" --gpg-sign="$CLAVE" --default-branch="$RAMA" \
  "$salida/build" "$MANIFIESTO"

# Los deltas estaticos hacen que una actualizacion descargue solo lo que cambio.
# El prune evita que el repositorio crezca sin limite: Pages no es un
# sitio donde convenga acumular.
flatpak build-update-repo --gpg-sign="$CLAVE" --title="$APP_TITULO" \
  --default-branch="$RAMA" --generate-static-deltas --prune --prune-depth=5 \
  "$salida/repo"

# ─── 3. Los ficheros que instala el usuario ──────────────────────────────
gpg --export "$CLAVE" > "$salida/clave-publica.gpg"
CLAVE_B64=$(base64 -w0 "$salida/clave-publica.gpg")

publico="$salida/publico"
rm -rf "$publico"; mkdir -p "$publico"
cp -r "$salida/repo" "$publico/repo"
# Sin esto, GitHub Pages pasa el repositorio por Jekyll y se come los ficheros
# y directorios que empiezan por guion bajo. Un repo ostree tiene varios.
touch "$publico/.nojekyll"

cat > "$publico/$REMOTO_NOMBRE.flatpakrepo" <<EOF
[Flatpak Repo]
Title=$APP_TITULO
Url=$URL_BASE/repo/
Homepage=https://solucionesconscientes.es/easy-screen-recorder
GPGKey=$CLAVE_B64
EOF

cat > "$publico/$APP_ID.flatpakref" <<EOF
[Flatpak Ref]
Name=$APP_ID
Branch=$RAMA
Title=$APP_TITULO
Url=$URL_BASE/repo/
SuggestRemoteName=$REMOTO_NOMBRE
RuntimeRepo=$FLATHUB
IsRuntime=false
GPGKey=$CLAVE_B64
EOF

# El bundle suelto, para quien prefiera un fichero. Con --repo-url apuntando al
# repositorio, quien lo instale asi TAMBIEN recibe actualizaciones.
flatpak build-bundle --gpg-keys="$salida/clave-publica.gpg" \
  --repo-url="$URL_BASE/repo/" --runtime-repo="$FLATHUB" \
  "$salida/repo" "$publico/$APP_ID.flatpak" "$APP_ID" "$RAMA"

cat > "$publico/index.html" <<EOF
<!doctype html>
<meta charset="utf-8">
<title>$APP_TITULO · Flatpak</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
 body{font-family:system-ui,sans-serif;max-width:40rem;margin:3rem auto;padding:0 1rem;
      line-height:1.6;color:#232629}
 code{background:#eff0f1;padding:.15em .4em;border-radius:3px;font-size:.9em}
 pre{background:#eff0f1;padding:1rem;border-radius:6px;overflow-x:auto}
 a{color:#1d99f3}
 @media(prefers-color-scheme:dark){
   body{background:#232629;color:#eff0f1} code,pre{background:#31363a}}
</style>
<h1>$APP_TITULO</h1>
<p>Record your screen with GPU acceleration on Linux. Wayland and X11.</p>
<h2>Install</h2>
<pre>flatpak install --user $URL_BASE/$APP_ID.flatpakref</pre>
<p>Updates come with <code>flatpak update</code>.</p>
<p>Needs <a href="https://git.dec05eba.com/gpu-screen-recorder/about/">gpu-screen-recorder</a>
   installed on the host — it does the capture and is not bundled.</p>
<p><a href="https://github.com/solucionesconscientes/easy-screen-recorder">Source code</a> ·
   GPL-3.0-or-later</p>
EOF

echo
echo "── Listo en $publico"
ls -1 "$publico" | sed 's/^/     /'

# ─── 4. Publicar ─────────────────────────────────────────────────────────
#
# Se empuja a la rama `main` del repositorio de ficheros y Cloudflare Pages
# despliega sola. El limite de 500 construcciones al mes del plan gratuito da
# para dieciseis publicaciones diarias: de sobra.
if ! $publicar; then
  echo
  echo "── No se ha publicado. Para hacerlo: $0 --publicar"
  echo "   Comprueba antes lo que hay en $publico"
  exit 0
fi

# Comprobacion de tamano antes de subir, porque el limite se descubre tarde:
# Cloudflare Pages rechaza el despliegue entero si un fichero pasa de 25 MiB o
# si hay mas de 20.000.
n_ficheros=$(find "$publico" -type f | wc -l)
mayor=$(find "$publico" -type f -printf '%s\n' | sort -rn | head -1)
echo "── $n_ficheros ficheros, el mayor de $((mayor / 1048576)) MiB"
[ "$n_ficheros" -lt 20000 ] || fallo "mas de 20.000 ficheros: Cloudflare Pages lo rechaza"
[ "$mayor" -lt 26214400 ] || fallo "hay un fichero de mas de 25 MiB: Cloudflare Pages lo rechaza"

cd "$publico"
git init -q -b main
git add -A
git commit -qm "Repositorio flatpak $(date +%F)"
git remote add origin "$GH_REPO"
# Force push a proposito: esta rama es un artefacto, no historia, y asi el
# repositorio de ficheros no acumula 8 MB por version. Lo que SI hay que
# conservar entre versiones es salida-flatpak/repo, que se queda en local.
git push -f -q origin main || fallo "no se pudo empujar. ¿Existe el repositorio $GH_REPO?"
echo "── Empujado. Cloudflare Pages despliega en un minuto."
echo "── Instalacion: flatpak install --user $URL_BASE/$APP_ID.flatpakref"
