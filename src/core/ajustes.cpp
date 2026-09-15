// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/ajustes.hpp"

#include <algorithm>
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

std::vector<std::string> codecs_audio_para(std::string_view extension) {
    // opus: mp4, mkv, webm, ts, whip. flac: mp4, mkv. aac: todos menos webm.
    // El orden importa: el primero es el que la UI preselecciona, y opus
    // delante en todos es el default del proyecto.
    if (extension == "mkv" || extension == "mp4") return {"opus", "aac", "flac"};
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

    const std::string ext = extension_de(a.salida);
    if (a.salida.empty() || ext.empty()) {
        if (!a.salida.empty()) {
            problemas.push_back("la salida no tiene extension y de ella sale el contenedor");
        }
        return problemas;
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
    args.insert(args.end(), {"-w", a.fuente});
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
