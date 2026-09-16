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
};

// La extension de la ruta, sin el punto y en minusculas. Vacia si no hay.
std::string extension_de(std::string_view ruta);

// Los contenedores que Easy Screen Recorder ofrece. mkv es el default de CLAUDE.md;
// mp4 y webm son los otros dos con reglas conocidas en codec_select.c.
// No es deteccion: es la lista de lo que este proyecto decide soportar.
std::vector<std::string> contenedores_soportados();

// Si esa pista junta varias fuentes con «|», o sea si GSR va a mezclarlas.
bool pista_mezclada(std::string_view pista);

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
