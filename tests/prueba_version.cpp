#include "esr/version.hpp"

#include "comprobar.hpp"

using esr::Version;

int main() {
    // Casos normales.
    {
        const auto v = Version::desde_texto("5.4.1");
        COMPROBAR(v.has_value());
        COMPROBAR(v && *v == (Version{5, 4, 1}));
    }
    {
        const auto v = Version::desde_texto("GPU Screen Recorder 5.4.1\n");
        COMPROBAR(v && *v == (Version{5, 4, 1}));
    }
    {
        // Dos componentes bastan; el tercero queda a cero.
        const auto v = Version::desde_texto("version 5.4");
        COMPROBAR(v && *v == (Version{5, 4, 0}));
    }
    {
        const auto v = Version::desde_texto("v1.2.3-rc1");
        COMPROBAR(v && *v == (Version{1, 2, 3}));
    }
    {
        // Un cuarto componente se ignora: nuestro modelo tiene tres.
        const auto v = Version::desde_texto("1.2.3.4");
        COMPROBAR(v && *v == (Version{1, 2, 3}));
    }

    // Casos en los que NO debe reconocerse nada.
    COMPROBAR(!Version::desde_texto("").has_value());
    COMPROBAR(!Version::desde_texto("sin numeros por ninguna parte").has_value());
    // Un numero suelto no es una version: por eso se exigen dos componentes.
    COMPROBAR(!Version::desde_texto("compilado en 2026").has_value());
    COMPROBAR(!Version::desde_texto("command not found").has_value());
    // Un numero suelto seguido de otro tampoco, si no van unidos por un punto.
    COMPROBAR(!Version::desde_texto("line 45: error 127").has_value());

    // Un numero suelto delante no debe tapar la version que viene detras.
    {
        const auto v = Version::desde_texto("build 7 de gsr 5.4.1");
        COMPROBAR(v && *v == (Version{5, 4, 1}));
    }

    // Orden. Es lo que usara --check cuando se fije la version minima.
    COMPROBAR((Version{1, 2, 3}) < (Version{5, 0, 0}));
    COMPROBAR((Version{5, 4, 1}) > (Version{5, 4, 0}));
    COMPROBAR((Version{5, 4, 1}) == (Version{5, 4, 1}));
    COMPROBAR(!((Version{5, 10, 0}) < (Version{5, 9, 0})));  // 10 va despues de 9

    COMPROBAR((Version{5, 4, 1}).texto() == "5.4.1");

    // La version de Easy Screen Recorder vive duplicada: kVersionEsr y el VERSION de
    // project() en CMakeLists.txt. Aqui se exige que coincidan; sin esto se
    // separan en el primer olvido y --version miente.
    COMPROBAR_NOTA(esr::kVersionEsr == std::string_view(ESR_VERSION_CMAKE),
                   std::string("version.hpp dice ") + std::string(esr::kVersionEsr) +
                       " y CMakeLists.txt dice " ESR_VERSION_CMAKE);

    // Ya esta decidida: 6.0.0, la version cuyo IPC se ha podido comprobar
    // entero en la maquina de desarrollo (ver CLAUDE.md y docs/gsr-ipc.md).
    const auto minima = esr::version_minima_gsr();
    COMPROBAR(minima.has_value());
    COMPROBAR(minima && *minima == (Version{6, 0, 0}));
    // Y ordena como debe: 5.15.3 se queda corta, 6.0.1 pasa.
    COMPROBAR(minima && (Version{5, 15, 3}) < *minima);
    COMPROBAR(minima && !((Version{6, 0, 1}) < *minima));

    return prueba::resumen("prueba_version");
}
