// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace esr {

// Una peticion de grabacion ya resuelta: lo que se pidio mas los defaults.
// Es un dato, sin GSR y sin GUI detras.
//
// Los defaults copian a GSR 6.0.0 donde GSR los tiene (opus en
// args_parser.c:262, 60 fps en args_parser.c:395, very_high como -q) y a
// CLAUDE.md donde son nuestros (contenedor mkv, audio del sistema).
struct AjustesGrabacion {
    std::string fuente;   // «eDP-1», «region», «portal», «focused», ruta v4l2, id de ventana
    std::string region;   // WxH+X+Y, solo tiene sentido con fuente == «region»
    std::string salida;   // ruta del fichero; su extension decide el contenedor
    std::string codec_video = "auto";  // «auto» delega en GSR, que es el criterio probado
    std::string codec_audio = "opus";
    // Cada entrada es una pista de audio (-a), y dentro de una entrada se
    // pueden juntar varias fuentes con «|», que es la sintaxis de GSR para
    // mezclarlas en una sola pista (gpu-screen-recorder.1, ejemplo «-a
    // "default_output|default_input"»).
    //
    // La diferencia importa y no es tecnica: con dos pistas separadas, casi
    // todos los reproductores suenan solo la primera, asi que quien grabe
    // sistema y microfono se encuentra el microfono mudo. Mezclado se oye todo
    // en cualquier reproductor; separado se puede equilibrar despues al editar.
    std::vector<std::string> audios = {"default_output"};
    // Grabar sin audio se pide, no se llega por descuido: con audios vacio y
    // esto en false la validacion falla a proposito.
    bool sin_audio = false;
    int fps = 60;
    std::string calidad = "very_high";
    std::string ruta_socket;  // para -ipc; vacio = sin IPC

    // --- La camara superpuesta ------------------------------------------
    //
    // GSR compone la camara sobre la pantalla EL MISMO, en vivo y en un solo
    // proceso: `-w` admite varias fuentes unidas por «|», y cada una acepta
    // tamaño, posicion y volteo (gpu-screen-recorder.1, seccion -w).
    //
    // Eso evita el diseño que habia planeado este proyecto —dos grabaciones y
    // un ffmpeg componiendo despues— que costaba 0,21x del tiempo grabado,
    // dejaba dos ficheros que cuadrar y añadia un estado «componiendo». Nada de
    // eso hace falta. Y no toca la licencia: es un argumento de linea de
    // comandos, no un plugin cargado dentro de GSR.
    std::string camara;  // «/dev/video0»; vacio = sin camara
    // El ancho en PORCENTAJE del video, no en pixeles: un 384 fijo es un cuarto
    // de pantalla en 1366 y un decimo en 4K.
    int camara_ancho_pct = 25;
    // La esquina superior izquierda de la camara, en PORCENTAJE del video.
    //
    // Empezo siendo una de cuatro esquinas y se quedo corto: sobre una barra de
    // tareas, un panel lateral o un video vertical, las cuatro esquinas fallan
    // todas. GSR admite posicion libre —«x=50%;y=10%», convertida a pixeles
    // contra el tamaño del video (capture_setup.c:526-534)— y esta verificado
    // grabando, asi que la interfaz deja colocarla donde sea.
    //
    // El default deja la camara abajo a la derecha con un margen, que es donde
    // menos tapa y donde la pone todo el mundo.
    int camara_x_pct = 73;
    int camara_y_pct = 73;
    // Espejo por defecto, y no es un capricho: sin el, quien se graba se ve al
    // reves de como se ve en un espejo y no se reconoce. Es lo que hace
    // cualquier aplicacion de videollamada.
    bool camara_espejo = true;

    // Carpeta donde se guarda a fichero lo que se esta emitiendo (-ro). Vacia,
    // que es el default, la emision no deja nada: lo que sale se ha ido.
    //
    // No es una segunda grabacion ni un segundo proceso: el MISMO GSR escribe el
    // fichero mientras emite, asi que no hay una segunda codificacion ni el
    // doble de GPU. Lo trae de serie desde la 6.0.0, que es nuestro minimo
    // (gpu-screen-recorder.1, «-ro» y la seccion IPC), y hay que encenderlo por
    // IPC con start-replay-recording: con -ro a secas no graba nada.
    std::string carpeta_guardado;

    // ¿Sale el puntero en el video? (-cursor). GSR lo graba por defecto.
    //
    // Se puede quitar, y no es un capricho: en una demo de una interfaz, el
    // puntero paseando mientras se explica distrae mas que ayuda, y en una
    // grabacion de una ventana para documentacion sobra siempre.
    bool cursor = true;

