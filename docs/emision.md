<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Emisión en directo por RTMP

Todo lo de aquí está comprobado ejecutándolo el 2026-09-17 en la máquina de
desarrollo, o citado de su fuente oficial. Lo que no, lleva «SIN VERIFICAR».

## Resumen

GSR ya emitía y no lo sabíamos. Reconoce `rtmp://` y `rtmps://` en `-o`
(`src/args_parser.c:234` de GSR 6.0.0) y su propio manual trae el ejemplo de
Twitch. Lo nuestro ha sido exponerlo con las reglas correctas, no implementar una
emisión.

**Verificado emitiendo de verdad** contra un servidor RTMP local —un `ffmpeg
-listen 1` en `127.0.0.1`, nunca contra una cuenta real—: llegaron h264 1366×768
y aac, 21,1 s a 2,16 Mbps de los 2500 pedidos. Repetido a 3000 kbps: 2,6 Mbps.
Y por el camino de la interfaz: h264 + aac, 24,8 s.

## Qué hace falta que no hacía falta grabando

| Qué | Por qué |
|---|---|
| Contenedor `flv` | Es el de RTMP. No estaba en nuestra lista |
| Códec de audio `aac` | flv **solo** respeta aac: pedir opus acaba en aac sin avisar (`codec_select.c:158-196`). La validación lo rechaza y `emitir` lo pone solo |
| `-bm cbr` | Bitrate constante. Con calidad constante el caudal sube en cuanto se mueve la pantalla, y eso una plataforma no lo admite |
| Ojo con `-q` | **Cambia de significado**: en modo cbr son kbps y no `very_high`. Está en el manual de GSR, en `-q` |
| Salida como URL | Había tres sitios que daban por hecho un fichero: la trampa de `/tmp`, la comprobación de que exista la carpeta, y el marcador de reparación |

## Requisitos de YouTube, de su fuente

De <https://support.google.com/youtube/answer/2853702>, consultado el
2026-09-17:

| Qué | YouTube pide | Qué hacemos |
|---|---|---|
| Códec de vídeo | H.264 (también HEVC y AV1) | h264 por defecto; `auto` de GSR elige h264 |
| Intervalo de keyframes | **2 s recomendado, no pasar de 4** | GSR usa **2,0 por defecto** (`args_parser.c:282`). Cumplimos sin tocar nada |
| Audio | AAC o MP3, 128 kbps estéreo | aac. GSR pone 160 kbps por defecto para aac, por encima de la recomendación |
| Protocolo | **RTMPS recomendado**, RTMP aceptado | Los dos. El texto de ejemplo de la interfaz es `rtmps://` |
| Bitrate de vídeo | 4 Mbps hasta 720p30 · 10 Mbps 1080p30 · 12 Mbps 1080p60 · 15 Mbps 1440p30 · 24 Mbps 1440p60 · 30 Mbps 4K30 · 35 Mbps 4K60 | Esas mismas cifras en el selector |

**Un error que cometí y conviene que quede escrito:** las primeras cifras que
puse (4500 kbps para 1080p) las saqué de memoria y estaban mal. YouTube pide
10 Mbps para 1080p30. Ninguna cifra sin fuente, ni aquí ni en la interfaz.

## Lo que no se puede hacer emitiendo

**Pausar.** El manual de GSR sugiere que no se puede («not for
streaming/replay», en la documentación de SIGUSR2), pero **probado, sí lo
acepta**: responde `Paused`. Lo que pasa es que deja de mandar imagen, y una
plataforma trata eso como emisión caída. Así que:

- La interfaz **no ofrece** pausa emitiendo, ni con el botón ni con el atajo.
- La línea de comandos **avisa y deja hacer**: quien lo pide por ahí sabrá por
  qué.

**Modo repetición.** Se excluyen: emitiendo no hay buffer que guardar, lo que
sale ya se ha ido. La validación lo rechaza.

## La clave de emisión

**No se guarda en ningún sitio.** Hay que pegarla cada vez, y eso se dice al
lado del campo donde se pega.

Lo que sí se recuerda es el **servidor de ingesta**, que es una URL pública —la
misma que YouTube publica en su propia página—. La distinción no es cosmética:
nuestro fichero de configuración es texto plano, y una clave de emisión da
permiso para emitir en tu canal. `recordar_url_emision()` se niega incluso a
guardar una cadena que parezca llevar la clave pegada detrás.

Tampoco se imprime al arrancar ni se escribe en el log: esa salida acaba en
informes de error y en capturas de pantalla.

**Guardarla de forma segura es posible y queda pendiente**: libsecret está en el
SDK de Flatpak (comprobado) y kwalletd expone `org.freedesktop.secrets` en esta
máquina. Costaría una dependencia enlazada, un permiso de DBus más en el
manifiesto —y la pregunta del revisor de Flathub que viene con él—. No entró en
la primera versión por eso.

## Otros destinos

- **Twitch**: `rtmp://live.twitch.tv/app/<clave>`. Es el ejemplo del propio
  manual de GSR. **SIN VERIFICAR** contra Twitch real.
- **RTMP genérico**: cualquier servidor. Es lo que se ha probado aquí.
- **WHIP** (WebRTC): GSR lo soporta con `-c whip`. Fuera de alcance: es otro
  protocolo y otra conversación.

## Lo que queda fuera a propósito

Escenas, transiciones, superposiciones, chat, alertas, multistream y estado de
la emisión (bitrate real, frames perdidos). Lo primero es OBS y no vamos a
competir con eso. Lo último **SIN VERIFICAR**: no se ha mirado qué información
expone GSR por su IPC durante una emisión.

## Cómo probarlo sin tocar una cuenta

```bash
# Servidor RTMP local: un ffmpeg escuchando
ffmpeg -listen 1 -i rtmp://127.0.0.1:1935/live/pruebas -c copy /tmp/recibido.flv &

# Emitir contra él
easy-screen-recorder-cli emitir --url rtmp://127.0.0.1:1935/live --clave pruebas \
  --bitrate 2500
sleep 10
easy-screen-recorder-cli parar

# Y comprobar lo que llegó
ffprobe -v error -show_entries stream=codec_name \
  -show_entries format=duration,bit_rate -of default=nw=1 /tmp/recibido.flv
```

Es lo que hace `scripts/verify-recording.sh`, que salta el caso si el puerto
1935 está ocupado en vez de fallar por algo que no es nuestro.
