// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/capacidades.hpp"

#include <algorithm>
#include <fstream>

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

namespace {

bool es_camara(std::string_view id) { return empieza_por(id, "/dev/"); }

FamiliaMonitor familia_de(std::string_view id) {
    // En mayusculas para no depender de como lo escriba cada driver, y eDP
    // antes que DP: «eDP-1» tambien acaba en DP y no es un cable.
    std::string may(id);
    for (char& c : may) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    if (empieza_por(may, "EDP") || empieza_por(may, "LVDS") || empieza_por(may, "DSI")) {
        return FamiliaMonitor::Interna;
    }
    if (empieza_por(may, "HDMI")) return FamiliaMonitor::Hdmi;
    if (empieza_por(may, "DP") || empieza_por(may, "DISPLAYPORT")) {
        return FamiliaMonitor::DisplayPort;
    }
    if (empieza_por(may, "VGA")) return FamiliaMonitor::Vga;
    if (empieza_por(may, "DVI")) return FamiliaMonitor::Dvi;
    // Lo que no se reconoce es un monitor sin mas. Generico es honesto;
    // adivinar no lo seria.
    return FamiliaMonitor::Otra;
}

// «1366x768» si el campo tiene esa forma. Vacio si no: el parser no adorna lo
// que no ha entendido, que es la misma regla que en el resto del fichero.
std::string resolucion_de(std::string_view campo) {
    if (campo.empty()) return {};
    const auto equis = campo.find('x');
    if (equis == std::string_view::npos || equis == 0 || equis + 1 == campo.size()) return {};
    const auto solo_digitos = [](std::string_view s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) {
            return c >= '0' && c <= '9';
        });
    };
    if (!solo_digitos(campo.substr(0, equis)) || !solo_digitos(campo.substr(equis + 1))) {
        return {};
    }
    return std::string(campo);
}

}  // namespace

std::vector<FuenteAmable> fuentes_amables(const std::vector<Opcion>& fuentes_captura) {
    std::vector<FuenteAmable> monitores;
    std::vector<FuenteAmable> especiales;
    std::vector<FuenteAmable> camaras;

    for (const auto& o : fuentes_captura) {
        // Una camara con N modos es UNA fuente para el usuario; el modo lo elige
        // GSR. Sin esto la lista enseñaba /dev/video0 ocho veces.
        const auto ya_esta = [&o](const std::vector<FuenteAmable>& v) {
            return std::any_of(v.begin(), v.end(),
                               [&o](const FuenteAmable& f) { return f.id == o.id; });
        };
        if (ya_esta(monitores) || ya_esta(especiales) || ya_esta(camaras)) continue;

        FuenteAmable f;
        f.id = o.id;
        if (o.id == "region") {
            f.tipo = TipoFuente::Region;
            especiales.push_back(f);
        } else if (o.id == "portal") {
            f.tipo = TipoFuente::Portal;
            especiales.push_back(f);
        } else if (o.id == "focused") {
            f.tipo = TipoFuente::VentanaActiva;
            especiales.push_back(f);
        } else if (es_camara(o.id)) {
            f.tipo = TipoFuente::Camara;
            f.nombre = nombre_camara(o.id);
            camaras.push_back(f);
        } else {
            f.tipo = TipoFuente::Monitor;
            f.familia = familia_de(o.id);
            f.resolucion = o.campos.empty() ? std::string() : resolucion_de(o.campos.front());
            monitores.push_back(f);
        }
    }

    // El desempate se decide con la lista entera delante: para saber si hay que
    // enseñar «(DP-2)» hay que haber contado cuantas DisplayPort hay, y eso no
    // se sabe mientras se recorre.
    for (auto& f : monitores) {
        f.desempatar = std::count_if(monitores.begin(), monitores.end(),
                                     [&f](const FuenteAmable& otra) {
                                         return otra.familia == f.familia;
                                     }) > 1;
    }
    for (auto& f : camaras) {
        f.desempatar = std::count_if(camaras.begin(), camaras.end(),
                                     [&f](const FuenteAmable& otra) {
                                         return otra.nombre == f.nombre;
                                     }) > 1;
    }

    // Los monitores primero: grabar la pantalla es el caso de siempre y el que
    // el atajo global elige solo. Luego lo que exige decidir al empezar, y al
    // final las camaras, que no son «grabar la pantalla».
    std::vector<FuenteAmable> todas;
    todas.reserve(monitores.size() + especiales.size() + camaras.size());
    todas.insert(todas.end(), monitores.begin(), monitores.end());
    todas.insert(todas.end(), especiales.begin(), especiales.end());
    todas.insert(todas.end(), camaras.begin(), camaras.end());
    return todas;
}

std::string limpiar_nombre_camara(std::string_view crudo) {
    // Los dos puntos separan el nombre de lo que el driver pega detras, que en
    // esta maquina es el mismo nombre otra vez y cortado. Lo de delante basta.
    const auto dos_puntos = crudo.find(':');
    std::string nombre(recortar(dos_puntos == std::string_view::npos ? crudo
                                                                    : crudo.substr(0, dos_puntos)));
    for (char& c : nombre) {
        if (c == '_') c = ' ';
    }
    return std::string(recortar(nombre));
}

std::string nombre_camara(std::string_view ruta_dispositivo) {
    // Solo /dev/videoN. Cualquier otra cosa no es una camara V4L2 y no tiene
    // entrada en /sys/class/video4linux.
    constexpr std::string_view kPrefijo = "/dev/";
    if (!empieza_por(ruta_dispositivo, kPrefijo)) return {};
    const std::string_view nodo = ruta_dispositivo.substr(kPrefijo.size());
    if (nodo.empty() || nodo.find('/') != std::string_view::npos) return {};

    std::ifstream f("/sys/class/video4linux/" + std::string(nodo) + "/name");
    std::string linea;
    if (!std::getline(f, linea)) return {};
    return limpiar_nombre_camara(linea);
}

std::vector<std::string> codecs_video_ofrecibles(const InfoGsr& info) {
    std::vector<std::string> lista;
    for (const auto& c : info.codecs_video) {
        // El unico nombre que --info imprime y -k no acepta. Se descarta por
        // nombre exacto y no por sufijo: si algun dia GSR añade otro, mejor
        // que aparezca y se vea, que descartarlo a ciegas por parecerse.
        if (c == "h264_software") continue;
        lista.push_back(c);
    }
    return lista;
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
