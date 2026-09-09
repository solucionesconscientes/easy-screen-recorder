#pragma once

#include <compare>
#include <optional>
#include <string>
#include <string_view>

namespace capturia {

// Version del propio Capturia. Se sincroniza con project() en CMakeLists.txt.
inline constexpr std::string_view kVersionCapturia = "0.1.0";

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

// Version minima de GSR soportada. SIN DECIDIR todavia: fijarla exige leer
// project.conf del arbol de GSR, que no esta disponible (ver ESTADO.md, bloqueo
// B1). Devuelve nullopt mientras siga sin decidirse, y --check informa de la
// version detectada sin dar por fallado nada.
std::optional<Version> version_minima_gsr();

}  // namespace capturia
