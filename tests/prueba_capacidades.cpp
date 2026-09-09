#include "capturia/capacidades.hpp"

#include <algorithm>
#include <string>

#include "capturia/entorno.hpp"
#include "comprobar.hpp"

using namespace capturia;

namespace {

std::string fixture(const std::string& nombre) {
    return prueba::leer(std::string(CAPTURIA_DIR_FIXTURES) + "/" + nombre);
}

bool algun_aviso_contiene(const Capacidades& c, std::string_view trozo) {
    return std::any_of(c.avisos.begin(), c.avisos.end(), [&](const std::string& a) {
        return a.find(trozo) != std::string::npos;
    });
}

// --- Volcado real de esta maquina -------------------------------------------
// Es el caso "no hay absolutamente nada": ningun binario instalado. Es el que
// mas facil se rompe si el parser da por hecho que GSR responde.
void volcado_real() {
    const std::string texto = prueba::leer(CAPTURIA_VOLCADO_REAL);
    COMPROBAR_NOTA(!texto.empty(), "docs/gsr-capabilities.txt no deberia estar vacio");

    const auto bloques = partir_volcado(texto);
    COMPROBAR_NOTA(bloques.size() == 6, "bloques=" + std::to_string(bloques.size()));

    // La cabecera de comentarios va antes del primer marcador y se ignora.
    if (!bloques.empty()) {
        COMPROBAR(bloques.front().comando == "gpu-screen-recorder --version");
        COMPROBAR(bloques.back().comando == "gsr-cli --help");
    }

    const auto c = interpretar_volcado(texto);
    COMPROBAR(!c.gsr_respondio);
    COMPROBAR(!c.gsr_cli_respondio);
    COMPROBAR(!c.version_gsr.has_value());
    COMPROBAR(c.fuentes_captura.empty());
    COMPROBAR(c.dispositivos_audio.empty());
    COMPROBAR(c.audio_por_aplicacion.empty());
    // Tres listas que no se pudieron leer, tres avisos. Silencio seria mentira.
    COMPROBAR_NOTA(c.avisos.size() == 3, "avisos=" + std::to_string(c.avisos.size()));

    const Entorno e = detectar_desde_volcado(texto);
    COMPROBAR(!e.graba_pantalla());
    COMPROBAR(!e.listo());
    COMPROBAR(!e.carencias.empty());
    COMPROBAR(std::any_of(e.carencias.begin(), e.carencias.end(),
                          [](const Carencia& x) { return x.bloqueante; }));
    // ffmpeg no sale en el volcado: "sin mirar" no es "ausente".
    COMPROBAR(!e.ffmpeg.evaluada);
}

// --- Casos sinteticos --------------------------------------------------------

void entrada_vacia() {
    const auto bloques = partir_volcado(fixture("vacio.txt"));
    COMPROBAR(bloques.empty());

    const auto c = interpretar_volcado(fixture("vacio.txt"));
    COMPROBAR(!c.gsr_respondio);
    COMPROBAR(c.avisos.size() == 1);
    COMPROBAR(algun_aviso_contiene(c, "no contiene ningun bloque"));
}

void formato_inesperado() {
    const auto texto = fixture("formato-inesperado.txt");
    COMPROBAR(!texto.empty());
    const auto bloques = partir_volcado(texto);
    COMPROBAR_NOTA(bloques.empty(), "texto sin marcadores no puede dar bloques");

    const auto c = interpretar_volcado(texto);
    COMPROBAR(algun_aviso_contiene(c, "no contiene ningun bloque"));
    COMPROBAR(!c.version_gsr.has_value());
}

void version_antigua() {
    const auto c = interpretar_volcado(fixture("version-antigua.txt"));
    COMPROBAR(c.gsr_respondio);
    COMPROBAR(c.version_gsr.has_value());
    COMPROBAR(c.version_gsr && *c.version_gsr == (Version{1, 2, 3}));

    // Comparacion contra una minima hipotetica. La real sigue sin decidirse
    // (version_minima_gsr() devuelve nullopt), asi que el umbral es de mentira
    // y solo prueba el orden.
    const Version hipotetica{5, 0, 0};
    COMPROBAR(c.version_gsr && *c.version_gsr < hipotetica);
}

void listas_con_separador() {
    const auto c = interpretar_volcado(fixture("listas-con-separador.txt"));
    COMPROBAR(c.gsr_respondio);
    COMPROBAR(c.gsr_cli_respondio);
    COMPROBAR(c.version_gsr && *c.version_gsr == (Version{5, 4, 1}));

    COMPROBAR_NOTA(c.fuentes_captura.size() == 4,
                   "fuentes=" + std::to_string(c.fuentes_captura.size()));
    if (c.fuentes_captura.size() == 4) {
        COMPROBAR(c.fuentes_captura[0].id == "DP-1");
        COMPROBAR(c.fuentes_captura[0].detalle == "2560x1440");
        // "portal|" trae separador pero nada detras: detalle vacio, no basura.
        COMPROBAR(c.fuentes_captura[2].id == "portal");
        COMPROBAR(c.fuentes_captura[2].detalle.empty());
    }
    COMPROBAR(c.dispositivos_audio.size() == 2);
    COMPROBAR(c.audio_por_aplicacion.size() == 2);
    // Las lineas que empiezan por # son comentarios del fixture, no entradas.
    COMPROBAR_NOTA(c.avisos.empty(), c.avisos.empty() ? "" : c.avisos.front());
}

void listas_sin_separador() {
    const auto c = interpretar_volcado(fixture("listas-sin-separador.txt"));
    COMPROBAR(c.dispositivos_audio.size() == 2);
    if (c.dispositivos_audio.size() == 2) {
        COMPROBAR(c.dispositivos_audio[0].id == "default_output");
        COMPROBAR(c.dispositivos_audio[0].detalle.empty());
    }
    // Lo importante: se queda con la linea entera y avisa de que no reconoce el
    // formato, en vez de partirla por donde le parezca.
    COMPROBAR(algun_aviso_contiene(c, "formato inesperado"));
}

void comando_fallido() {
    const auto c = interpretar_volcado(fixture("comando-fallido.txt"));
    COMPROBAR(c.gsr_respondio);
    COMPROBAR(c.version_gsr && *c.version_gsr == (Version{5, 4, 1}));
    // Un comando que revienta no son "cero dispositivos". La UI no debe poder
    // confundir las dos cosas.
    COMPROBAR(c.dispositivos_audio.empty());
    COMPROBAR(algun_aviso_contiene(c, "no termino bien"));
}

void volcado_truncado() {
    const auto bloques = partir_volcado(fixture("truncado.txt"));
    COMPROBAR_NOTA(bloques.size() == 2, "bloques=" + std::to_string(bloques.size()));
    if (bloques.size() == 2) {
        COMPROBAR(bloques[1].comando == "gpu-screen-recorder --list-capture-options");
        COMPROBAR(bloques[1].salida == "DP-1|2560x1440");
    }
    const auto c = interpretar_volcado(fixture("truncado.txt"));
    COMPROBAR(c.fuentes_captura.size() == 1);
}

void bloque_vacio_pero_correcto() {
    BloqueVolcado b;
    b.comando = "gpu-screen-recorder --list-audio-devices";
    b.codigo = 0;
    b.tiene_codigo = true;
    std::vector<std::string> avisos;
    const auto opciones = interpretar_lista(b, avisos);
    COMPROBAR(opciones.empty());
    COMPROBAR(avisos.size() == 1);
    COMPROBAR(avisos.size() == 1 && avisos[0].find("ninguna entrada") != std::string::npos);
}

void bloque_sin_codigo() {
    BloqueVolcado b;
    b.comando = "gpu-screen-recorder --list-audio-devices";
    b.salida = "default_output|Altavoces";
    b.tiene_codigo = false;  // volcado escrito a mano, o marcador ilegible
    std::vector<std::string> avisos;
    const auto opciones = interpretar_lista(b, avisos);
    COMPROBAR(opciones.empty());
    COMPROBAR(avisos.size() == 1 && avisos[0].find("desconocido") != std::string::npos);
}

}  // namespace

int main() {
    volcado_real();
    entrada_vacia();
    formato_inesperado();
    version_antigua();
    listas_con_separador();
    listas_sin_separador();
    comando_fallido();
    volcado_truncado();
    bloque_vacio_pero_correcto();
    bloque_sin_codigo();
    return prueba::resumen("prueba_capacidades");
}
