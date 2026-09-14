#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace capturia {

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
    // Cada entrada es una pista de audio (-a). La sintaxis «a|b» de GSR que
    // mezcla fuentes en una pista no se ofrece: con ella FLAC cambia a Opus
    // por detras (codec_select.c:186-191) y ese silencio es el que evitamos.
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

// capturia-AAAAMMDD-HHMMSS.<extension> dentro de la carpeta dada.
std::string nombre_por_defecto(const std::string& carpeta,
                               const std::string& extension = "mkv");

}  // namespace capturia
