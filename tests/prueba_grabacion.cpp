// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <clocale>
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

// Recortar, el espacio libre y descartar. Lo que se puede comprobar sin una
// grabacion de verdad es a quien dicen que no; el camino bueno se verifico
// grabando por CLI y midiendo con ffprobe (CHANGELOG).
void recortar_y_espacio() {
    std::string motivo;
    COMPROBAR(!recortar_grabacion("/no/existe.mkv", 1.0, 0.0, motivo));
    COMPROBAR(!motivo.empty());
    // Negativo no tiene sentido y no se intenta siquiera.
    motivo.clear();
    COMPROBAR(!recortar_grabacion("/no/existe.mkv", -1.0, 0.0, motivo));
    COMPROBAR(motivo.find("negativos") != std::string::npos);
    // Y no quitar nada es un exito que no toca el fichero: asi quien llame a
    // esto puede hacerlo siempre, sin mirar antes si hay algo que quitar.
    COMPROBAR(recortar_grabacion("/no/existe.mkv", 0.0, 0.0, motivo));

    // El espacio libre: la raiz siempre dice algo, y una ruta inventada no.
    COMPROBAR(espacio_libre_mb("/") > 0);
    COMPROBAR(espacio_libre_mb("/no/existe/de/ninguna/manera/x.mkv") == -1);

    // Los numeros NO miran el idioma del sistema.
    //
    // Este test existe por un fallo real: std::stod y atof usan la
    // configuracion regional, Qt la pone al arrancar la ventana, y en español
    // «2.967» se lee como 2. El recorte funcionaba desde el CLI, que no toca el
    // idioma, y se negaba desde la ventana diciendo que una grabacion de tres
    // segundos duraba dos. Se pone el idioma a mano para reproducirlo.
    {
        const char* antes = std::setlocale(LC_NUMERIC, nullptr);
        const std::string guardado = antes != nullptr ? antes : "C";
        // Si el sistema no tiene el idioma español instalado, esto no cambia
        // nada y el test sigue valiendo con el idioma que haya.
        std::setlocale(LC_NUMERIC, "es_ES.UTF-8");
        COMPROBAR(segundos_de("2.967") > 2.9);
        COMPROBAR(segundos_de("0.5") > 0.4);
        COMPROBAR(segundos_de("10") == 10.0);
        COMPROBAR(segundos_de("") == 0.0);
        std::setlocale(LC_NUMERIC, guardado.c_str());
    }

    // Descartar sin nada que descartar avisa y no rompe.
    SesionGrabacion s;
    s.dir = "/tmp/esr-sesion-que-no-existe";
    s.ruta_socket = s.dir + "/ipc.sock";
    s.ruta_pid = s.dir + "/gsr.pid";
    motivo.clear();
    COMPROBAR(descartar_grabacion(s, motivo).empty());
    COMPROBAR(!motivo.empty());
}

}  // namespace

int main() {
    diagnosticos();
    cliente_ipc();
    sesion();
    reducir();
    recortar_y_espacio();
    // --- recomendar_codec: la regla, sin GSR ---
    {
        VeredictoCodecs v;
        v.probado = true;
        v.bytes_por_s_h264 = 1000.0;
        // Sin hevc: h264 y sin cifras que comparar.
        auto r = recomendar_codec(v);
        COMPROBAR(r.codec == "h264");
        COMPROBAR(r.hevc_frente_a_h264_pct == 0);
        // hevc un 30 % mas pequeño: hevc.
        v.hevc_ofrecido = true;
        v.bytes_por_s_hevc = 700.0;
        r = recomendar_codec(v);
        COMPROBAR(r.codec == "hevc");
        COMPROBAR_NOTA(r.hevc_frente_a_h264_pct == -30, std::to_string(r.hevc_frente_a_h264_pct));
        // El mismo ahorro, pero el driver no declara lo que sabe: h264.
        v.hevc_a_ojo = true;
        r = recomendar_codec(v);
        COMPROBAR(r.codec == "h264");
        v.hevc_a_ojo = false;
        // hevc solo un 3 % mas pequeño: no vale la pena, h264.
        v.bytes_por_s_hevc = 970.0;
        COMPROBAR(recomendar_codec(v).codec == "h264");
        // hevc un 20 % mas grande: h264, y la cifra lo dice.
        v.bytes_por_s_hevc = 1200.0;
        r = recomendar_codec(v);
        COMPROBAR(r.codec == "h264");
        COMPROBAR(r.hevc_frente_a_h264_pct == 20);
        // El codec actual (vp8) sale un 50 % mas grande que el recomendado.
        v.codec_actual = "vp8";
        v.bytes_por_s_actual = 1500.0;
        r = recomendar_codec(v);
        COMPROBAR_NOTA(r.actual_frente_a_recomendado_pct == 50,
                       std::to_string(r.actual_frente_a_recomendado_pct));
        // Y si esa sonda fallo (-1), no se dice nada de el.
        v.bytes_por_s_actual = -1.0;
        COMPROBAR(recomendar_codec(v).actual_frente_a_recomendado_pct == 0);
    }

    return prueba::resumen("prueba_grabacion");
}
