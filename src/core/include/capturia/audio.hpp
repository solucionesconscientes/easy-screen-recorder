#pragma once

#include <string>
#include <vector>

namespace capturia {

// El modo audio-only. Va por ffmpeg y PipeWire, no por GSR: GSR exige una
// fuente de video siempre (-w obligatorio, args_parser.c:536, comprobado
// ejecutandolo; docs/gsr-audio-only.md). Este backend no toca GSR y tiene
// que funcionar en una maquina que no lo tenga.
//
// Formatos: opus para el caso general y flac para calidad. MP3 no: obliga a
// otro codificador y no hay un caso que opus no cubra. Decidido en la
// Tanda 5, ESTADO.md.
struct AjustesAudio {
    // «default_output» (lo que suena), «default_input» (el microfono) o un
    // nombre de fuente de PipeWire tal como lo lista pactl. Los dos primeros
    // son los mismos nombres que usa GSR, para no tener dos vocabularios.
    std::string dispositivo = "default_output";
    std::string formato = "opus";  // opus | flac
    std::string salida;            // sin ella: capturia-FECHA.opus en Musica... no: Videos
};

std::vector<std::string> validar_audio(const AjustesAudio& a);

// Traduce «default_output»/«default_input» al nombre real de la fuente de
// PipeWire, preguntando a pactl. Un nombre explicito pasa tal cual.
// Devuelve vacio, con el motivo relleno, si no se puede resolver.
std::string resolver_dispositivo(const std::string& dispositivo, std::string& motivo);

// Los argumentos de ffmpeg para grabar ese dispositivo en ese formato.
std::vector<std::string> argumentos_ffmpeg(const AjustesAudio& a,
                                           const std::string& fuente_pipewire);

// Fuentes de audio segun pactl, sin GSR: «nombre» y descripcion breve.
struct FuenteAudio {
    std::string nombre;
    std::string detalle;
};
std::vector<FuenteAudio> fuentes_audio_pactl(std::string& motivo);

// La sesion del audio-only. Aparte de la de pantalla: son procesos y
// protocolos distintos (ffmpeg se para con SIGINT, GSR por IPC).
struct SesionAudio {
    std::string dir;       // ~/.cache/capturia/sesion-audio
    std::string ruta_pid;
    std::string ruta_log;
    std::string ruta_destino;  // apunta el fichero que se esta grabando
    std::string ruta_inicio;   // epoch de cuando empezo, para el reloj de la UI
};
SesionAudio sesion_audio_por_defecto();

struct ResultadoAudio {
    bool bien = false;
    std::string ruta_fichero;
    std::string motivo;
};

// Lanza ffmpeg desatendido. Falla si ya hay una grabacion de audio en marcha.
ResultadoAudio empezar_audio(const AjustesAudio& a, const SesionAudio& sesion);

// SIGINT a ffmpeg, que cierra el contenedor con su cola, y espera a que
// termine. Sin la espera el fichero podria estar aun a medio cerrar.
ResultadoAudio parar_audio(const SesionAudio& sesion);

bool audio_en_marcha(const SesionAudio& sesion);

}  // namespace capturia
