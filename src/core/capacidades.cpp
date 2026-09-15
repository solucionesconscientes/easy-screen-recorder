// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/capacidades.hpp"

#include <algorithm>

namespace esr {
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

// Interpreta el bloque de --info. Las secciones que no se conocen se saltan
// con un aviso: un GSR mas nuevo puede añadir una y eso no es un fallo, pero
// tampoco se interpreta a ciegas.
InfoGsr interpretar_info(const BloqueVolcado& bloque, std::vector<std::string>& avisos,
                         std::vector<Opcion>& fuentes) {
    InfoGsr info;
    if (!bloque.tiene_codigo || bloque.codigo != 0) {
        avisos.push_back("«" + bloque.comando + "» no termino bien (codigo " +
                         (bloque.tiene_codigo ? std::to_string(bloque.codigo)
                                              : std::string("desconocido")) +
                         "); su salida no se interpreta");
        return info;
    }

    std::string seccion;
    bool seccion_conocida = false;
    por_lineas(bloque.salida, [&](std::string_view linea) {
        const std::string_view limpia = recortar(linea);
        if (limpia.empty() || limpia.front() == '#') return;
        if (empieza_por(limpia, "gsr error:") || empieza_por(limpia, "gsr warning:")) return;

        if (empieza_por(limpia, "section=")) {
            seccion = std::string(limpia.substr(std::string_view("section=").size()));
            seccion_conocida = seccion == "system_info" || seccion == "gpu_info" ||
                               seccion == "video_codecs" || seccion == "image_formats" ||
                               seccion == "capture_options";
            if (!seccion_conocida) {
                avisos.push_back("seccion desconocida «" + seccion +
                                 "» en «--info»; se salta sin interpretar");
            }
            info.presente = true;
            return;
        }
        if (!seccion_conocida) return;

        if (seccion == "video_codecs") {
            info.codecs_video.emplace_back(limpia);
            return;
        }
        if (seccion == "image_formats") {
            info.formatos_imagen.emplace_back(limpia);
            return;
        }
        if (seccion == "capture_options") {
            // Mismo formato que --list-capture-options, que ya no se sondea
            // aparte: --info trae la seccion identica y sondear dos veces
            // costaba cientos de milisegundos de arranque.
            Opcion o;
            std::string_view resto = limpia;
            const auto corte = resto.find('|');
            o.id = std::string(recortar(resto.substr(0, corte)));
            if (corte != std::string_view::npos) {
                resto.remove_prefix(corte + 1);
                for (;;) {
                    const auto c2 = resto.find('|');
                    o.campos.push_back(std::string(recortar(resto.substr(0, c2))));
                    if (c2 == std::string_view::npos) break;
                    resto.remove_prefix(c2 + 1);
                }
                if (o.campos.size() == 1 && o.campos.front().empty()) o.campos.clear();
            }
            fuentes.push_back(std::move(o));
            return;
        }

        // system_info y gpu_info: clave|valor.
        const auto corte = limpia.find('|');
        const std::string_view clave = recortar(limpia.substr(0, corte));
        const std::string_view valor =
            corte == std::string_view::npos ? std::string_view{} : recortar(limpia.substr(corte + 1));
        if (clave == "display_server") info.servidor_grafico = std::string(valor);
        else if (clave == "supports_app_audio") info.audio_por_aplicacion = (valor == "yes");
        else if (clave == "vendor") info.vendedor_gpu = std::string(valor);
        // El resto de claves (gsr_version, card_path, is_steam_deck...) no se
        // necesitan hoy; se leen del volcado el dia que hagan falta.
    });

    if (!info.presente) {
        avisos.push_back("«" + bloque.comando + "» respondio pero sin ninguna seccion; no se interpreta");
    }
    return info;
}

}  // namespace

bool codec_es_hardware(std::string_view nombre) {
    // El unico sufijo que marca CPU en GSR 6.0.0 es «_software». Si el dia de
    // mañana aparece otro, este predicado es el unico sitio que tocar.
    constexpr std::string_view sufijo = "_software";
    return !(nombre.size() >= sufijo.size() &&
             nombre.substr(nombre.size() - sufijo.size()) == sufijo);
}

std::optional<std::string> mejor_codec_hardware(const InfoGsr& info) {
    const auto tiene = [&](std::string_view nombre) {
        return std::find(info.codecs_video.begin(), info.codecs_video.end(), nombre) !=
               info.codecs_video.end();
    };
    for (std::string_view candidato : {"h264", "hevc", "av1"}) {
        if (tiene(candidato)) return std::string(candidato);
    }
    return std::nullopt;
}

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
        } else if (comando_es(b, "gpu-screen-recorder", "--info")) {
            // De aqui salen los codecs y, desde la Tanda 4, tambien las
            // fuentes de captura: su seccion capture_options es identica a la
            // salida de --list-capture-options y sondear dos veces costaba
            // arranque.
            std::vector<Opcion> fuentes;
            c.info = interpretar_info(b, c.avisos, fuentes);
            if (c.fuentes_captura.empty()) c.fuentes_captura = std::move(fuentes);
        } else if (comando_es(b, "gpu-screen-recorder", "--list-capture-options")) {
            // Ya no se sondea, pero los volcados viejos y los fixtures lo
            // traen y se sigue entendiendo. Pisa a lo leido de --info: es la
            // salida especifica.
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
    }
    return c;
}

}  // namespace esr
