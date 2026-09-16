// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/grabacion.hpp"

#include <unistd.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <thread>

#include "esr/entorno.hpp"
#include "esr/ipc.hpp"
#include "esr/proceso.hpp"

namespace esr {
namespace {

// Cuanto se espera a que el socket IPC aparezca tras lanzar GSR. Con el
// flatpak frio la primera sonda de esta maquina tardo hasta 746 ms; el portal
// ademas abre un dialogo que el usuario tiene que aceptar, asi que corto no
// puede ser.
constexpr int kEsperaSocketMs = 15000;
constexpr int kPasoEsperaMs = 100;

// Limite para los comandos con respuesta inmediata. gsr-cli usa 10 s
// (tools/gsr-cli/main.c:18-19); se copia.
constexpr int kLimiteInmediatoMs = 10000;

std::string leer_pid(const std::string& ruta) {
    std::ifstream f(ruta);
    std::string linea;
    std::getline(f, linea);
    return linea;
}

// Los motivos de error que el IPC devuelve en ingles, traducidos. Los textos
// exactos salen de src/cli/ipc.c:424-432 y 643-660 de GSR 6.0.0.
std::string traducir_error_ipc(const std::string& data) {
    if (data.find("already stopping") != std::string::npos) {
        return "la grabacion ya se esta parando; la primera peticion sigue su curso";
    }
    if (data.find("exited before the request finished") != std::string::npos) {
        return "el grabador termino antes de completar la peticion; la grabacion puede no haberse guardado";
    }
    if (data.find("failed to save") != std::string::npos) {
        return "no se pudo guardar la grabacion (respuesta de GSR: " + data + ")";
    }
    if (data.find("option -r is required") != std::string::npos) {
        return "eso solo vale en modo replay, y esta grabacion no lo es";
    }
    return "el grabador respondio con un error: " + data;
}

}  // namespace

long inicio_grabacion(const std::string& ruta_inicio) {
    std::ifstream f(ruta_inicio);
    long t = 0;
    f >> t;
    return f ? t : 0;
}

SesionGrabacion sesion_por_defecto() {
    const char* hogar = std::getenv("HOME");
    const std::string casa = (hogar != nullptr && hogar[0] != '\0') ? hogar : ".";
    SesionGrabacion s;
    s.dir = casa + "/.cache/easy-screen-recorder/sesion";
    s.ruta_socket = s.dir + "/ipc.sock";
    s.ruta_log = s.dir + "/gsr.log";
    s.ruta_pid = s.dir + "/gsr.pid";
    s.ruta_inicio = s.dir + "/inicio.txt";
    return s;
}

std::string diagnostico_de_log(const std::string& ruta_log) {
    std::ifstream f(ruta_log);
    if (!f) return "y su log no se puede leer (" + ruta_log + ")";

    std::string todo((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // Fallos con causa conocida, comprobados en esta maquina (ESTADO.md de la
    // Tanda 2 y docs/gsr-audio-only.md).
    if (todo.find("no /dev/dri/card") != std::string::npos) {
        return "no hay ningun plano de video activo. Suele significar pantalla apagada o "
               "suspendida: enciendela y repite. Si persiste, la fuente pedida no existe";
    }
    if (todo.find("missing argument") != std::string::npos) {
        return "GSR rechazo los argumentos, y eso es un fallo de easy-screen-recorder-cli, no tuyo. "
               "Copia el log de " + ruta_log + " en un informe de error";
    }
    if (todo.find("No space left") != std::string::npos) {
        return "no queda espacio en disco para grabar";
    }

    // Sin causa conocida: las ultimas lineas del log tal cual, que es mejor
    // que un diagnostico inventado.
    std::string cola;
    std::size_t desde = todo.size();
    int lineas = 0;
    while (desde > 0 && lineas < 5) {
        --desde;
        if (todo[desde] == '\n' && desde + 1 < todo.size()) ++lineas;
    }
    cola = todo.substr(desde == 0 ? 0 : desde + 1);
    while (!cola.empty() && (cola.back() == '\n' || cola.back() == ' ')) cola.pop_back();
    if (cola.empty()) return "y su log quedo vacio: murio antes de decir nada";
    return "esto es el final de su log:\n" + cola;
}

ResultadoLanzamiento empezar_grabacion(const AjustesGrabacion& ajustes,
                                       const SesionGrabacion& sesion) {
    ResultadoLanzamiento r;

    if (grabacion_en_marcha(sesion)) {
        r.motivo = "ya hay una grabacion en marcha; parala con «easy-screen-recorder-cli parar»";
        return r;
    }

    const auto problemas = validar(ajustes);
    if (!problemas.empty()) {
        r.motivo = "los ajustes no valen:";
        for (const auto& p : problemas) r.motivo += "\n  - " + p;
        return r;
    }

    const auto inv = localizar_gsr("gpu-screen-recorder");
    if (!inv) {
        r.motivo = dentro_de_sandbox()
                       ? "gpu-screen-recorder no esta en el anfitrion. Instalalo ahi, no "
                         "dentro del sandbox: la captura la hace un proceso del anfitrion"
                       : "gpu-screen-recorder no esta ni en PATH ni como flatpak";
        return r;
    }

    // La trampa de /tmp, que ahora tiene tres versiones del mismo problema:
    // el fichero lo escribe un proceso que NO comparte /tmp con nosotros. Con
    // GSR en flatpak, su /tmp es suyo; desde dentro de un sandbox, el /tmp que
    // ve GSR es el del anfitrion y el nuestro es privado. En los dos casos el
    // video acaba en un /tmp que nadie va a mirar.
    if (escribe_fuera_de_nuestro_sandbox(inv->origen) &&
        ajustes.salida.rfind("/tmp/", 0) == 0) {
        r.motivo = "el fichero no puede ir a /tmp: lo escribe otro proceso y su /tmp no es "
                   "el mismo que el tuyo, asi que el video se quedaria donde no lo ves. "
                   "Usa una ruta bajo tu home";
        return r;
    }

    std::error_code ec;
    std::filesystem::create_directories(sesion.dir, ec);
    if (ec) {
        r.motivo = "no se puede crear " + sesion.dir + ": " + ec.message();
        return r;
    }
    const auto dir_salida = std::filesystem::path(ajustes.salida).parent_path();
    if (!dir_salida.empty() && !std::filesystem::is_directory(dir_salida, ec)) {
        r.motivo = "la carpeta de destino no existe: " + dir_salida.string();
        return r;
    }

    // Un socket huerfano de un GSR muerto lo limpia GSR solo; cualquier otro
    // fichero en esa ruta le impide arrancar (src/cli/ipc.c:767-784). Mejor
    // decirlo aqui que dejar que muera con su error criptico en el log.
    if (std::filesystem::exists(sesion.ruta_socket, ec) &&
        !std::filesystem::is_socket(sesion.ruta_socket, ec)) {
        r.motivo = "en " + sesion.ruta_socket + " hay un fichero que no es un socket; quitalo";
        return r;
    }

    AjustesGrabacion con_ipc = ajustes;
    con_ipc.ruta_socket = sesion.ruta_socket;
    std::vector<std::string> args = inv->prefijo;
    const auto args_gsr = argumentos_gsr(con_ipc);
    args.insert(args.end(), args_gsr.begin(), args_gsr.end());

    const auto lanzado = lanzar_desatendido(inv->programa, args, sesion.ruta_log);
    if (!lanzado.ejecutado) {
        r.motivo = lanzado.motivo;
        return r;
    }

    std::ofstream(sesion.ruta_pid) << lanzado.pid << "\n";
    std::ofstream(sesion.ruta_inicio) << std::time(nullptr) << "\n";

    // Esperar a que el socket escuche. Si GSR muere antes, el log dice por que.
    for (int esperado = 0; esperado < kEsperaSocketMs; esperado += kPasoEsperaMs) {
        if (grabacion_en_marcha(sesion)) {
            r.en_marcha = true;
            r.pid = lanzado.pid;
            return r;
        }
        if (!proceso_vivo(lanzado.pid)) {
            r.motivo = "el grabador murio al arrancar; " + diagnostico_de_log(sesion.ruta_log);
            std::filesystem::remove(sesion.ruta_pid, ec);
            return r;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPasoEsperaMs));
    }

    r.motivo = "el grabador no abrio su socket IPC en " + std::to_string(kEsperaSocketMs / 1000) +
               " s; sigue vivo (pid " + std::to_string(lanzado.pid) +
               "). Si era el portal, puede estar esperando a que aceptes el dialogo";
    return r;
}

ResultadoParada parar_grabacion(const SesionGrabacion& sesion) {
    ResultadoParada r;

    std::string motivo;
    auto conexion = ConexionIpc::conectar(sesion.ruta_socket, &motivo);
    if (!conexion) {
        r.motivo = "no hay ninguna grabacion en marcha";
        // Si quedo un pid apuntado y el proceso existe, decirlo: socket caido
        // con grabador vivo es un estado que el usuario debe conocer.
        const std::string pid_texto = leer_pid(sesion.ruta_pid);
        if (!pid_texto.empty() && proceso_vivo(std::atol(pid_texto.c_str()))) {
            r.motivo = "el socket IPC no responde pero el grabador (pid " + pid_texto +
                       ") sigue vivo. Algo va mal: mira " + sesion.ruta_log;
        }
        return r;
    }

    // Sin limite de tiempo: la respuesta llega con el fichero ya escrito.
    const auto respuesta = conexion->pedir("stop", -1, &motivo);
    if (!respuesta) {
        r.motivo = "no se pudo hablar con el grabador: " + motivo;
        return r;
    }
    if (!respuesta->ok) {
        r.motivo = traducir_error_ipc(respuesta->data);
        return r;
    }

    r.parado = true;
    r.ruta_fichero = respuesta->tiene_data ? respuesta->data : "";

    std::error_code ec;
    std::filesystem::remove(sesion.ruta_pid, ec);
    std::filesystem::remove(sesion.ruta_inicio, ec);
    return r;
}

bool grabacion_en_marcha(const SesionGrabacion& sesion) {
    return ConexionIpc::conectar(sesion.ruta_socket).has_value();
}

bool poner_pausa(const SesionGrabacion& sesion, bool pausada, std::string& motivo) {
    auto conexion = ConexionIpc::conectar(sesion.ruta_socket, &motivo);
    if (!conexion) {
        motivo = "no hay ninguna grabacion en marcha";
        return false;
    }
    const auto respuesta = conexion->pedir("set-paused", pausada, kLimiteInmediatoMs, &motivo);
    if (!respuesta) {
        motivo = "no se pudo hablar con el grabador: " + motivo;
        return false;
    }
    if (!respuesta->ok) {
        motivo = traducir_error_ipc(respuesta->data);
        return false;
    }
    return true;
}

}  // namespace esr
