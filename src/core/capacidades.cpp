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

// El comando del bloque ya no empieza siempre por el nombre del programa:
// cuando GSR viene en flatpak la linea es «flatpak run --command=X <app-id> ...».
// Por eso se busca el nombre en cualquier posicion. No hay ambiguedad porque el
// id de la aplicacion lleva guiones bajos («com.dec05eba.gpu_screen_recorder»)
// y el binario guiones («gpu-screen-recorder»), asi que no se confunden.
bool comando_es(const BloqueVolcado& b, std::string_view programa, std::string_view opcion) {
    return b.comando.find(programa) != std::string::npos &&
           b.comando.find(opcion) != std::string::npos;
}

}  // namespace

std::string Opcion::detalle() const {
    std::string texto;
    for (const auto& c : campos) {
        if (!texto.empty()) texto.push_back(' ');
        texto += c;
    }
    return texto;
}

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
                                      std::vector<std::string>& avisos,
                                      FormatoLista formato) {
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
        // GSR escribe sus errores por stderr y el volcado los mezcla con la
        // salida. Una linea de error no es una opcion de captura.
        if (empieza_por(limpia, "gsr error:") || empieza_por(limpia, "gsr warning:")) return;
        lineas.push_back(limpia);
    });

    if (lineas.empty()) {
        if (!formato.vacio_normal) {
            avisos.push_back("«" + bloque.comando + "» no devolvio ninguna entrada");
        }
        return {};
    }

    if (formato.exige_separador) {
        const auto sin = std::find_if(lineas.begin(), lineas.end(), [](std::string_view l) {
            return l.find('|') == std::string_view::npos;
        });
        if (sin != lineas.end()) {
            avisos.push_back("formato inesperado en «" + bloque.comando +
                             "»: se esperaba «id|detalle» en toda linea y «" + std::string(*sin) +
                             "» no trae separador; se toma la linea entera como identificador");
        }
    }

    std::vector<Opcion> opciones;
    opciones.reserve(lineas.size());
    for (std::string_view l : lineas) {
        Opcion o;
        std::string_view resto = l;
        const auto primer_corte = resto.find('|');
        o.id = std::string(recortar(resto.substr(0, primer_corte)));
        if (primer_corte == std::string_view::npos) {
            opciones.push_back(std::move(o));
            continue;
        }
        resto.remove_prefix(primer_corte + 1);
        // Se parte por todos los «|», no solo por el primero: una camara imprime
        // «/dev/video0|640x480@30hz|mjpeg» y la resolucion y el formato son dos
        // datos distintos, no uno con una barra dentro.
        for (;;) {
            const auto corte = resto.find('|');
            o.campos.push_back(std::string(recortar(resto.substr(0, corte))));
            if (corte == std::string_view::npos) break;
            resto.remove_prefix(corte + 1);
        }
        // «portal|» trae separador pero nada detras: eso no es un campo vacio,
        // es una linea sin detalle.
        if (o.campos.size() == 1 && o.campos.front().empty()) o.campos.clear();
        opciones.push_back(std::move(o));
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
            // Mezcla lineas de uno, dos y tres campos (commands.c:198-229), asi
            // que no se le exige separador. Vacia si que seria raro: significa
            // que la maquina no ofrece ni monitor ni portal ni camara.
            c.fuentes_captura = interpretar_lista(b, c.avisos, FormatoLista{});
        } else if (comando_es(b, "gpu-screen-recorder", "--list-audio-devices")) {
            c.dispositivos_audio = interpretar_lista(b, c.avisos, FormatoLista{true, false});
        } else if (comando_es(b, "gpu-screen-recorder", "--list-application-audio")) {
            // Imprime el nombre pelado de cada aplicacion que suena, sin
            // separador, y nada cuando no suena ninguna (commands.c:296-316).
            c.audio_por_aplicacion = interpretar_lista(b, c.avisos, FormatoLista{false, true});
        } else if (b.comando.find("gsr-cli") != std::string::npos) {
            c.gsr_cli_respondio = b.tiene_codigo && b.codigo == 0;
        }
        // «--info» se vuelca pero no se interpreta. Su formato ya se conoce
        // (secciones «section=nombre» y lineas «clave|valor», commands.c:238-274)
        // pero de ahi salen los codecs, y eso es trabajo de la Fase 1.
    }
    return c;
}

}  // namespace capturia
