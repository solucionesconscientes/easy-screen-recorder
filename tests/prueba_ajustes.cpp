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
            "-ac", "opus", "-q", "very_high", "-ffmpeg-opts", "flush_packets=1",
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
        COMPROBAR(!pista_mezclada(a.audios[0]));
    }
    {
        // Las dos fuentes MEZCLADAS: un solo -a con «|» dentro, que es la
        // sintaxis de GSR (gpu-screen-recorder.1, ejemplo de -a).
        auto a = base();
        a.audios = {"default_output|default_input"};
        COMPROBAR(pista_mezclada(a.audios[0]));
        const auto args = argumentos_gsr(a);
        COMPROBAR(std::count(args.begin(), args.end(), "-a") == 1);
        COMPROBAR(std::find(args.begin(), args.end(), "default_output|default_input") != args.end());
        COMPROBAR(validar(a).empty());  // con opus, que es el default
    }
    {
        // flac + mezcla: GSR lo cambiaria a opus por detras
        // (codec_select.c:186-191). Recibir otra cosa de lo pedido es error.
        auto a = base();
        a.codec_audio = "flac";
        a.audios = {"default_output|default_input"};
        COMPROBAR(algun_problema_contiene(validar(a), "flac no se respeta cuando se mezclan"));
        // Sin mezclar, flac en mkv sigue siendo valido.
        a.audios = {"default_output", "default_input"};
        COMPROBAR(validar(a).empty());
    }
    // Contenedor + codec de VIDEO. La tabla sale de meter cada codec en cada
    // contenedor con ffmpeg, y de grabaciones reales: h264 en webm no deja
    // fichero, GSR muere al escribir la cabecera.
    {
        COMPROBAR(familia_codec_video("hevc_10bit") == "hevc");
        COMPROBAR(familia_codec_video("hevc_hdr_vulkan") == "hevc");
        COMPROBAR(familia_codec_video("h265") == "hevc");
        COMPROBAR(familia_codec_video("av1_vulkan") == "av1");
        COMPROBAR(familia_codec_video("h264") == "h264");
        // «auto» no es un codec: es dejar elegir a GSR, y no tiene familia.
        COMPROBAR(familia_codec_video("auto").empty());
        COMPROBAR(familia_codec_video("").empty());
    }
    {
        // webm: solo vp8, vp9 y av1.
        COMPROBAR(!contenedor_admite_video("webm", "h264"));
        COMPROBAR(!contenedor_admite_video("webm", "hevc"));
        COMPROBAR(contenedor_admite_video("webm", "vp8"));
        COMPROBAR(contenedor_admite_video("webm", "av1_10bit"));
        // mp4: todo menos vp8.
        COMPROBAR(!contenedor_admite_video("mp4", "vp8"));
        COMPROBAR(contenedor_admite_video("mp4", "h264"));
        COMPROBAR(contenedor_admite_video("mp4", "hevc_10bit"));
        // mkv se lo traga todo, y «auto» vale en cualquiera: GSR mira el
        // contenedor antes de elegir (medido: auto en .webm da vp8).
        COMPROBAR(contenedor_admite_video("mkv", "vp8"));
        COMPROBAR(contenedor_admite_video("webm", "auto"));
        // Un contenedor sin medir no se bloquea: no se inventa una regla.
        COMPROBAR(contenedor_admite_video("ts", "h264"));
    }
    {
        // Y el selector de la UI no ofrece lo que no cabe.
        const std::vector<std::string> maquina = {"h264", "hevc", "vp8"};
        const auto en_webm = codecs_video_para("webm", maquina);
        COMPROBAR(en_webm.size() == 1 && en_webm[0] == "vp8");
        const auto en_mp4 = codecs_video_para("mp4", maquina);
        COMPROBAR(en_mp4.size() == 2 && en_mp4[0] == "h264" && en_mp4[1] == "hevc");
        COMPROBAR(codecs_video_para("mkv", maquina).size() == 3);
    }
    {
        // Y la validacion lo para antes de lanzar, tambien desde el CLI.
        auto a = base();
        a.salida = "/tmp/x.webm";
        a.codec_audio = "opus";
        a.codec_video = "h264";
        COMPROBAR(algun_problema_contiene(validar(a), "no admite video h264"));
        a.codec_video = "auto";
        COMPROBAR(validar(a).empty());
        a.salida = "/tmp/x.mp4";
        a.codec_video = "vp8";
        COMPROBAR(algun_problema_contiene(validar(a), "no admite video vp8"));
    }
    {
        // Y el selector de la UI no puede ni ofrecer flac al mezclar.
        COMPROBAR(codecs_audio_para("mkv").size() == 3);
        const auto con_mezcla = codecs_audio_para("mkv", true);
        COMPROBAR(std::find(con_mezcla.begin(), con_mezcla.end(), "flac") == con_mezcla.end());
        COMPROBAR(con_mezcla.size() == 2 && con_mezcla[0] == "opus");
    }

    {
        // El volcado forzado va SIEMPRE: es lo que decide si una grabacion
        // sobrevive a un apagon. Medido: sin el se recuperan 0 s de 10; con el,
        // 8,1. Si alguien lo quita, esta comprobacion se entera.
        const auto args = argumentos_gsr(base());
        const auto opts = std::find(args.begin(), args.end(), "-ffmpeg-opts");
        COMPROBAR(opts != args.end());
        COMPROBAR(opts + 1 != args.end() && *(opts + 1) == "flush_packets=1");
    }

    // --- La camara superpuesta -------------------------------------------
    // GSR la compone el mismo, en vivo y en un solo proceso: «-w» admite varias
    // fuentes unidas por «|» y cada una lleva sus opciones detras con «;».
    {
        auto a = base();
        COMPROBAR(fuente_gsr(a) == "eDP-1");  // sin camara, la fuente tal cual
        a.camara = "/dev/video0";
        COMPROBAR_NOTA(fuente_gsr(a) ==
                           "eDP-1|v4l2:/dev/video0;width=25%;x=73%;y=73%;"
                           "halign=start;valign=start;hflip=true",
                       fuente_gsr(a));
        // Colocada libremente: GSR convierte el porcentaje a pixeles contra el
        // tamaño del video. Verificado grabando con x=50%,y=10%.
        a.camara_x_pct = 50;
        a.camara_y_pct = 10;
        a.camara_espejo = false;
        a.camara_ancho_pct = 30;
        COMPROBAR_NOTA(fuente_gsr(a) ==
                           "eDP-1|v4l2:/dev/video0;width=30%;x=50%;y=10%;halign=start;valign=start",
                       fuente_gsr(a));
        // Y llega hasta los argumentos, en -w y no en otro sitio.
        const auto args = argumentos_gsr(a);
        const auto w = std::find(args.begin(), args.end(), "-w");
        COMPROBAR(w != args.end() && *(w + 1) == fuente_gsr(a));
    }
    {
        // La altura NO se pasa: sin ella GSR mantiene la proporcion de la
        // camara. Fijar las dos la deformaria.
        auto a = base();
        a.camara = "/dev/video0";
        COMPROBAR(fuente_gsr(a).find("height=") == std::string::npos);
    }
    {
        auto a = base();
        a.camara = "/dev/video0";
        a.camara_ancho_pct = 80;  // taparia la pantalla
        COMPROBAR(algun_problema_contiene(validar(a), "entre el 5 % y el 50 %"));
        a.camara_ancho_pct = 25;
        // Colocada mas alla del borde, GSR la recortaria sin avisar.
        a.camara_x_pct = 85;
        COMPROBAR(algun_problema_contiene(validar(a), "se sale por el lado"));
        a.camara_x_pct = 73;
        a.camara_y_pct = 140;
        COMPROBAR(algun_problema_contiene(validar(a), "de 0 a 100"));
        a.camara_y_pct = 73;
        a.camara = a.fuente;  // la camara no puede ser tambien la pantalla
        COMPROBAR(algun_problema_contiene(validar(a), "no puede ser ademas la fuente"));
    }

    // --- Modo de fotogramas y limite de resolucion -------------------------
    {
        auto a = base();
        a.modo_fotogramas = "content";
        a.limite_resolucion = "1920x1080";
        COMPROBAR(validar(a).empty());
        const auto args = argumentos_gsr(a);
        COMPROBAR(std::find(args.begin(), args.end(), "-fm") != args.end());
        COMPROBAR(std::find(args.begin(), args.end(), "content") != args.end());
        COMPROBAR(std::find(args.begin(), args.end(), "-s") != args.end());
        COMPROBAR(std::find(args.begin(), args.end(), "1920x1080") != args.end());
        // Sin pedirlos no ensucian la linea de comandos.
        const auto limpios = argumentos_gsr(base());
        COMPROBAR(std::find(limpios.begin(), limpios.end(), "-fm") == limpios.end());
        COMPROBAR(std::find(limpios.begin(), limpios.end(), "-s") == limpios.end());
    }
    {
        auto a = base();
        a.modo_fotogramas = "rapido";
        COMPROBAR(algun_problema_contiene(validar(a), "cfr, vfr o content"));
        a.modo_fotogramas.clear();
        a.limite_resolucion = "muy grande";
        COMPROBAR(algun_problema_contiene(validar(a), "AnchoxAlto"));
    }
    {
        // «content» solo hace algo en X11, o en Wayland por el portal. Medido:
        // en Wayland con un monitor GSR avisa y lo ignora.
        COMPROBAR(modo_content_efectivo("eDP-1", "x11"));
        COMPROBAR(!modo_content_efectivo("eDP-1", "wayland"));
        COMPROBAR(modo_content_efectivo("portal", "wayland"));
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
