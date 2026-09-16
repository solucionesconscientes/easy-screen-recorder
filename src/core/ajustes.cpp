// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/ajustes.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>

namespace esr {
namespace {

bool contiene(std::initializer_list<std::string_view> lista, std::string_view valor) {
    return std::find(lista.begin(), lista.end(), valor) != lista.end();
}

}  // namespace

std::string extension_de(std::string_view ruta) {
    const auto barra = ruta.find_last_of('/');
    const std::string_view nombre = barra == std::string_view::npos ? ruta : ruta.substr(barra + 1);
    const auto punto = nombre.find_last_of('.');
    if (punto == std::string_view::npos || punto == 0 || punto + 1 == nombre.size()) return {};
    std::string ext(nombre.substr(punto + 1));
    for (char& c : ext) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return ext;
}

std::vector<std::string> contenedores_soportados() { return {"mkv", "mp4", "webm"}; }

bool pista_mezclada(std::string_view pista) {
    return pista.find('|') != std::string_view::npos;
}

bool modo_content_efectivo(std::string_view fuente, std::string_view servidor_grafico) {
    if (fuente == "portal") return true;
    return servidor_grafico == "x11";
}

std::vector<std::string> esquinas_camara() {
    return {"abajo-derecha", "abajo-izquierda", "arriba-derecha", "arriba-izquierda"};
}

std::string fuente_gsr(const AjustesGrabacion& a) {
    if (a.camara.empty()) return a.fuente;

    // La fuente principal va tal cual: su identificador es el de GSR y no se
    // toca. Los prefijos «monitor:»/«v4l2:» son opcionales —GSR deduce el tipo
    // del nombre (capture_source.c:283-297)— y aqui solo se pone el de la
    // camara, que es el unico que hace falta para que no se confunda con un
    // nombre de monitor.
    std::string w = a.fuente + "|v4l2:" + a.camara;
    w += ";width=" + std::to_string(a.camara_ancho_pct) + "%";
    // La altura no se pasa: sin ella GSR mantiene la proporcion de la camara.
    // Fijar las dos deformaria la imagen.
    const bool derecha = a.camara_esquina.find("derecha") != std::string::npos;
    const bool abajo = a.camara_esquina.rfind("abajo", 0) == 0;
    w += derecha ? ";halign=end" : ";halign=start";
    w += abajo ? ";valign=end" : ";valign=start";
    if (a.camara_espejo) w += ";hflip=true";
    return w;
}

std::string familia_codec_video(std::string_view nombre) {
    // Por prefijo, porque las variantes son sufijos del nombre base:
    // hevc_10bit, av1_hdr, h264_vulkan, hevc_hdr_vulkan. h265 es el alias de
    // hevc en la tabla de -k de GSR (args_parser.c:23).
    if (nombre.rfind("h265", 0) == 0) return "hevc";
    for (std::string_view base : {"h264", "hevc", "av1", "vp8", "vp9"}) {
        if (nombre.rfind(base, 0) == 0) return std::string(base);
    }
    return {};
}

bool contenedor_admite_video(std::string_view extension, std::string_view codec) {
    const std::string familia = familia_codec_video(codec);
    // Sin familia reconocida no hay regla que aplicar: «auto» entra por aqui, y
    // tambien cualquier nombre que GSR añada y este codigo no conozca todavia.
    if (familia.empty()) return true;
    if (extension == "webm") return familia == "vp8" || familia == "vp9" || familia == "av1";
    if (extension == "mp4") return familia != "vp8";
    if (extension == "mkv") return true;
    return true;  // contenedor sin medir: no se bloquea por no saber
}

std::vector<std::string> codecs_video_para(std::string_view extension,
                                           const std::vector<std::string>& disponibles) {
    std::vector<std::string> lista;
    for (const auto& c : disponibles) {
        if (contenedor_admite_video(extension, c)) lista.push_back(c);
    }
    return lista;
}

std::vector<std::string> codecs_audio_para(std::string_view extension, bool mezcla) {
    // opus: mp4, mkv, webm, ts, whip. flac: mp4, mkv. aac: todos menos webm.
    // El orden importa: el primero es el que la UI preselecciona, y opus
    // delante en todos es el default del proyecto.
    if (extension == "mkv" || extension == "mp4") {
        if (mezcla) return {"opus", "aac"};
        return {"opus", "aac", "flac"};
    }
    if (extension == "webm") return {"opus"};
    if (extension == "ts" || extension == "whip") return {"opus", "aac"};
    return {};
}

std::vector<std::string> validar(const AjustesGrabacion& a) {
    std::vector<std::string> problemas;

    if (a.fuente.empty()) problemas.push_back("falta la fuente de captura");
    if (a.salida.empty()) problemas.push_back("falta el fichero de salida");
    if (a.fuente == "region" && a.region.empty()) {
        problemas.push_back("la fuente «region» necesita el recorte AnchoxAlto+X+Y");
    }
    if (!a.camara.empty()) {
        if (a.camara == a.fuente) {
            problemas.push_back("la camara no puede ser ademas la fuente principal");
        }
        // Por debajo del 5 % no se distingue una cara; al 100 % tapa la
        // pantalla entera y entonces lo que se quiere es grabar solo la camara.
        if (a.camara_ancho_pct < 5 || a.camara_ancho_pct > 50) {
            problemas.push_back("el tamaño de la camara va entre el 5 % y el 50 % del ancho; se pidio " +
                                std::to_string(a.camara_ancho_pct));
        }
        const auto esquinas = esquinas_camara();
        if (std::find(esquinas.begin(), esquinas.end(), a.camara_esquina) == esquinas.end()) {
            problemas.push_back("esquina de camara desconocida «" + a.camara_esquina + "»");
        }
    }
    if (!a.modo_fotogramas.empty() &&
        !contiene({"cfr", "vfr", "content"}, a.modo_fotogramas)) {
        problemas.push_back("el modo de fotogramas es cfr, vfr o content; se pidio «" +
                            a.modo_fotogramas + "»");
    }
    if (!a.limite_resolucion.empty()) {
        int ancho = 0;
        int alto = 0;
        char sobra = 0;
        if (std::sscanf(a.limite_resolucion.c_str(), "%dx%d%c", &ancho, &alto, &sobra) != 2 ||
            ancho <= 0 || alto <= 0) {
            problemas.push_back("el limite de resolucion se escribe AnchoxAlto, por ejemplo "
                                "1920x1080; se pidio «" + a.limite_resolucion + "»");
        }
    }
    if (a.fps < 1 || a.fps > 1000) {
        // Los limites son los que declara GSR para -f (args_parser.c:538).
        problemas.push_back("fps fuera del rango de GSR (1 a 1000): " + std::to_string(a.fps));
    }
    if (a.audios.empty() && !a.sin_audio) {
        problemas.push_back(
            "sin ninguna pista de audio; para grabar sin audio hay que pedirlo con --sin-audio");
    }
    if (a.sin_audio && !a.audios.empty()) {
        problemas.push_back("--sin-audio y pistas de audio a la vez no tiene sentido");
    }

    if (a.replay_segundos != 0 &&
        (a.replay_segundos < kReplayMinimo || a.replay_segundos > kReplayMaximo)) {
        problemas.push_back("el buffer de replay va de " + std::to_string(kReplayMinimo) +
                            " a " + std::to_string(kReplayMaximo) + " segundos; se pidio " +
                            std::to_string(a.replay_segundos));
    }

    // En modo replay la salida es una CARPETA y el contenedor va aparte, asi que
    // la extension no solo no hace falta: sobra. El resto de reglas de codec se
    // comprueban igual, contra el contenedor pedido.
    const std::string ext = a.replay_segundos != 0 ? a.contenedor : extension_de(a.salida);
    if (a.replay_segundos != 0) {
        if (a.salida.empty()) return problemas;
        if (!extension_de(a.salida).empty()) {
            problemas.push_back("en modo replay la salida es una carpeta, no un fichero: GSR "
                                "pone un nombre a cada volcado");
        }
    } else if (a.salida.empty() || ext.empty()) {
        if (!a.salida.empty()) {
            problemas.push_back("la salida no tiene extension y de ella sale el contenedor");
        }
        return problemas;
    }

    // La pareja contenedor + codec de VIDEO. Aqui no hay cambio por detras: el
    // grabador arranca, muere al escribir la cabecera y no deja fichero. Es el
    // fallo mas caro de todos, porque se descubre cuando ya has grabado.
    if (!contenedor_admite_video(ext, a.codec_video)) {
        std::string admite = "vp8, vp9 o av1";
        if (ext == "mp4") admite = "h264, hevc, vp9 o av1";
        problemas.push_back("un «." + ext + "» no admite video " +
                            familia_codec_video(a.codec_video) + "; solo " + admite +
                            ". Cambia de formato o de codec, o deja el codec en «auto», que "
                            "lo elige GSR mirando el contenedor");
    }

    // Las parejas contenedor + codec de audio en las que GSR respeta lo
    // pedido (codec_select.c:158-196). Fuera de ellas GSR cambia el codec
    // por detras, y eso aqui es un error, no un aviso.
    if (a.codec_audio == "opus") {
        if (!contiene({"mp4", "mkv", "webm", "ts", "whip"}, ext)) {
            problemas.push_back("opus no se respeta en «." + ext +
                                "»: GSR lo cambiaria a AAC por detras. Contenedores validos: "
                                "mkv, mp4, webm, ts");
        }
    } else if (a.codec_audio == "flac") {
        if (!contiene({"mp4", "mkv"}, ext)) {
            problemas.push_back("flac solo se respeta en mkv y mp4; en «." + ext +
                                "» GSR lo cambiaria por detras");
        } else if (std::any_of(a.audios.begin(), a.audios.end(), pista_mezclada)) {
            problemas.push_back(
                "flac no se respeta cuando se mezclan varias fuentes en una pista: GSR lo "
                "cambiaria a opus por detras (codec_select.c:186-191). Pide opus, o deja "
                "cada fuente en su propia pista");
        }
    } else if (a.codec_audio == "aac") {
        if (ext == "webm") {
            problemas.push_back("aac no se respeta en webm: GSR lo cambiaria a Opus por detras");
        }
    } else {
        problemas.push_back("codec de audio desconocido «" + a.codec_audio +
                            "»: GSR 6.0.0 trae aac, opus y flac");
    }

    return problemas;
}

std::vector<std::string> argumentos_gsr(const AjustesGrabacion& a) {
    std::vector<std::string> args;
    args.insert(args.end(), {"-w", fuente_gsr(a)});
    if (a.fuente == "region" && !a.region.empty()) {
        args.insert(args.end(), {"-region", a.region});
    }
    if (a.fuente == "portal") {
        // Sin esto el usuario veria el dialogo de permiso del portal en CADA
        // grabacion. Con el token guardado (GSR lo gestiona en su ruta por
        // defecto) solo lo ve la primera vez.
        args.insert(args.end(), {"-restore-portal-session", "yes"});
    }
    args.insert(args.end(), {"-f", std::to_string(a.fps)});
    for (const auto& dispositivo : a.audios) {
        args.insert(args.end(), {"-a", dispositivo});
    }
    // «auto» no se pasa: es el default de GSR y asi la linea de comandos
    // enseña solo lo que se decidio de verdad.
    if (!a.codec_video.empty() && a.codec_video != "auto") {
        args.insert(args.end(), {"-k", a.codec_video});
    }
    args.insert(args.end(), {"-ac", a.codec_audio});
    args.insert(args.end(), {"-q", a.calidad});
    if (a.replay_segundos != 0) {
        args.insert(args.end(), {"-r", std::to_string(a.replay_segundos)});
        // El contenedor por -c y no por la extension: en replay la salida es
        // una carpeta y no hay extension de la que sacarlo.
        args.insert(args.end(), {"-c", a.contenedor});
    }
    if (!a.modo_fotogramas.empty()) {
        args.insert(args.end(), {"-fm", a.modo_fotogramas});
    }
    if (!a.limite_resolucion.empty()) {
        args.insert(args.end(), {"-s", a.limite_resolucion});
    }
    if (!a.ruta_socket.empty()) {
        args.insert(args.end(), {"-ipc", a.ruta_socket});
    }
    args.insert(args.end(), {"-o", a.salida});
    return args;
}

namespace {

std::string carpeta_usuario(const std::string& clave_xdg) {
    const char* hogar = std::getenv("HOME");
    const std::string casa = (hogar != nullptr && hogar[0] != '\0') ? hogar : ".";

    std::string ruta_conf;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0') {
        ruta_conf = std::string(xdg) + "/user-dirs.dirs";
    } else {
        ruta_conf = casa + "/.config/user-dirs.dirs";
    }

