// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "esr/json_ipc.hpp"

namespace esr {

// Conexion con un GSR lanzado con -ipc. Habla el protocolo de
// docs/gsr-ipc.md: una peticion JSON por linea y una respuesta por peticion.
//
// Una conexion por comando, como hace gsr-cli: encadenar varias por la misma
// conexion parece funcionar pero no esta comprobado (docs/gsr-ipc.md, "Lo que
// no queda claro"), y aqui no se apoya nada en lo no comprobado.
class ConexionIpc {
public:
    ConexionIpc() = default;
    ConexionIpc(const ConexionIpc&) = delete;
    ConexionIpc& operator=(const ConexionIpc&) = delete;
    ConexionIpc(ConexionIpc&& otra) noexcept;
    ConexionIpc& operator=(ConexionIpc&& otra) noexcept;
    ~ConexionIpc();

    // connect() al socket. Si entra, hay un grabador escuchando: es la
    // señal de "running" de gsr-cli (tools/gsr-cli/main.c:235-247), sin
    // mandar nada.
    static std::optional<ConexionIpc> conectar(const std::string& ruta_socket,
                                               std::string* motivo = nullptr);

    bool abierta() const { return fd_ >= 0; }
    void cerrar();

    // Manda la peticion y espera su respuesta. limite_ms < 0 espera sin
    // limite, que es OBLIGATORIO para stop y save-replay: la respuesta llega
    // cuando el fichero esta escrito y eso tarda lo que tarde
    // (docs/gsr-ipc.md, "Sin tiempo limite"). Cortar antes tira la peticion:
    // GSR descarta las pendientes del cliente que se desconecta.
    std::optional<RespuestaIpc> pedir(std::string_view nombre, int limite_ms,
                                      std::string* motivo = nullptr);
    // La forma con data booleano, para set-paused.
    std::optional<RespuestaIpc> pedir(std::string_view nombre, bool data, int limite_ms,
                                      std::string* motivo = nullptr);

private:
    std::optional<RespuestaIpc> transaccion(const std::string& peticion, int limite_ms,
                                            std::string* motivo);

    int fd_ = -1;
    long siguiente_id_ = 1;
};

}  // namespace esr
