// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/audio.hpp"

#include <algorithm>
#include <utility>
#include <cstdlib>

#include "comprobar.hpp"

using namespace esr;

namespace {

bool alguno_contiene(const std::vector<std::string>& v, std::string_view trozo) {
    return std::any_of(v.begin(), v.end(), [&](const std::string& p) {
        return p.find(trozo) != std::string::npos;
    });
}

}  // namespace

int main() {
    // Defaults: el audio del sistema, en opus y sin bitrate fijado.
    {
        const AjustesAudio a;
        COMPROBAR(a.dispositivo == "default_output");
        COMPROBAR(a.formato == "opus");
    }

    // Formato y extension tienen que casar: aqui no hay un GSR que cambie el
    // codec por detras, pero un .opus con flac dentro confundiria igual.
    {
        AjustesAudio a;
        a.salida = "/x/nota.opus";
        COMPROBAR(validar_audio(a).empty());
        a.salida = "/x/nota.ogg";
        COMPROBAR(validar_audio(a).empty());
        a.salida = "/x/nota.flac";
        COMPROBAR(alguno_contiene(validar_audio(a), "opus va en"));
    }
    {
        AjustesAudio a;
        a.formato = "flac";
        a.salida = "/x/nota.flac";
        COMPROBAR(validar_audio(a).empty());
        a.salida = "/x/nota.opus";
        COMPROBAR(alguno_contiene(validar_audio(a), "flac va en"));
    }
    // Los tres formatos nuevos, con su contenedor natural y con uno que no lo es.
    {
        AjustesAudio a;
        a.formato = "aac";
        a.salida = "/x/nota.m4a";
        COMPROBAR(validar_audio(a).empty());
        a.salida = "/x/nota.aac";
        COMPROBAR(validar_audio(a).empty());
        a.salida = "/x/nota.mka";
        COMPROBAR(validar_audio(a).empty());
        // m4a es el CONTENEDOR de aac, no un formato: pedirlo como formato falla.
        a.formato = "m4a";
        COMPROBAR(alguno_contiene(validar_audio(a), "desconocido"));
    }
    {
        AjustesAudio a;
        a.formato = "wav";
        a.salida = "/x/nota.wav";
        COMPROBAR(validar_audio(a).empty());
        a.salida = "/x/nota.mka";  // matroska no lleva pcm en esta tabla
        COMPROBAR(alguno_contiene(validar_audio(a), "wav va en"));
    }
    {
        AjustesAudio a;
        a.formato = "mp3";
        a.salida = "/x/nota.mp3";
        COMPROBAR(validar_audio(a).empty());
    }
    {
        AjustesAudio a;
        a.formato = "vorbis";  // no esta en la tabla
        a.salida = "/x/nota.ogg";
        const auto p = validar_audio(a);
        COMPROBAR(alguno_contiene(p, "desconocido"));
        // El mensaje enumera lo que si hay, para no obligar a mirar el --help.
        COMPROBAR(alguno_contiene(p, "opus, aac, flac, wav y mp3"));
    }

    // El bitrate: se acepta con perdida, se rechaza sin ella, y con rango.
    {
        AjustesAudio a;
        a.salida = "/x/nota.opus";
        a.bitrate_kbps = 128;
        COMPROBAR(validar_audio(a).empty());
        a.bitrate_kbps = 16;
        COMPROBAR(alguno_contiene(validar_audio(a), "entre 32 y 512"));
        a.bitrate_kbps = 9999;
        COMPROBAR(alguno_contiene(validar_audio(a), "entre 32 y 512"));
    }
    {
        // En flac y wav el bitrate no es un valor malo: es un ajuste que no
        // existe. Se rechaza en vez de aceptarlo y no usarlo, porque un ajuste
        // que se ignora en silencio es peor que uno que no esta.
        AjustesAudio a;
        a.formato = "flac";
        a.salida = "/x/nota.flac";
        a.bitrate_kbps = 128;
        COMPROBAR(alguno_contiene(validar_audio(a), "no tiene bitrate"));
        a.formato = "wav";
        a.salida = "/x/nota.wav";
        COMPROBAR(alguno_contiene(validar_audio(a), "no tiene bitrate"));
        // Y sin bitrate los dos valen.
        a.bitrate_kbps = 0;
        COMPROBAR(validar_audio(a).empty());
    }

    // La extension por defecto sale de la tabla, no de un ternario repetido.
    {
        COMPROBAR(extension_por_defecto("opus") == "opus");
        COMPROBAR(extension_por_defecto("aac") == "m4a");   // el contenedor, no el codec
        COMPROBAR(extension_por_defecto("flac") == "flac");
        COMPROBAR(extension_por_defecto("wav") == "wav");
        COMPROBAR(extension_por_defecto("mp3") == "mp3");
        // Un formato que no existe no revienta: lo rechaza validar_audio().
        COMPROBAR(extension_por_defecto("inventado") == "opus");
    }
    {
        COMPROBAR(audio_sin_perdida("flac"));
        COMPROBAR(audio_sin_perdida("wav"));
        COMPROBAR(!audio_sin_perdida("opus"));
        COMPROBAR(!audio_sin_perdida("aac"));
        COMPROBAR(!audio_sin_perdida("mp3"));
        COMPROBAR(formatos_audio().size() == 5);
        COMPROBAR(formatos_audio().front() == "opus");  // el recomendado, primero
    }
    {
        AjustesAudio a;  // sin salida
        COMPROBAR(alguno_contiene(validar_audio(a), "salida"));
    }

    // La linea de ffmpeg: desatendida (-nostdin) y con el codec pedido.
    {
        AjustesAudio a;
        a.salida = "/x/nota.opus";
        const auto args = argumentos_ffmpeg(a, "fuente.monitor");
        const std::vector<std::string> esperado = {"-nostdin", "-hide_banner", "-y",
                                                   "-f", "pulse", "-i", "fuente.monitor",
                                                   "-c:a", "libopus", "/x/nota.opus"};
        COMPROBAR(args == esperado);
    }
    {
        AjustesAudio a;
        a.formato = "flac";
        a.salida = "/x/nota.flac";
        const auto args = argumentos_ffmpeg(a, "micro");
        COMPROBAR(std::find(args.begin(), args.end(), "flac") != args.end());
    }
    // Cada formato con SU codificador. Comprobado ejecutandolo: los cinco
    // graban y ffprobe devuelve el codec esperado.
    {
        const std::vector<std::pair<std::string, std::string>> pares = {
            {"opus", "libopus"}, {"aac", "aac"}, {"flac", "flac"},
            {"wav", "pcm_s16le"}, {"mp3", "libmp3lame"},
        };
        for (const auto& [formato, codificador] : pares) {
            AjustesAudio a;
            a.formato = formato;
            a.salida = "/x/nota." + extension_por_defecto(formato);
            const auto args = argumentos_ffmpeg(a, "fuente");
            COMPROBAR_NOTA(std::find(args.begin(), args.end(), codificador) != args.end(),
                           formato + " deberia usar " + codificador);
        }
    }
    // -b:a solo cuando hay bitrate Y el formato lo admite.
    {
        AjustesAudio a;
        a.salida = "/x/nota.opus";
        auto sin = argumentos_ffmpeg(a, "fuente");
        COMPROBAR(std::find(sin.begin(), sin.end(), "-b:a") == sin.end());

        a.bitrate_kbps = 96;
        auto con = argumentos_ffmpeg(a, "fuente");
        const auto it = std::find(con.begin(), con.end(), "-b:a");
        COMPROBAR(it != con.end());
        COMPROBAR(it + 1 != con.end() && *(it + 1) == "96k");

        // En un formato sin perdida no se pasa aunque venga puesto: validar_audio
        // ya lo rechaza, y si alguien se salta la validacion no se cuela un
        // argumento que ffmpeg ignoraria.
        a.formato = "flac";
        a.salida = "/x/nota.flac";
        auto flac = argumentos_ffmpeg(a, "fuente");
        COMPROBAR(std::find(flac.begin(), flac.end(), "-b:a") == flac.end());
    }

    // Un nombre explicito de PipeWire no pasa por pactl: vuelve tal cual.
    {
        std::string motivo;
        COMPROBAR(resolver_dispositivo("alsa_input.algo", motivo) == "alsa_input.algo");
        COMPROBAR(motivo.empty());
    }

    // La sesion de audio: aparte de la de pantalla y jamas bajo /tmp.
    {
        setenv("HOME", "/home/prueba", 1);
        const auto s = sesion_audio_por_defecto();
        COMPROBAR(s.dir == "/home/prueba/.cache/easy-screen-recorder/sesion-audio");
        COMPROBAR(s.dir.rfind("/tmp/", 0) != 0);
    }

    return prueba::resumen("prueba_audio");
}
