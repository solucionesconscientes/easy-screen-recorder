#include "capturia/capacidades.hpp"

#include <algorithm>

namespace capturia {
namespace {

std::string_view recortar(std::string_view s) {
    const auto blanco = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && blanco(s.front())) s.remove_prefix(1);
    while (!s.empty() && blanco(s.back())) s.remove_suffix(1);
    return s;
}

// Recorre el texto linea a linea sin copiarlo.
template <typename F>
void por_lineas(std::string_view texto, F&& f) {
    while (!texto.empty()) {
        const auto salto = texto.find('\n');
        f(texto.substr(0, salto));
        if (salto == std::string_view::npos) return;
        texto.remove_prefix(salto + 1);
    }
}

bool empieza_por(std::string_view s, std::string_view prefijo) {
    return s.size() >= prefijo.size() && s.compare(0, prefijo.size(), prefijo) == 0;
}

// El comando del bloque es "programa arg arg...". Se comprueba el programa por
// prefijo y la opcion por contenido, para no depender del orden.
bool comando_es(const BloqueVolcado& b, std::string_view programa, std::string_view opcion) {
    return empieza_por(b.comando, programa) && b.comando.find(opcion) != std::string::npos;
}

}  // namespace

std::vector<BloqueVolcado> partir_volcado(std::string_view volcado) {
    std::vector<BloqueVolcado> bloques;
    BloqueVolcado actual;
    bool dentro = false;

    const auto cerrar = [&] {
        if (!dentro) return;
        // El ultimo salto de linea es del marcador, no del contenido.
        while (!actual.salida.empty() && actual.salida.back() == '\n') actual.salida.pop_back();
        bloques.push_back(actual);
        actual = BloqueVolcado{};
        dentro = false;
    };

    por_lineas(volcado, [&](std::string_view linea) {
        const std::string_view limpia = recortar(linea);
        if (empieza_por(limpia, kMarcaComando)) {
            cerrar();  // un bloque sin "### fin" (volcado truncado) se cierra aqui
            dentro = true;
            actual.comando = std::string(recortar(limpia.substr(kMarcaComando.size())));
            return;
        }
        if (!dentro) return;  // texto suelto antes del primer marcador
        if (empieza_por(limpia, kMarcaCodigo)) {
            const std::string_view n = recortar(limpia.substr(kMarcaCodigo.size()));
            try {
                actual.codigo = std::stoi(std::string(n));
                actual.tiene_codigo = true;
            } catch (...) {
                actual.tiene_codigo = false;
            }
            return;
        }
        if (empieza_por(limpia, kMarcaFin)) {
            cerrar();
            return;
        }
        actual.salida.append(linea);
        actual.salida.push_back('\n');
    });

    cerrar();
    return bloques;
}

std::vector<Opcion> interpretar_lista(const BloqueVolcado& bloque,
                                      std::vector<std::string>& avisos) {
    if (!bloque.tiene_codigo || bloque.codigo != 0) {
        avisos.push_back("«" + bloque.comando + "» no termino bien (codigo " +
                         (bloque.tiene_codigo ? std::to_string(bloque.codigo) : std::string("desconocido")) +
                         "); su salida no se interpreta");
        return {};
    }

    std::vector<std::string_view> lineas;
    por_lineas(bloque.salida, [&](std::string_view linea) {
        const std::string_view limpia = recortar(linea);
        if (limpia.empty()) return;
        if (limpia.front() == '#') return;
        lineas.push_back(limpia);
    });

    if (lineas.empty()) {
        avisos.push_back("«" + bloque.comando + "» no devolvio ninguna entrada");
        return {};
    }

    const bool hay_separador =
        std::any_of(lineas.begin(), lineas.end(),
                    [](std::string_view l) { return l.find('|') != std::string_view::npos; });
    if (!hay_separador) {
        avisos.push_back("formato inesperado en «" + bloque.comando +
                         "»: ninguna linea trae separador «|»; se toma la linea entera como "
                         "identificador");
    }

    std::vector<Opcion> opciones;
    opciones.reserve(lineas.size());
    for (std::string_view l : lineas) {
        const auto corte = l.find('|');
        if (corte == std::string_view::npos) {
            opciones.push_back(Opcion{std::string(l), {}});
        } else {
            opciones.push_back(Opcion{std::string(recortar(l.substr(0, corte))),
                                      std::string(recortar(l.substr(corte + 1)))});
        }
    }
    return opciones;
}

Capacidades interpretar_volcado(std::string_view volcado) {
    Capacidades c;
    const auto bloques = partir_volcado(volcado);

    if (bloques.empty()) {
        c.avisos.push_back(
            "el volcado no contiene ningun bloque en el formato esperado («### comando: ...»)");
        return c;
    }

    for (const auto& b : bloques) {
        if (comando_es(b, "gpu-screen-recorder", "--version")) {
            c.gsr_respondio = b.tiene_codigo && b.codigo == 0;
            c.version_gsr = Version::desde_texto(b.salida);
            if (c.gsr_respondio && !c.version_gsr) {
                c.avisos.push_back(
                    "«" + b.comando + "» respondio pero no se reconoce un numero de version en su salida");
            }
        } else if (comando_es(b, "gpu-screen-recorder", "--list-capture-options")) {
            c.fuentes_captura = interpretar_lista(b, c.avisos);
        } else if (comando_es(b, "gpu-screen-recorder", "--list-audio-devices")) {
            c.dispositivos_audio = interpretar_lista(b, c.avisos);
        } else if (comando_es(b, "gpu-screen-recorder", "--list-application-audio")) {
            c.audio_por_aplicacion = interpretar_lista(b, c.avisos);
        } else if (empieza_por(b.comando, "gsr-cli")) {
            c.gsr_cli_respondio = b.tiene_codigo && b.codigo == 0;
        }
        // «--info» se vuelca pero no se interpreta: su formato no esta
        // verificado contra el codigo de GSR (ESTADO.md, bloqueo B1).
    }
    return c;
}

}  // namespace capturia
