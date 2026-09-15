#include "esr/json_ipc.hpp"

#include "comprobar.hpp"

using namespace esr;

int main() {
    // Peticiones: las formas exactas que espera el servidor de GSR
    // (src/cli/ipc.c:271-329). Con el \n final, porque procesa por linea.
    COMPROBAR(peticion_ipc(7, "stop") == "{\"id\":7,\"name\":\"stop\"}\n");
    COMPROBAR(peticion_ipc(8, "set-paused", true) ==
              "{\"id\":8,\"name\":\"set-paused\",\"data\":true}\n");
    COMPROBAR(peticion_ipc(8, "set-paused", false) ==
              "{\"id\":8,\"name\":\"set-paused\",\"data\":false}\n");

    // Respuestas literales de la transcripcion real de docs/gsr-ipc.md.
    {
        const auto r = interpretar_respuesta_ipc("{\"id\":7,\"result\":\"ok\"}");
        COMPROBAR(r && r->id == 7 && r->ok && !r->tiene_data);
    }
    {
        const auto r = interpretar_respuesta_ipc(
            "{\"id\":11,\"result\":\"ok\",\"data\":\"/home/pc/.cache/easy-screen-recorder-prueba/prueba.mkv\"}");
        COMPROBAR(r && r->id == 11 && r->ok && r->tiene_data);
        COMPROBAR(r && r->data == "/home/pc/.cache/easy-screen-recorder-prueba/prueba.mkv");
    }
    {
        const auto r = interpretar_respuesta_ipc(
            "{\"id\":9,\"result\":\"error\",\"data\":\"unknown request name 'no-existe'\"}");
        COMPROBAR(r && r->id == 9 && !r->ok);
        COMPROBAR(r && r->data == "unknown request name 'no-existe'");
    }
    {
        // Peticion sin id: GSR contesta con id 0 (src/cli/ipc.c:272 y 505).
        const auto r = interpretar_respuesta_ipc(
            "{\"id\":0,\"result\":\"error\",\"data\":\"the request is missing the 'id' field\"}");
        COMPROBAR(r && r->id == 0 && !r->ok);
    }

    // Escapes: una ruta con comillas o una barra invertida tiene que volver
    // intacta, porque esa ruta se le enseña al usuario y se abre.
    {
        const auto r = interpretar_respuesta_ipc(
            "{\"id\":1,\"result\":\"ok\",\"data\":\"/v\\u00eddeos/a \\\"b\\\"\\nc\\\\d\"}");
        COMPROBAR(!r.has_value());  // \u00ed no es ASCII: se rechaza, no se inventa
    }
    {
        const auto r = interpretar_respuesta_ipc(
            "{\"id\":1,\"result\":\"ok\",\"data\":\"a \\\"b\\\" y \\\\ y \\n y \\u0041\"}");
        COMPROBAR(r && r->data == "a \"b\" y \\ y \n y A");
    }

    // Con blancos alrededor y entre campos tambien vale.
    {
        const auto r = interpretar_respuesta_ipc("  { \"id\" : 3 , \"result\" : \"ok\" }\n");
        COMPROBAR(r && r->id == 3 && r->ok);
    }

    // Lo que NO es una respuesta del protocolo se rechaza, no se adivina.
    COMPROBAR(!interpretar_respuesta_ipc("").has_value());
    COMPROBAR(!interpretar_respuesta_ipc("no es json").has_value());
    COMPROBAR(!interpretar_respuesta_ipc("{\"id\":1}").has_value());        // sin result
    COMPROBAR(!interpretar_respuesta_ipc("{\"result\":\"ok\"}").has_value());  // sin id
    COMPROBAR(!interpretar_respuesta_ipc("{\"id\":x,\"result\":\"ok\"}").has_value());
    COMPROBAR(!interpretar_respuesta_ipc("{\"id\":1,\"result\":\"ok\"").has_value());  // sin cerrar
    COMPROBAR(!interpretar_respuesta_ipc("{\"id\":1,\"result\":\"ok\",\"data\":\"sin cierre}").has_value());
    // Un campo desconocido rechaza la respuesta entera: saltarse un valor
    // arbitrario ya seria un parser general.
    COMPROBAR(!interpretar_respuesta_ipc("{\"id\":1,\"result\":\"ok\",\"extra\":[1,2]}").has_value());

    return prueba::resumen("prueba_json_ipc");
}
