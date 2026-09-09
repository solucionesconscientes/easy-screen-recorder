#pragma once

#include <optional>
#include <string>
#include <vector>

namespace capturia {

// Resultado de lanzar un programa externo.
//
// `ejecutado` separa "el binario no esta" de "el binario esta y devolvio
// error". Para --check son dos diagnosticos distintos y el usuario necesita
// saber cual de los dos le ha tocado.
struct ResultadoProceso {
    bool ejecutado = false;
    bool expirado = false;
    int codigo = -1;
    std::string salida;  // stdout y stderr mezclados, en orden de llegada
    std::string motivo;  // solo si !ejecutado o expirado
};

// Busca el programa en PATH y devuelve su ruta absoluta.
std::optional<std::string> localizar(const std::string& programa);

// Lanza el programa sin pasar por el shell. El limite existe porque una sonda
// de GSR puede quedarse esperando a un compositor que no responde, y --check
// tiene que terminar siempre.
ResultadoProceso ejecutar(const std::string& programa,
                          const std::vector<std::string>& args,
                          int limite_ms = 5000);

}  // namespace capturia
