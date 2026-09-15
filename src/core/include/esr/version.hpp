// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <compare>
#include <optional>
#include <string>
#include <string_view>

namespace esr {

// Version del propio Easy Screen Recorder. Se sincroniza con project() en CMakeLists.txt.
inline constexpr std::string_view kVersionEsr = "0.1.0";

// Numero de version de tres componentes. Es dato de maquina: para enseñar algo
// al usuario se usa texto(), nunca los enteros sueltos.
struct Version {
    int mayor = 0;
    int menor = 0;
    int parche = 0;

    // Extrae el primer "X.Y" o "X.Y.Z" que aparezca en el texto. Exige dos
    // componentes a proposito: asi un año suelto ("2026") o un numero de build
    // no se cuelan como version.
    static std::optional<Version> desde_texto(std::string_view texto);

    std::string texto() const;

    friend auto operator<=>(const Version&, const Version&) = default;
    friend bool operator==(const Version&, const Version&) = default;
};

// Version minima de GSR soportada: 6.0.0. El razonamiento y las fuentes estan
// en CLAUDE.md y en docs/gsr-ipc.md. Ya no devuelve nullopt.
std::optional<Version> version_minima_gsr();

}  // namespace esr
