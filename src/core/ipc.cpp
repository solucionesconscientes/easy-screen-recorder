// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/ipc.hpp"

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace esr {

ConexionIpc::ConexionIpc(ConexionIpc&& otra) noexcept
    : fd_(otra.fd_), siguiente_id_(otra.siguiente_id_) {
    otra.fd_ = -1;
}

ConexionIpc& ConexionIpc::operator=(ConexionIpc&& otra) noexcept {
    if (this != &otra) {
        cerrar();
        fd_ = otra.fd_;
        siguiente_id_ = otra.siguiente_id_;
        otra.fd_ = -1;
    }
    return *this;
}

ConexionIpc::~ConexionIpc() { cerrar(); }

void ConexionIpc::cerrar() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

std::optional<ConexionIpc> ConexionIpc::conectar(const std::string& ruta_socket,
                                                 std::string* motivo) {
    sockaddr_un direccion{};
    direccion.sun_family = AF_UNIX;
    // El limite de sun_path es el mismo contra el que GSR valida la suya
    // (src/cli/ipc.c:848-851): 107 caracteres utiles en Linux.
    if (ruta_socket.size() >= sizeof(direccion.sun_path)) {
        if (motivo) {
            *motivo = "la ruta del socket es demasiado larga (" +
                      std::to_string(ruta_socket.size()) + " caracteres, maximo " +
                      std::to_string(sizeof(direccion.sun_path) - 1) + ")";
        }
        return std::nullopt;
    }
    std::memcpy(direccion.sun_path, ruta_socket.c_str(), ruta_socket.size() + 1);

    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        if (motivo) *motivo = std::string("socket: ") + std::strerror(errno);
        return std::nullopt;
    }
    if (::connect(fd, reinterpret_cast<const sockaddr*>(&direccion), sizeof(direccion)) != 0) {
        if (motivo) *motivo = std::string("connect: ") + std::strerror(errno);
        ::close(fd);
        return std::nullopt;
    }

    ConexionIpc c;
    c.fd_ = fd;
    return c;
}

std::optional<RespuestaIpc> ConexionIpc::pedir(std::string_view nombre, int limite_ms,
                                               std::string* motivo) {
    return transaccion(peticion_ipc(siguiente_id_++, nombre), limite_ms, motivo);
}

std::optional<RespuestaIpc> ConexionIpc::pedir(std::string_view nombre, bool data,
                                               int limite_ms, std::string* motivo) {
    return transaccion(peticion_ipc(siguiente_id_++, nombre, data), limite_ms, motivo);
}

std::optional<RespuestaIpc> ConexionIpc::transaccion(const std::string& peticion,
                                                     int limite_ms, std::string* motivo) {
    if (fd_ < 0) {
        if (motivo) *motivo = "la conexion no esta abierta";
        return std::nullopt;
    }

    std::size_t mandado = 0;
    while (mandado < peticion.size()) {
        const ssize_t n = ::write(fd_, peticion.data() + mandado, peticion.size() - mandado);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (motivo) *motivo = std::string("write: ") + std::strerror(errno);
            return std::nullopt;
        }
        mandado += static_cast<std::size_t>(n);
    }

    // Se acumula hasta el salto de linea, igual que hace el servidor con
    // nuestras peticiones (src/cli/ipc.c:526-539).
    std::string linea;
    char c = 0;
    for (;;) {
        pollfd pfd{fd_, POLLIN, 0};
        const int listo = ::poll(&pfd, 1, limite_ms < 0 ? -1 : limite_ms);
        if (listo < 0) {
            if (errno == EINTR) continue;
            if (motivo) *motivo = std::string("poll: ") + std::strerror(errno);
            return std::nullopt;
        }
        if (listo == 0) {
            if (motivo) {
                *motivo = "el grabador no respondio en " + std::to_string(limite_ms) + " ms";
            }
            return std::nullopt;
        }
        const ssize_t n = ::read(fd_, &c, 1);
        if (n == 0) {
            if (motivo) *motivo = "el grabador cerro la conexion sin responder";
            return std::nullopt;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            if (motivo) *motivo = std::string("read: ") + std::strerror(errno);
            return std::nullopt;
        }
        if (c == '\n') break;
        linea.push_back(c);
        if (linea.size() > 4096) {
            // La peticion maxima del protocolo es 4096 (include/cli/ipc.h:11);
            // una respuesta mas larga no es del protocolo.
            if (motivo) *motivo = "respuesta demasiado larga: no parece el protocolo de GSR";
            return std::nullopt;
        }
    }

    auto r = interpretar_respuesta_ipc(linea);
    if (!r && motivo) *motivo = "respuesta que no se entiende: " + linea;
    return r;
}

}  // namespace esr
