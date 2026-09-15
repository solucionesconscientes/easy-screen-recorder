// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/ajustes.hpp"

#include <algorithm>
#include <cstdlib>

#include "comprobar.hpp"

using namespace esr;

namespace {

bool algun_problema_contiene(const std::vector<std::string>& problemas, std::string_view trozo) {
    return std::any_of(problemas.begin(), problemas.end(), [&](const std::string& p) {
        return p.find(trozo) != std::string::npos;
    });
}

AjustesGrabacion base() {
    AjustesGrabacion a;
    a.fuente = "eDP-1";
    a.salida = "/tmp/x.mkv";
    return a;
}

}  // namespace

int main() {
    // Extension: de ahi sale el contenedor, asi que sin ambiguedades.
    COMPROBAR(extension_de("/a/b/video.mkv") == "mkv");
    COMPROBAR(extension_de("video.MKV") == "mkv");
    COMPROBAR(extension_de("sin_extension") == "");
    COMPROBAR(extension_de("/a.b/sin_extension") == "");
    COMPROBAR(extension_de(".oculto") == "");
    COMPROBAR(extension_de("acaba.en.punto.") == "");

    // Defaults: los de GSR (opus, 60, very_high) y codec delegado en auto.
    {
        const AjustesGrabacion a;
        COMPROBAR(a.codec_audio == "opus");
        COMPROBAR(a.fps == 60);
        COMPROBAR(a.calidad == "very_high");
        COMPROBAR(a.codec_video == "auto");
        COMPROBAR(a.audios.size() == 1 && a.audios[0] == "default_output");
    }

    // La pareja contenedor + codec de audio, contra codec_select.c:158-196.
    // GSR cambiaria el codec por detras; aqui eso es error antes de lanzar.
    COMPROBAR(validar(base()).empty());  // opus en mkv: valido
    {
        auto a = base();
        a.salida = "/tmp/x.flv";  // opus fuera de mp4/mkv/webm/ts/whip
        COMPROBAR(algun_problema_contiene(validar(a), "opus no se respeta"));
    }
    {
        auto a = base();
        a.codec_audio = "flac";
        COMPROBAR(validar(a).empty());  // flac en mkv: valido
        a.salida = "/tmp/x.webm";
        COMPROBAR(algun_problema_contiene(validar(a), "flac solo se respeta"));
    }
    {
        auto a = base();
        a.codec_audio = "aac";
        a.salida = "/tmp/x.webm";
        COMPROBAR(algun_problema_contiene(validar(a), "aac no se respeta"));
        a.salida = "/tmp/x.mp4";
        COMPROBAR(validar(a).empty());
    }
    {
        auto a = base();
        a.codec_audio = "mp3";  // GSR 6.0.0 no lo trae (include/defs.h:77-79)
        COMPROBAR(algun_problema_contiene(validar(a), "desconocido"));
    }
    {
        auto a = base();
        a.fuente = "region";  // sin recorte
        COMPROBAR(algun_problema_contiene(validar(a), "AnchoxAlto"));
        a.region = "800x600+10+10";
        COMPROBAR(validar(a).empty());
    }
    {
        auto a = base();
        a.fps = 0;
        COMPROBAR(algun_problema_contiene(validar(a), "fps"));
        a.fps = 1001;
        COMPROBAR(algun_problema_contiene(validar(a), "fps"));
    }
    {
        auto a = base();
        a.salida = "/tmp/sin_extension";
        COMPROBAR(algun_problema_contiene(validar(a), "extension"));
    }
    {
        AjustesGrabacion a;  // sin fuente ni salida
        const auto p = validar(a);
        COMPROBAR(algun_problema_contiene(p, "fuente"));
        COMPROBAR(algun_problema_contiene(p, "salida"));
    }

    // La regla que llena los selectores de la UI. Coherencia exigida: todo
    // lo que codecs_audio_para devuelve tiene que pasar validar(), y lo que
    // no devuelve tiene que fallarla. Si las dos fuentes divergen, este test
    // lo caza.
    for (const std::string& cont : contenedores_soportados()) {
        const auto codecs = codecs_audio_para(cont);
        COMPROBAR_NOTA(!codecs.empty(), cont);
        COMPROBAR(codecs.front() == "opus");  // el default del proyecto, delante
        for (const std::string& codec : {std::string("aac"), std::string("opus"), std::string("flac")}) {
            auto a = base();
            a.salida = "/x/v." + cont;
            a.codec_audio = codec;
            const bool permitido =
                std::find(codecs.begin(), codecs.end(), codec) != codecs.end();
            COMPROBAR_NOTA(validar(a).empty() == permitido, cont + "+" + codec);
        }
    }
    COMPROBAR(codecs_audio_para("flv").empty());

    // La linea de comandos que ve GSR. El orden de -w primero no es manía:
    // es el argumento obligatorio (args_parser.c:536) y asi los errores de
    // GSR lo citan el primero.
    {
        auto a = base();
        a.ruta_socket = "/home/u/.cache/easy-screen-recorder/ipc.sock";
        const auto args = argumentos_gsr(a);
        const std::vector<std::string> esperado = {
            "-w", "eDP-1", "-f", "60", "-a", "default_output",
            "-ac", "opus", "-q", "very_high",
            "-ipc", "/home/u/.cache/easy-screen-recorder/ipc.sock", "-o", "/tmp/x.mkv"};
        COMPROBAR(args == esperado);
    }
    {
        // «auto» no se pasa; un codec pedido si.
        auto a = base();
        a.codec_video = "hevc";
        const auto args = argumentos_gsr(a);
        COMPROBAR(std::find(args.begin(), args.end(), "-k") != args.end());
        COMPROBAR(std::find(args.begin(), args.end(), "hevc") != args.end());
        a.codec_video = "auto";
        const auto args2 = argumentos_gsr(a);
        COMPROBAR(std::find(args2.begin(), args2.end(), "-k") == args2.end());
    }
    {
        auto a = base();
        a.fuente = "region";
        a.region = "800x600+0+0";
        const auto args = argumentos_gsr(a);
        COMPROBAR(std::find(args.begin(), args.end(), "-region") != args.end());
    }
    {
        // El portal recuerda su sesion: sin esto, dialogo de permiso en cada
        // grabacion.
        auto a = base();
        a.fuente = "portal";
        const auto args = argumentos_gsr(a);
        COMPROBAR(std::find(args.begin(), args.end(), "-restore-portal-session") != args.end());
        // Y con un monitor no se pasa: no pinta nada ahi.
        const auto args_monitor = argumentos_gsr(base());
        COMPROBAR(std::find(args_monitor.begin(), args_monitor.end(),
                            "-restore-portal-session") == args_monitor.end());
    }
    {
        // Dos pistas de audio, dos -a.
        auto a = base();
        a.audios = {"default_output", "default_input"};
        const auto args = argumentos_gsr(a);
        COMPROBAR(std::count(args.begin(), args.end(), "-a") == 2);
    }

    // carpeta_videos lee user-dirs.dirs. Se prueba con un XDG_CONFIG_HOME
    // sintetico para no depender de la maquina.
    {
        const std::string dir = std::string(ESR_DIR_FIXTURES) + "/user-dirs";
        setenv("XDG_CONFIG_HOME", dir.c_str(), 1);
        setenv("HOME", "/home/prueba", 1);
        COMPROBAR_NOTA(carpeta_videos() == "/home/prueba/Vídeos", carpeta_videos());
        // Sin fichero de configuracion: el home, sin inventar carpeta.
        setenv("XDG_CONFIG_HOME", "/no/existe", 1);
        COMPROBAR(carpeta_videos() == "/home/prueba");
        unsetenv("XDG_CONFIG_HOME");
    }

    // El nombre por defecto: carpeta, prefijo y extension mkv.
    {
        const std::string n = nombre_por_defecto("/tmp/v");
        COMPROBAR(n.rfind("/tmp/v/easy-screen-recorder-", 0) == 0);
        COMPROBAR(extension_de(n) == "mkv");
    }

    return prueba::resumen("prueba_ajustes");
}
