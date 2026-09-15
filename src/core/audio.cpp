// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/audio.hpp"

#include <signal.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <thread>

#include "esr/ajustes.hpp"
#include "esr/proceso.hpp"

namespace esr {
namespace {

std::string primera_linea(const std::string& texto) {
    const auto salto = texto.find('\n');
    return salto == std::string::npos ? texto : texto.substr(0, salto);
}

long leer_pid(const std::string& ruta) {
    std::ifstream f(ruta);
    long pid = -1;
    f >> pid;
    return f ? pid : -1;
}

}  // namespace

std::vector<std::string> validar_audio(const AjustesAudio& a) {
    std::vector<std::string> problemas;
    if (a.dispositivo.empty()) problemas.push_back("falta el dispositivo de audio");
    if (a.salida.empty()) {
        problemas.push_back("falta el fichero de salida");
        return problemas;
    }

    const std::string ext = extension_de(a.salida);
    if (a.formato == "opus") {
        if (ext != "opus" && ext != "ogg" && ext != "mka") {
            problemas.push_back("opus va en .opus, .ogg o .mka; la salida acaba en «." + ext + "»");
        }
    } else if (a.formato == "flac") {
        if (ext != "flac" && ext != "mka") {
            problemas.push_back("flac va en .flac o .mka; la salida acaba en «." + ext + "»");
        }
    } else {
        problemas.push_back("formato desconocido «" + a.formato +
                            "»: el modo audio-only ofrece opus y flac");
    }
    return problemas;
}

std::string resolver_dispositivo(const std::string& dispositivo, std::string& motivo) {
    if (dispositivo != "default_output" && dispositivo != "default_input") {
        return dispositivo;  // un nombre de PipeWire, tal cual
    }

    // pactl es la interfaz pulse de PipeWire y esta en cualquier escritorio
    // con pipewire-pulse; es lo mismo que usa GSR por dentro. Sin el no se
    // pueden resolver los nombres comodos, pero uno explicito sigue valiendo.
    const bool salida_del_sistema = dispositivo == "default_output";
    const auto r = ejecutar("pactl", {salida_del_sistema ? "get-default-sink" : "get-default-source"});
    if (!r.ejecutado || r.codigo != 0) {
        motivo = "no se pudo preguntar a pactl por el dispositivo por defecto";
        if (!r.ejecutado) motivo += " (" + r.motivo + ")";
        motivo += ". Elige uno explicito de «easy-screen-recorder-cli dispositivos»";
        return {};
    }
    std::string nombre = primera_linea(r.salida);
    while (!nombre.empty() && (nombre.back() == '\n' || nombre.back() == '\r')) nombre.pop_back();
    if (nombre.empty()) {
        motivo = "pactl no devolvio ningun dispositivo por defecto";
        return {};
    }
    // Lo que suena por un sink se captura por su fuente espejo «.monitor».
    return salida_del_sistema ? nombre + ".monitor" : nombre;
}

std::vector<std::string> argumentos_ffmpeg(const AjustesAudio& a,
                                           const std::string& fuente_pipewire) {
    // -nostdin: ffmpeg desatendido no tiene terminal que atender.
    // -y: la ruta la generamos nosotros; si existe, es una repeticion querida.
    std::vector<std::string> args = {"-nostdin", "-hide_banner", "-y",
                                     "-f", "pulse", "-i", fuente_pipewire};
    if (a.formato == "opus") {
        args.insert(args.end(), {"-c:a", "libopus"});
    } else {
        args.insert(args.end(), {"-c:a", "flac"});
    }
    args.push_back(a.salida);
    return args;
}

std::vector<FuenteAudio> fuentes_audio_pactl(std::string& motivo) {
    const auto r = ejecutar("pactl", {"list", "short", "sources"});
    if (!r.ejecutado || r.codigo != 0) {
        motivo = "pactl no responde";
        if (!r.ejecutado) motivo = r.motivo;
        return {};
    }
    // Formato: «id\tnombre\tdriver\tformato\testado», una fuente por linea.
    std::vector<FuenteAudio> fuentes;
    std::string_view resto = r.salida;
    while (!resto.empty()) {
        const auto salto = resto.find('\n');
        const std::string_view linea = resto.substr(0, salto);
        resto = salto == std::string_view::npos ? std::string_view{} : resto.substr(salto + 1);
        const auto t1 = linea.find('\t');
        if (t1 == std::string_view::npos) continue;
        const auto t2 = linea.find('\t', t1 + 1);
        FuenteAudio f;
        f.nombre = std::string(linea.substr(t1 + 1, t2 == std::string_view::npos
                                                        ? std::string_view::npos
                                                        : t2 - t1 - 1));
        if (t2 != std::string_view::npos) f.detalle = std::string(linea.substr(t2 + 1));
        if (!f.nombre.empty()) fuentes.push_back(std::move(f));
    }
    return fuentes;
}

SesionAudio sesion_audio_por_defecto() {
    const char* hogar = std::getenv("HOME");
    const std::string casa = (hogar != nullptr && hogar[0] != '\0') ? hogar : ".";
    SesionAudio s;
    s.dir = casa + "/.cache/easy-screen-recorder/sesion-audio";
    s.ruta_pid = s.dir + "/ffmpeg.pid";
    s.ruta_log = s.dir + "/ffmpeg.log";
    s.ruta_destino = s.dir + "/destino.txt";
    s.ruta_inicio = s.dir + "/inicio.txt";
    return s;
}

bool audio_en_marcha(const SesionAudio& sesion) {
    return proceso_vivo(leer_pid(sesion.ruta_pid));
}

ResultadoAudio empezar_audio(const AjustesAudio& a, const SesionAudio& sesion) {
    ResultadoAudio r;

    if (audio_en_marcha(sesion)) {
        r.motivo = "ya hay una grabacion de audio en marcha; parala con «easy-screen-recorder-cli parar»";
        return r;
    }

    const auto problemas = validar_audio(a);
    if (!problemas.empty()) {
        r.motivo = "los ajustes no valen:";
        for (const auto& p : problemas) r.motivo += "\n  - " + p;
        return r;
    }

    std::string motivo;
    const std::string fuente = resolver_dispositivo(a.dispositivo, motivo);
    if (fuente.empty()) {
        r.motivo = motivo;
        return r;
    }

    std::error_code ec;
    std::filesystem::create_directories(sesion.dir, ec);
    if (ec) {
        r.motivo = "no se puede crear " + sesion.dir + ": " + ec.message();
        return r;
    }
    const auto dir_salida = std::filesystem::path(a.salida).parent_path();
    if (!dir_salida.empty() && !std::filesystem::is_directory(dir_salida, ec)) {
        r.motivo = "la carpeta de destino no existe: " + dir_salida.string();
        return r;
    }

    const auto lanzado =
        lanzar_desatendido("ffmpeg", argumentos_ffmpeg(a, fuente), sesion.ruta_log);
    if (!lanzado.ejecutado) {
        r.motivo = lanzado.motivo;
        return r;
    }

    std::ofstream(sesion.ruta_pid) << lanzado.pid << "\n";
    std::ofstream(sesion.ruta_destino) << a.salida << "\n";
    std::ofstream(sesion.ruta_inicio) << std::time(nullptr) << "\n";

    // Darle un momento: si el dispositivo no existe, ffmpeg muere ya mismo y
    // es mejor decirlo aqui que dejar un "grabando" falso.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (!proceso_vivo(lanzado.pid)) {
        std::ifstream log(sesion.ruta_log);
        std::string todo((std::istreambuf_iterator<char>(log)), std::istreambuf_iterator<char>());
        std::string cola = todo.size() > 400 ? todo.substr(todo.size() - 400) : todo;
        while (!cola.empty() && cola.back() == '\n') cola.pop_back();
        r.motivo = "ffmpeg murio al arrancar. El final de su log:\n" + cola;
        std::filesystem::remove(sesion.ruta_pid, ec);
        return r;
    }

