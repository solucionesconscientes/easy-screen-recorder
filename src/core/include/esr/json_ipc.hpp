#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace esr {

// JSON justo para el IPC de GSR, y nada mas.
//
// No es un parser de JSON general a proposito. Las peticiones que mandamos son
// dos formas fijas y la respuesta trae tres campos ("id", "result", "data",
// src/cli/ipc.c:247-268 de GSR). Meter una dependencia para eso seria pagar
// miles de lineas por tres campos; escribir un parser general propio seria
// reinventarla. Esto cubre exactamente el protocolo documentado en
// docs/gsr-ipc.md y se niega ante todo lo demas.

// La respuesta de GSR a cualquier peticion: una linea JSON.
struct RespuestaIpc {
    long id = 0;
    bool ok = false;      // result == "ok"
    std::string data;     // ruta del fichero en los diferidos, motivo en los errores
    bool tiene_data = false;
};

// Peticion sin data: {"id":N,"name":"<nombre>"}
// El nombre viene de nuestro codigo, no del usuario, asi que no se escapa.
std::string peticion_ipc(long id, std::string_view nombre);

// Peticion con data booleano: {"id":N,"name":"<nombre>","data":true|false}
// Es la forma de set-paused (src/cli/ipc.c:462-468 de GSR).
std::string peticion_ipc(long id, std::string_view nombre, bool data);

// Interpreta una linea de respuesta. nullopt si no es una respuesta del
// protocolo; en ese caso el que llama decide, no se inventa un "ok".
std::optional<RespuestaIpc> interpretar_respuesta_ipc(std::string_view linea);

}  // namespace esr
