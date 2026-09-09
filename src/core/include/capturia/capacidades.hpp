#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "capturia/version.hpp"

namespace capturia {

// Una entrada de una lista que GSR enumera: un monitor, un dispositivo de
// audio, una aplicacion. `detalle` queda vacio si la linea no traia mas.
struct Opcion {
    std::string id;
    std::string detalle;
};

// Un bloque del volcado: un comando, su codigo de salida y su salida completa.
struct BloqueVolcado {
    std::string comando;
    std::string salida;
    int codigo = -1;
    bool tiene_codigo = false;
};

struct Capacidades {
    bool gsr_respondio = false;
    bool gsr_cli_respondio = false;
    std::optional<Version> version_gsr;
    std::vector<Opcion> fuentes_captura;
    std::vector<Opcion> dispositivos_audio;
    std::vector<Opcion> audio_por_aplicacion;
    // Que no se pudo interpretar y por que. --check lo imprime tal cual: el
    // parser nunca inventa una capacidad, prefiere avisar de que no entendio.
    std::vector<std::string> avisos;
};

// Marcadores del formato de volcado. Son NUESTROS, no de GSR: el envoltorio lo
// generamos nosotros (scripts/volcar-capacidades.sh y detectar()), asi que este
// nivel del parser es verificable. Lo que va dentro de cada bloque lo escribe
// GSR y ahi el parser es deliberadamente conservador.
inline constexpr std::string_view kMarcaComando = "### comando: ";
inline constexpr std::string_view kMarcaCodigo = "### codigo: ";
inline constexpr std::string_view kMarcaFin = "### fin";

std::vector<BloqueVolcado> partir_volcado(std::string_view volcado);

// Convierte la salida de un bloque en una lista de opciones.
//
// AVISO: el formato real de las listas de GSR NO esta verificado (ver
// docs/gsr-ipc.md, bloqueo B1). La heuristica es "id|detalle" por linea y, si
// ninguna linea trae separador, se toma la linea entera como id y se deja un
// aviso. Cuando haya un volcado real hay que revisar esta funcion.
std::vector<Opcion> interpretar_lista(const BloqueVolcado& bloque,
                                      std::vector<std::string>& avisos);

Capacidades interpretar_volcado(std::string_view volcado);

}  // namespace capturia