    // --- Cosas que solo tienen sentido en modo repeticion ----------------
    //
    // Donde vive el buffer (-replay-storage). GSR lo guarda en RAM por defecto,
    // y eso significa que los minutos elegidos son memoria ocupada: con
    // movimiento, 15 minutos rondan medio giga. En disco no ocupa RAM, a cambio
    // de escribir sin parar; el propio GSR avisa de que eso desgasta un SSD.
    bool buffer_en_disco = false;
    // Carpetas por fecha para cada volcado (-df). Con doscientas repeticiones
    // en una carpeta plana no hay quien encuentre nada.
    bool carpetas_por_fecha = false;

    // --- Un guion propio al terminar (-sc) -------------------------------
    //
    // GSR ejecuta ese programa cuando acaba de guardar y le pasa la ruta del
    // fichero y el tipo («regular», «replay» o «screenshot»). Es el gancho para
    // lo que no vamos a hacer nosotros: subirlo, moverlo, avisar por donde sea.
    //
    // No se valida lo que haga: es el guion de quien lo pone. Lo unico que se
    // comprueba antes de lanzar es que exista y se pueda ejecutar, porque un
    // -sc que no existe hace que GSR ni arranque.
    std::string guion_al_terminar;

    // Modo de fotogramas (-fm): «cfr», «vfr» o «content». Vacio deja el default
    // de GSR, que es vfr. «content» solo codifica cuando la pantalla cambia.
    std::string modo_fotogramas;
    // Limite de resolucion de salida (-s), «1920x1080». Vacio = sin limite.
    // GSR escala para caber dentro, respetando la proporcion.
    std::string limite_resolucion;

    // --- Replay ---------------------------------------------------------
    //
    // --- Emision en directo ---------------------------------------------
    //
    // No hay un campo «emitir»: la salida ES la URL. GSR ya reconoce «rtmp://»
    // y «rtmps://» en -o (args_parser.c:234) y su manual trae el ejemplo de
    // Twitch, asi que esto no añade una funcion nueva, expone una que ya
    // estaba.
    //
    // Lo que si cambia es el resto de los ajustes, y por eso hay reglas propias:
    // el contenedor tiene que ser flv, el modo de bitrate CONSTANTE, y la
    // calidad deja de ser un preset para pasar a ser kbps.
    //
    // Modo de bitrate (-bm): «qp», «vbr» o «cbr». Vacio deja el default de GSR,
    // que es qp (calidad constante).
    std::string modo_bitrate;
    // Kbps, solo con modo_bitrate == «cbr». En ese modo GSR lee -q como un
    // numero de kbps y no como «very_high» (gpu-screen-recorder.1, -q).
    int bitrate_kbps = 0;

    // El contenedor, SOLO para el modo replay: fuera de el sale de la extension
    // de `salida`, que es el criterio probado y no se toca.
    std::string contenedor = "mkv";
    // Segundos de buffer (-r). 0 = grabacion normal. Con esto puesto, GSR NO
    // escribe nada hasta que se le pide: guarda en memoria los ultimos N
    // segundos y los vuelca cuando llega «save-replay».
    //
    // Y cambia la forma de la salida: en modo replay «-o» es una CARPETA, no un
    // fichero, porque GSR pone un nombre por cada volcado. El contenedor pasa a
    // «-c», que es de donde saldria la extension.
    int replay_segundos = 0;
};

// El rango de -r que declara GSR (gpu-screen-recorder.1): 2 a 86400 segundos.
inline constexpr int kReplayMinimo = 2;
inline constexpr int kReplayMaximo = 86400;

// Si «-fm content» va a servir de algo con esa fuente y ese servidor grafico.
//
// GSR lo acepta SIEMPRE y, cuando no puede aplicarlo, lo dice por stderr y sigue
// como si nada: «"-fm content" has no effect on Wayland when recording a
// monitor» (medido el 2026-09-16). Para el usuario eso es pedir una cosa y
// recibir otra, que es justo lo que este proyecto valida antes de lanzar.
//
// Funciona en X11, y en Wayland solo capturando por el portal.
bool modo_content_efectivo(std::string_view fuente, std::string_view servidor_grafico);

// La cadena que se le pasa a `-w`: la fuente principal y, si hay camara,
// «|v4l2:RUTA;width=N%;halign=...;valign=...;hflip=...» detras.
std::string fuente_gsr(const AjustesGrabacion& a);

// La extension de la ruta, sin el punto y en minusculas. Vacia si no hay.
std::string extension_de(std::string_view ruta);

// Los contenedores que Easy Screen Recorder ofrece. mkv es el default de CLAUDE.md;
// mp4 y webm son los otros dos con reglas conocidas en codec_select.c.
// No es deteccion: es la lista de lo que este proyecto decide soportar.
std::vector<std::string> contenedores_soportados();

// Si esa pista junta varias fuentes con «|», o sea si GSR va a mezclarlas.
bool pista_mezclada(std::string_view pista);