    std::ifstream f(ruta_conf);
    std::string linea;
    while (std::getline(f, linea)) {
        // Formato: XDG_VIDEOS_DIR="$HOME/Vídeos". Solo se expande $HOME, que
        // es lo unico que xdg-user-dirs escribe ahi.
        const std::string clave = clave_xdg + "=\"";
        const auto pos = linea.find(clave);
        if (pos == std::string::npos) continue;
        std::string valor = linea.substr(pos + clave.size());
        const auto cierre = valor.find('"');
        if (cierre == std::string::npos) continue;
        valor.resize(cierre);
        const std::string marca = "$HOME";
        if (valor.compare(0, marca.size(), marca) == 0) valor = casa + valor.substr(marca.size());
        if (!valor.empty()) return valor;
    }
    return casa;
}

}  // namespace

std::string carpeta_videos() { return carpeta_usuario("XDG_VIDEOS_DIR"); }

std::string carpeta_musica() { return carpeta_usuario("XDG_MUSIC_DIR"); }

std::string nombre_por_defecto(const std::string& carpeta, const std::string& extension) {
    std::time_t ahora = std::time(nullptr);
    std::tm desglosado{};
    localtime_r(&ahora, &desglosado);
    char sello[32];
    std::strftime(sello, sizeof(sello), "%Y%m%d-%H%M%S", &desglosado);
    return carpeta + "/easy-screen-recorder-" + sello + "." + extension;
}

}  // namespace esr
