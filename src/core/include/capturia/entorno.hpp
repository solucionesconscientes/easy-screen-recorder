#pragma once

#include <optional>
#include <string>
#include <vector>

#include "capturia/capacidades.hpp"
#include "capturia/version.hpp"

namespace capturia {

struct Herramienta {
    std::string nombre;
    // `evaluada` en false significa "no se ha llegado a mirar", que no es lo
    // mismo que ausente. detectar_desde_volcado() no ejecuta nada y por eso no
    // puede opinar sobre ffmpeg.
    bool evaluada = false;
    bool presente = false;
    std::string ruta;
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

}  // namespace capturia