// Si esa salida es una emision en directo y no un fichero.
//
// Se decide por el esquema de la URL, igual que lo decide GSR
// (args_parser.c:234). Importa en varios sitios: una URL no tiene extension de
// la que sacar el contenedor, no tiene carpeta que comprobar, no se puede
// reparar si se corta y no admite pausa.
bool es_emision(std::string_view salida);

// Une el servidor de ingesta y la clave, que es lo que espera un RTMP:
// «rtmp://servidor/app» + «clave» -> «rtmp://servidor/app/clave».
//
// Van separados a proposito hasta el ultimo momento: la URL del servidor es
// publica y se puede recordar entre sesiones; la clave NO se guarda en ningun
// sitio. Juntarlas antes obligaria a tratar toda la cadena como un secreto.
std::string url_de_emision(std::string_view servidor, std::string_view clave);

// El rango de bitrate que se ofrece para emitir, en kbps. Los extremos son
// nuestros y no de GSR: por debajo de 500 no se ve nada a 1080p, y 51000 deja
// sitio de sobra por encima del maximo que recomienda YouTube, que son 35 Mbps
// para 4K a 60 imagenes por segundo (docs/emision.md).
inline constexpr int kBitrateEmisionMinimo = 500;
inline constexpr int kBitrateEmisionMaximo = 51000;

// La familia de un nombre de codec de video de GSR: «hevc_10bit» y
// «hevc_hdr_vulkan» son hevc, «av1_vulkan» es av1, «h265» es hevc. Vacia si no
// se reconoce, que incluye «auto» a proposito: auto no es un codec, es dejar
// elegir a GSR.
std::string familia_codec_video(std::string_view nombre);

// Si ese contenedor admite ese codec de video.
//
// La tabla sale de meter un flujo de cada codec en cada contenedor con ffmpeg,
// el 2026-09-16 en la maquina de desarrollo:
//
//         mkv   mp4   webm
//   h264   si    si    NO
//   hevc   si    si    NO
//   vp8    si    NO    si
//   vp9    si    si    si
//   av1    si    si    si
//
// Importa porque hasta ahora la interfaz dejaba pedir cualquier pareja, y tres
// de las cinco que ofrecia en webm y mp4 NO GRABABAN NADA: GSR muere al
// escribir la cabecera («Only VP8 or VP9 or AV1 video ... are supported for
// WebM») y el fichero no llega a existir. Medido grabando de verdad.
//
// «auto» siempre vale: GSR mira el contenedor antes de elegir (medido, auto en
// un .webm da vp8 y no h264). Y un contenedor que no esta en la tabla tambien
// pasa: no se ha medido, y bloquear por no saber seria inventarse una regla.
bool contenedor_admite_video(std::string_view extension, std::string_view codec);

// Los codecs de video que ese contenedor admite, de los que se le pasen. La UI
// llena su selector con esto para que una pareja que no grabaria ni se pueda
// pedir, igual que con `codecs_audio_para`.
std::vector<std::string> codecs_video_para(std::string_view extension,
                                           const std::vector<std::string>& disponibles);

// Los codecs de audio que GSR respeta en ese contenedor, sin cambiarlos por
// detras (codec_select.c:158-196). La UI llena su selector con esto: asi es
// imposible pedir una pareja que saldria distinta de lo pedido.
//
// `mezcla` quita flac de la lista: al mezclar fuentes GSR lo cambia a opus
// (codec_select.c:186-191), asi que ofrecerlo ahi seria ofrecer algo que no va
// a salir.
std::vector<std::string> codecs_audio_para(std::string_view extension, bool mezcla = false);

// Comprueba la pareja contenedor + codec de audio contra las reglas de
// codec_select.c:158-196 de GSR 6.0.0. GSR no falla ante una pareja invalida:
// cambia el codec por detras y avisa por stderr, que para el usuario es
// recibir algo distinto de lo que pidio. Por eso se valida antes de lanzar.
// Devuelve los problemas redactados para el usuario; vacio = valido.
std::vector<std::string> validar(const AjustesGrabacion& a);

// Los argumentos de gpu-screen-recorder, tal como los espera args_parser.c.
// El contenedor no se pasa con -c: sale de la extension de -o, que es lo que
// se ejecuto en la prueba real de docs/gsr-ipc.md.
std::vector<std::string> argumentos_gsr(const AjustesGrabacion& a);

// Las carpetas del usuario, leidas de ~/.config/user-dirs.dirs. Sin ese
// dato se devuelve el home: no se inventa una carpeta que no existe.
std::string carpeta_videos();   // XDG_VIDEOS_DIR, para las grabaciones de pantalla
std::string carpeta_musica();   // XDG_MUSIC_DIR, para el modo audio-only

// easy-screen-recorder-AAAAMMDD-HHMMSS.<extension> dentro de la carpeta dada.
std::string nombre_por_defecto(const std::string& carpeta,
                               const std::string& extension = "mkv");

}  // namespace esr
