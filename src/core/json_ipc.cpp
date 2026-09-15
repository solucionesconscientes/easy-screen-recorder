// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/json_ipc.hpp"

#include <cstdlib>

namespace esr {
namespace {

std::string_view recortar(std::string_view s) {
    const auto blanco = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && blanco(s.front())) s.remove_prefix(1);
    while (!s.empty() && blanco(s.back())) s.remove_suffix(1);
    return s;
}

// Avanza sobre los blancos JSON.
void saltar_blancos(std::string_view& s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' ||
                          s.front() == '\n')) {
        s.remove_prefix(1);
    }
}

// Lee una cadena JSON con sus escapes. GSR escapa lo que emite
// (src/cli/ipc.c, las respuestas van "ya escapadas"), asi que aqui se
// deshacen los escapes estandar. Un \uXXXX se acepta solo en ASCII: las rutas
// y los mensajes de GSR no traen mas, y ante otra cosa se prefiere fallar a
// inventar bytes.
bool leer_cadena(std::string_view& s, std::string& destino) {
    if (s.empty() || s.front() != '"') return false;
    s.remove_prefix(1);
    destino.clear();
    while (!s.empty()) {
        const char c = s.front();
        s.remove_prefix(1);
        if (c == '"') return true;
        if (c != '\\') {
            destino.push_back(c);
            continue;
        }
        if (s.empty()) return false;
        const char e = s.front();
        s.remove_prefix(1);
        switch (e) {
            case '"': destino.push_back('"'); break;
            case '\\': destino.push_back('\\'); break;
            case '/': destino.push_back('/'); break;
            case 'n': destino.push_back('\n'); break;
            case 't': destino.push_back('\t'); break;
            case 'r': destino.push_back('\r'); break;
            case 'b': destino.push_back('\b'); break;
            case 'f': destino.push_back('\f'); break;
            case 'u': {
                if (s.size() < 4) return false;
                unsigned valor = 0;
                for (int i = 0; i < 4; ++i) {
                    const char h = s.front();
                    s.remove_prefix(1);
                    valor <<= 4;
                    if (h >= '0' && h <= '9') valor |= static_cast<unsigned>(h - '0');
                    else if (h >= 'a' && h <= 'f') valor |= static_cast<unsigned>(h - 'a' + 10);
                    else if (h >= 'A' && h <= 'F') valor |= static_cast<unsigned>(h - 'A' + 10);
                    else return false;
                }
                if (valor > 0x7f) return false;
                destino.push_back(static_cast<char>(valor));
                break;
            }
            default: return false;
        }
    }
    return false;  // se acabo la linea sin cerrar la cadena
}

}  // namespace

std::string peticion_ipc(long id, std::string_view nombre) {
    return "{\"id\":" + std::to_string(id) + ",\"name\":\"" + std::string(nombre) + "\"}\n";
}

std::string peticion_ipc(long id, std::string_view nombre, bool data) {
    return "{\"id\":" + std::to_string(id) + ",\"name\":\"" + std::string(nombre) +
           "\",\"data\":" + (data ? "true" : "false") + "}\n";
}

std::optional<RespuestaIpc> interpretar_respuesta_ipc(std::string_view linea) {
    std::string_view s = recortar(linea);
    if (s.empty() || s.front() != '{' || s.back() != '}') return std::nullopt;
    s.remove_prefix(1);
    s.remove_suffix(1);

    RespuestaIpc r;
    bool tiene_id = false;
    bool tiene_result = false;

    while (true) {
        saltar_blancos(s);
        if (s.empty()) break;

        std::string clave;
        if (!leer_cadena(s, clave)) return std::nullopt;
        saltar_blancos(s);
        if (s.empty() || s.front() != ':') return std::nullopt;
        s.remove_prefix(1);
        saltar_blancos(s);

        if (clave == "id") {
            char* fin = nullptr;
            const std::string resto(s);
            r.id = std::strtol(resto.c_str(), &fin, 10);
            if (fin == resto.c_str()) return std::nullopt;
            s.remove_prefix(static_cast<std::size_t>(fin - resto.c_str()));
            tiene_id = true;
        } else if (clave == "result") {
            std::string valor;
            if (!leer_cadena(s, valor)) return std::nullopt;
            r.ok = (valor == "ok");
            tiene_result = true;
        } else if (clave == "data") {
            if (!leer_cadena(s, r.data)) return std::nullopt;
            r.tiene_data = true;
        } else {
            // Un campo que no conocemos: la respuesta entera se rechaza. Es
            // preferible a saltarselo, porque saltarse un valor JSON
            // arbitrario ya exige el parser general que no queremos tener.
            return std::nullopt;
        }

        saltar_blancos(s);
        if (s.empty()) break;
        if (s.front() != ',') return std::nullopt;
        s.remove_prefix(1);
    }

    if (!tiene_id || !tiene_result) return std::nullopt;
    return r;
}

}  // namespace esr
