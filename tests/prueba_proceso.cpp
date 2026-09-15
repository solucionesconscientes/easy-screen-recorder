// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/proceso.hpp"

#include "comprobar.hpp"

using namespace esr;

int main() {
    // localizar
    COMPROBAR(localizar("sh").has_value());
    COMPROBAR(!localizar("binario-que-no-existe-en-ninguna-parte-9f2a").has_value());
    COMPROBAR(localizar("/bin/sh").has_value());
    COMPROBAR(!localizar("/bin/no-existe-9f2a").has_value());

    // ejecutar: caso feliz
    {
        const auto r = ejecutar("sh", {"-c", "echo hola"});
        COMPROBAR(r.ejecutado);
        COMPROBAR(!r.expirado);
        COMPROBAR(r.codigo == 0);
        COMPROBAR_NOTA(r.salida == "hola\n", r.salida);
    }

    // ejecutar: stderr tambien se recoge. Sin esto, un GSR que se queja por
    // stderr pareceria no decir nada.
    {
        const auto r = ejecutar("sh", {"-c", "echo malo >&2"});
        COMPROBAR(r.ejecutado);
        COMPROBAR_NOTA(r.salida == "malo\n", r.salida);
    }

    // ejecutar: codigo de salida distinto de cero no es "no ejecutado".
    {
        const auto r = ejecutar("sh", {"-c", "exit 3"});
        COMPROBAR(r.ejecutado);
        COMPROBAR(r.codigo == 3);
    }

    // ejecutar: binario ausente. Es el caso que distingue "no esta instalado"
    // de "esta y falla", y --check los cuenta distinto.
    {
        const auto r = ejecutar("binario-que-no-existe-en-ninguna-parte-9f2a", {});
        COMPROBAR(!r.ejecutado);
        COMPROBAR(!r.motivo.empty());
    }

    // ejecutar: el limite existe para que --check termine siempre, aunque una
    // sonda se quede esperando a un compositor que no contesta.
    {
        const auto r = ejecutar("sh", {"-c", "sleep 30"}, 300);
        COMPROBAR(r.ejecutado);
        COMPROBAR(r.expirado);
        COMPROBAR(!r.motivo.empty());
    }

    return prueba::resumen("prueba_proceso");
}
