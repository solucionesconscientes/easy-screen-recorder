#pragma once

#include <optional>
#include <string>
#include <vector>

namespace esr {

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

// Un proceso lanzado para quedarse: el grabador. ejecutar() no sirve para
// esto porque espera a que el hijo termine, y una grabacion termina cuando
// el usuario diga.
struct ProcesoLanzado {
    bool ejecutado = false;
    long pid = -1;
    std::string motivo;  // solo si !ejecutado
};

// Lanza el programa en su propia sesion (setsid), con stdout y stderr al
// fichero dado. No espera: el proceso sobrevive a quien lo lanzo, que es lo
// que necesita «easy-screen-recorder-cli grabar» para volver al instante. El fichero de log
// es la unica ventana a sus errores, por eso no es opcional.
ProcesoLanzado lanzar_desatendido(const std::string& programa,
                                  const std::vector<std::string>& args,
                                  const std::string& fichero_log);

// Si el proceso sigue vivo. kill(pid, 0) con permisos: dice si existe, no
// que sea el nuestro; por eso se combina siempre con el socket del IPC.
bool proceso_vivo(long pid);

}  // namespace esr
