// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/version.hpp"

namespace esr {
namespace {

bool es_digito(char c) { return c >= '0' && c <= '9'; }

}  // namespace

std::optional<Version> Version::desde_texto(std::string_view texto) {
    for (std::size_t i = 0; i < texto.size(); ++i) {
        if (!es_digito(texto[i])) continue;
        // No empezar a media de un numero ya recorrido.
        if (i > 0 && (es_digito(texto[i - 1]) || texto[i - 1] == '.')) continue;

        std::size_t j = i;
        int componentes[3] = {0, 0, 0};
        int leidos = 0;
        while (leidos < 3 && j < texto.size() && es_digito(texto[j])) {
            long valor = 0;
            while (j < texto.size() && es_digito(texto[j])) {
                if (valor < 1000000) valor = valor * 10 + (texto[j] - '0');
                ++j;
            }
            componentes[leidos++] = static_cast<int>(valor);
            const bool sigue_punto = j + 1 < texto.size() && texto[j] == '.' && es_digito(texto[j + 1]);
            if (leidos < 3 && sigue_punto) {
                ++j;
            } else {
                break;
            }
        }

        if (leidos >= 2) {
            return Version{componentes[0], componentes[1], componentes[2]};
        }
        i = j;  // seguir buscando despues del numero descartado
    }
    return std::nullopt;
}

std::string Version::texto() const {
    return std::to_string(mayor) + "." + std::to_string(menor) + "." + std::to_string(parche);
}

std::optional<Version> version_minima_gsr() {
    // 6.0.0. El criterio es "la mas antigua que ya traiga el IPC de gsr-cli
    // completo", y 6.0.0 es la mas antigua que se ha podido *comprobar* que lo
    // trae: es la del arbol de referencia de GSR (project.conf, version = "6.0.0"),
    // la que responde a --version en esta maquina, y sobre ella se ejecuto el
    // protocolo entero, incluida la respuesta diferida de «stop» que devuelve la
    // ruta del fichero guardado.
    //
    // La pagina de manual gsr-cli.1:1 lleva sellado "5.15.3", asi que gsr-cli ya
    // existia antes. No se baja el minimo por eso: que existiera el binario no
    // dice que el IPC estuviera completo, y sin ese arbol delante seria suponer.
    // Se baja el dia que se lea ese codigo, no antes.
    return Version{6, 0, 0};
}

}  // namespace esr
