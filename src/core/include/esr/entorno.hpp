#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "esr/capacidades.hpp"
#include "esr/version.hpp"

namespace esr {

// Identificador de la aplicacion flatpak de GSR. Es un nombre propio, no una
// lista de capacidades: no se detecta, se sabe. Lo que si se detecta es si esta
// instalada.
inline constexpr std::string_view kAppFlatpakGsr = "com.dec05eba.gpu_screen_recorder";

// Como se invoca de verdad un binario de GSR en esta maquina.
//
// Existe porque GSR se distribuye tambien como flatpak y en KDE Plasma esa es
// la via habitual. Buscarlo solo en PATH daba «ausente» en maquinas donde GSR
// funciona perfectamente, que es justo el error que --check no puede cometer.
struct Invocacion {
    std::string programa;              // el binario que se ejecuta de verdad
    std::vector<std::string> prefijo;  // lo que va antes de los argumentos de la sonda
    std::string origen;                // «PATH» o «flatpak»: --check lo enseña
    std::string ruta;                  // ruta del binario, o el id de la aplicacion

    // La linea de comando completa, para el envoltorio del volcado.
    std::string linea(const std::vector<std::string>& args) const;
};

// Busca un binario de GSR primero en PATH y luego en el flatpak. Devuelve
// nullopt si no esta por ninguna de las dos vias.
std::optional<Invocacion> localizar_gsr(const std::string& binario);

struct Herramienta {
    std::string nombre;
    // `evaluada` en false significa "no se ha llegado a mirar", que no es lo
    // mismo que ausente. detectar_desde_volcado() no ejecuta nada y por eso no
    // puede opinar sobre ffmpeg.
    bool evaluada = false;
    bool presente = false;
    std::string ruta;
    std::string origen;  // «PATH», «flatpak» o vacio si no se sabe
    std::optional<Version> version;
    std::string diagnostico;  // por que falta, o que fallo al sondearla
};

// Un requisito que no se cumple, ya redactado para enseñarselo al usuario.
struct Carencia {
    std::string que;         // que falta
    std::string consecuencia;  // que deja de funcionar por eso
    bool bloqueante = false;   // si impide grabar algo
};

struct Entorno {
    Herramienta gsr;
    Herramienta gsr_cli;
    Herramienta ffmpeg;
    Herramienta ffprobe;
    Capacidades capacidades;
    std::vector<Carencia> carencias;

    bool graba_pantalla() const;
    bool graba_audio_solo() const;
    bool listo() const;  // todo lo bloqueante cumplido
};

// Comandos con los que se sondea el entorno. Se exponen para que el script de
// volcado y el binario usen exactamente la misma lista.
struct Sonda {
    std::string programa;
    std::vector<std::string> args;
};
std::vector<Sonda> sondas();

// Ejecuta las sondas, arma un volcado en el formato de capacidades.hpp y lo
// interpreta. Un solo parser para produccion y para tests.
std::string volcar();
Entorno detectar();

// Misma deteccion partiendo de un volcado ya escrito. Es la puerta de entrada
// de los tests y de un futuro --check sobre fichero.
Entorno detectar_desde_volcado(std::string_view volcado);

}  // namespace esr
