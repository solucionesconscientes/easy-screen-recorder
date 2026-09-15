#include "esr/audio.hpp"

#include <algorithm>
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
    // Defaults: lo que suena, en opus. Decidido en la Tanda 5 (ESTADO.md).
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
    {
        AjustesAudio a;
        a.formato = "mp3";  // no se ofrece: obliga a otro codificador
        a.salida = "/x/nota.mp3";
        COMPROBAR(alguno_contiene(validar_audio(a), "desconocido"));
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
