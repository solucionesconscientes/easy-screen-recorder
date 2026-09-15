// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

namespace esr {

// El modo audio-only. Va por ffmpeg y PipeWire, no por GSR: GSR exige una
// fuente de video siempre (-w obligatorio, args_parser.c:536, comprobado
// ejecutandolo; docs/gsr-audio-only.md). Este backend no toca GSR y tiene
// que funcionar en una maquina que no lo tenga.
//
// Cinco formatos, y cada uno tiene un caso que los otros no cubren:
//
//   opus  el general. La mejor relacion calidad/tamano que hay
//   aac   compatibilidad. En .m4a lo abre cualquier cosa, tambien hardware viejo
//   flac  sin perdida y comprimido. Para archivar
//   wav   sin perdida y sin comprimir. Es lo que quiere un editor de audio
//   mp3   solo compatibilidad heredada. Peor que opus a igual tamano y peor
//         que aac; esta porque a veces te lo piden, no porque sea bueno
//
// En la Tanda 5 se descarto mp3 con el argumento de que no habia un caso que
// opus no cubriera. Sigue siendo cierto en calidad; lo que cubre es que a
// alguien le exijan un .mp3 y no pueda discutirlo.
//
// El formato es el CODEC, no el contenedor. El contenedor lo decide la
// extension de `salida` y validar_audio() comprueba que sean compatibles: por
// eso «m4a» no es un formato de esta lista, es donde va aac.
struct AjustesAudio {
    // «default_output» (el audio del sistema), «default_input» (el microfono) o
    // un nombre de fuente de PipeWire tal como lo lista pactl. Los dos primeros
    // son los mismos nombres que usa GSR, para no tener dos vocabularios.
    std::string dispositivo = "default_output";
    std::string formato = "opus";  // opus | aac | flac | wav | mp3
    std::string salida;            // sin ella: easy-screen-recorder-FECHA.<ext> en Musica
    // kbps. 0 = lo que decida el codificador, que es un valor sensato. No
    // aplica a los formatos sin perdida y validar_audio() lo rechaza ahi, en
    // vez de aceptarlo y no usarlo: un ajuste que se ignora en silencio es
    // peor que uno que no existe.
    int bitrate_kbps = 0;
};

// Los formatos que ofrece el modo solo-audio, en el orden en que conviene
// enseñarlos. Una sola lista: la UI y el CLI leen de aqui para no divergir.
const std::vector<std::string>& formatos_audio();

// Si el formato no pierde informacion, o sea si el bitrate no tiene sentido.
bool audio_sin_perdida(const std::string& formato);

// La extension natural de ese formato: la que lleva el fichero cuando el
// usuario no elige nombre. Existe porque antes esto era un
// `formato == "flac" ? "flac" : "opus"` repetido en la UI y en el CLI, y un
// ternario de dos ramas no sobrevive a tener cinco formatos.
std::string extension_por_defecto(const std::string& formato);

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
    std::string dir;       // ~/.cache/easy-screen-recorder/sesion-audio
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

}  // namespace esr