    r.bien = true;
    r.ruta_fichero = a.salida;
    return r;
}

ResultadoAudio parar_audio(const SesionAudio& sesion) {
    ResultadoAudio r;

    const long pid = leer_pid(sesion.ruta_pid);
    if (!proceso_vivo(pid)) {
        r.motivo = "no hay ninguna grabacion de audio en marcha";
        return r;
    }

    // SIGINT es la parada limpia de ffmpeg: escribe la cola del contenedor y
    // sale. SIGKILL dejaria el fichero a medio cerrar.
    if (::kill(static_cast<pid_t>(pid), SIGINT) != 0) {
        r.motivo = "no se pudo parar el proceso " + std::to_string(pid);
        return r;
    }

    // Esperar a que cierre. No es nuestro hijo directo tras «easy-screen-recorder-cli audio»,
    // asi que se sondea. Diez segundos dan de sobra para escribir una cola.
    for (int esperado = 0; esperado < 10000; esperado += 100) {
        if (!proceso_vivo(pid)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (proceso_vivo(pid)) {
        r.motivo = "ffmpeg no termina; mira " + sesion.ruta_log;
        return r;
    }

    std::ifstream destino(sesion.ruta_destino);
    std::getline(destino, r.ruta_fichero);

    std::error_code ec;
    if (r.ruta_fichero.empty() || !std::filesystem::exists(r.ruta_fichero, ec)) {
        r.motivo = "ffmpeg termino pero el fichero no aparece";
        if (!r.ruta_fichero.empty()) r.motivo += ": " + r.ruta_fichero;
        return r;
    }

    r.bien = true;
    std::filesystem::remove(sesion.ruta_pid, ec);
    std::filesystem::remove(sesion.ruta_destino, ec);
    std::filesystem::remove(sesion.ruta_inicio, ec);
    return r;
}

}  // namespace esr
