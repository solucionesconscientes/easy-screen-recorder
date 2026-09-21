// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include "esr/grabacion.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <thread>

#include "esr/ipc.hpp"
#include "comprobar.hpp"

using namespace esr;

namespace {

std::string escribir_log(const std::string& nombre, const std::string& contenido) {
    const std::string ruta = nombre;  // cwd del test: build/tests
    std::ofstream(ruta) << contenido;
    return ruta;
}

// El catalogo de diagnosticos: cada fallo conocido con su causa en español.
void diagnosticos() {
    {
        const auto d = diagnostico_de_log(
            escribir_log("log-dri.txt", "gsr error: no /dev/dri/cardX device found\n"));
        COMPROBAR_NOTA(d.find("pantalla apagada") != std::string::npos, d);
    }
    {
        const auto d = diagnostico_de_log(
            escribir_log("log-arg.txt", "gsr error: missing argument '-w'\n"));
        COMPROBAR_NOTA(d.find("fallo de easy-screen-recorder-cli") != std::string::npos, d);
    }
    {
        const auto d = diagnostico_de_log(
            escribir_log("log-disco.txt", "Error writing: No space left on device\n"));
        COMPROBAR_NOTA(d.find("espacio en disco") != std::string::npos, d);
    }
    {
        // Fallo sin causa conocida: el final del log tal cual, no un invento.
        const auto d = diagnostico_de_log(
            escribir_log("log-raro.txt", "linea 1\nun error nunca visto\n"));
        COMPROBAR_NOTA(d.find("un error nunca visto") != std::string::npos, d);
    }
    {
        const auto d = diagnostico_de_log(escribir_log("log-vacio.txt", ""));
        COMPROBAR_NOTA(d.find("murio antes de decir nada") != std::string::npos, d);
    }
    {
        const auto d = diagnostico_de_log("no-existe.txt");
        COMPROBAR_NOTA(d.find("no se puede leer") != std::string::npos, d);
    }
}

// Un servidor de un solo uso que habla como el de GSR: lee hasta el salto de
// linea y contesta la linea dada. Sirve para probar la transaccion entera por
// un socket de verdad sin necesitar GSR.
void servidor_de_una_respuesta(int fd_escucha, std::string respuesta) {
    const int fd = ::accept(fd_escucha, nullptr, nullptr);
    if (fd < 0) return;
    char c = 0;
    while (::read(fd, &c, 1) == 1 && c != '\n') {
    }
    // El servidor de prueba manda la linea entera de una vez; en un socket
    // local con este tamaño no hay escritura parcial que gestionar.
    if (::write(fd, respuesta.data(), respuesta.size()) !=
        static_cast<ssize_t>(respuesta.size())) {
        std::fprintf(stderr, "el servidor de prueba no pudo responder entero\n");
    }
    ::close(fd);
}

int escuchar_en(const std::string& ruta) {
    ::unlink(ruta.c_str());
    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;
    sockaddr_un direccion{};
    direccion.sun_family = AF_UNIX;
    std::memcpy(direccion.sun_path, ruta.c_str(), ruta.size() + 1);
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&direccion), sizeof(direccion)) != 0 ||
        ::listen(fd, 1) != 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

void cliente_ipc() {
    // Sin nadie escuchando: nullopt con motivo util.
    {
        std::string motivo;
        COMPROBAR(!ConexionIpc::conectar("no-hay-nadie.sock", &motivo).has_value());
        COMPROBAR(!motivo.empty());
    }
    // Ruta imposible de largo: el mismo limite contra el que valida GSR.
    {
        std::string motivo;
        COMPROBAR(!ConexionIpc::conectar(std::string(200, 'x'), &motivo).has_value());
        COMPROBAR_NOTA(motivo.find("demasiado larga") != std::string::npos, motivo);
    }

    // Una transaccion entera: stop que contesta con la ruta, como el GSR real
    // en la transcripcion de docs/gsr-ipc.md.
    {
        const std::string ruta = "prueba-ipc.sock";
        const int fd = escuchar_en(ruta);
        COMPROBAR_NOTA(fd >= 0, "no se pudo escuchar en el socket de prueba");
        std::thread servidor(servidor_de_una_respuesta, fd,
                             "{\"id\":1,\"result\":\"ok\",\"data\":\"/tmp/v.mkv\"}\n");

        auto conexion = ConexionIpc::conectar(ruta);
        COMPROBAR(conexion.has_value());
        std::string motivo;
        const auto r = conexion->pedir("stop", 5000, &motivo);
        COMPROBAR_NOTA(r.has_value(), motivo);
        COMPROBAR(r && r->ok && r->data == "/tmp/v.mkv");

        servidor.join();
        ::close(fd);
        ::unlink(ruta.c_str());
    }
    // Un error del protocolo llega como error, no como excepcion ni como ok.
    {
        const std::string ruta = "prueba-ipc-2.sock";
        const int fd = escuchar_en(ruta);
        COMPROBAR(fd >= 0);
        std::thread servidor(servidor_de_una_respuesta, fd,
                             "{\"id\":1,\"result\":\"error\",\"data\":\"GPU Screen Recorder is already stopping\"}\n");
        auto conexion = ConexionIpc::conectar(ruta);
        COMPROBAR(conexion.has_value());
        const auto r = conexion->pedir("stop", 5000);
        COMPROBAR(r && !r->ok);
        COMPROBAR(r && r->data.find("already stopping") != std::string::npos);
        servidor.join();
        ::close(fd);
        ::unlink(ruta.c_str());
    }
}

void sesion() {
    setenv("HOME", "/home/prueba", 1);
    const auto s = sesion_por_defecto();
    COMPROBAR(s.dir == "/home/prueba/.cache/easy-screen-recorder/sesion");
    COMPROBAR(s.ruta_socket == s.dir + "/ipc.sock");
    // El socket por defecto cabe de sobra en sun_path (107).
    COMPROBAR(s.ruta_socket.size() < 100);

    // Y jamas bajo /tmp: es la trampa del flatpak.
    COMPROBAR(s.dir.rfind("/tmp/", 0) != 0);
}

// Reducir: lo que se puede comprobar sin un video de verdad delante es a QUIEN
// dice que no, que es donde estan las decisiones. Lo que si encoge se verifico
// grabando y reduciendo con el CLI (docs/post-proceso.md).
void reducir() {
    std::string motivo;
    COMPROBAR(!se_puede_reducir("/no/existe/de/ninguna/manera.mkv", motivo));
    COMPROBAR(motivo.find("no existe") != std::string::npos);

    // Un fichero que existe pero no es video: ffprobe no encuentra pista de
    // video y se rechaza sin intentar nada.
    const std::string vacio = "/tmp/esr-prueba-reducir.mkv";
    { std::ofstream f(vacio); f << "esto no es un video\n"; }
    motivo.clear();
    COMPROBAR(!se_puede_reducir(vacio, motivo));
    COMPROBAR(!motivo.empty());
    // Y sobre eso, reducir tampoco toca nada ni deja restos.
    const auto r = reducir_grabacion(vacio, NivelReduccion::Normal);
    COMPROBAR(!r.hecho);
    COMPROBAR(!r.motivo.empty());
    COMPROBAR(std::filesystem::exists(vacio));
    COMPROBAR(!std::filesystem::exists("/tmp/esr-prueba-reducir.reduciendo.mkv"));
    std::filesystem::remove(vacio);
}

}  // namespace

int main() {
    diagnosticos();
    cliente_ipc();
    sesion();
    reducir();
    return prueba::resumen("prueba_grabacion");
}
