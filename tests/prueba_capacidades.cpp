// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/capacidades.hpp"

#include <algorithm>
#include <string>

#include "esr/entorno.hpp"
#include "comprobar.hpp"

using namespace esr;

namespace {

std::string fixture(const std::string& nombre) {
    return prueba::leer(std::string(ESR_DIR_FIXTURES) + "/" + nombre);
}

bool algun_aviso_contiene(const Capacidades& c, std::string_view trozo) {
    return std::any_of(c.avisos.begin(), c.avisos.end(), [&](const std::string& a) {
        return a.find(trozo) != std::string::npos;
    });
}

const Opcion* buscar(const std::vector<Opcion>& v, std::string_view id) {
    const auto it = std::find_if(v.begin(), v.end(), [&](const Opcion& o) { return o.id == id; });
    return it == v.end() ? nullptr : &*it;
}

// --- Volcado real de esta maquina -------------------------------------------
// GSR 6.0.0 en flatpak, KDE Plasma sobre Wayland, GPU Intel. Este test es el
// que ata el parser a la realidad: si GSR cambia el formato de sus listas,
// falla aqui y no en la UI.
void volcado_real() {
    const std::string texto = prueba::leer(ESR_VOLCADO_REAL);
    COMPROBAR_NOTA(!texto.empty(), "docs/gsr-capabilities.txt no deberia estar vacio");

    // Cinco desde la Tanda 4: --list-capture-options ya no se sondea porque
    // --info trae la seccion capture_options identica (commands.c:268-269).
    const auto bloques = partir_volcado(texto);
    COMPROBAR_NOTA(bloques.size() == 5, "bloques=" + std::to_string(bloques.size()));

    const auto c = interpretar_volcado(texto);
    COMPROBAR(c.gsr_respondio);
    COMPROBAR(c.gsr_cli_respondio);
    COMPROBAR_NOTA(c.version_gsr && *c.version_gsr == (Version{6, 0, 0}),
                   c.version_gsr ? c.version_gsr->texto() : "sin version");

    // La version instalada tiene que llegar a la minima, que es ella misma.
    const auto minima = version_minima_gsr();
    COMPROBAR_NOTA(minima.has_value(), "version_minima_gsr() ya no puede ser nullopt");
    COMPROBAR(minima && c.version_gsr && !(*c.version_gsr < *minima));

    // Fuentes de captura: el monitor, la region, la camara y el portal.
    COMPROBAR(!c.fuentes_captura.empty());
    // Si esto falla, lo primero que hay que mirar es si el volcado se regenero
    // con la pantalla apagada: sin plano de scanout activo GSR no lista el
    // monitor. Ver ESTADO.md, "con la pantalla apagada GSR miente".
    COMPROBAR_NOTA(buscar(c.fuentes_captura, "eDP-1") != nullptr,
                   "falta el monitor eDP-1: ¿volcado hecho con la pantalla apagada?");
    if (const Opcion* m = buscar(c.fuentes_captura, "eDP-1")) {
        COMPROBAR(m->campos.size() == 1 && m->campos[0] == "1366x768");
        COMPROBAR(m->detalle() == "1366x768");
    }
    // «region» y «portal» vienen sin separador en la misma lista que si lo
    // trae. Antes eso disparaba un aviso; es formato normal, no una anomalia.
    if (const Opcion* r = buscar(c.fuentes_captura, "region")) {
        COMPROBAR(r->campos.empty());
        COMPROBAR(r->detalle().empty());
    }
    COMPROBAR(buscar(c.fuentes_captura, "portal") != nullptr);

    // Lo que rompia la heuristica vieja: una camara trae TRES campos.
    if (const Opcion* v = buscar(c.fuentes_captura, "/dev/video0")) {
        COMPROBAR_NOTA(v->campos.size() == 2, "campos=" + std::to_string(v->campos.size()));
        COMPROBAR(v->campos.size() == 2 && v->campos[0] == "1280x720@30hz");
        COMPROBAR(v->campos.size() == 2 && v->campos[1] == "mjpeg");
    } else {
        COMPROBAR_NOTA(false,
                       "falta /dev/video0: este test da por hecho que la maquina del volcado "
                       "tiene camara. Si no la tiene, el caso lo cubre igual el fixture sintetico");
    }

    // Dispositivos de audio: siempre «nombre|descripcion».
    COMPROBAR(!c.dispositivos_audio.empty());
    if (const Opcion* d = buscar(c.dispositivos_audio, "default_output")) {
        COMPROBAR(d->detalle() == "Default output");
    } else {
        COMPROBAR_NOTA(false, "falta default_output");
    }

    // Nada sonaba al hacer el volcado. Lista vacia con codigo 0 es un estado
    // legitimo, no un fallo, y no debe dejar aviso.
    COMPROBAR(c.audio_por_aplicacion.empty());

    // Con un volcado sano el parser no tiene nada que decir. Un aviso aqui
    // significa que el formato de GSR cambio.
    COMPROBAR_NOTA(c.avisos.empty(), c.avisos.empty() ? "" : c.avisos.front());

    const Entorno e = detectar_desde_volcado(texto);
    COMPROBAR(e.graba_pantalla());
    // ffmpeg no sale en el volcado: "sin mirar" no es "ausente".
    COMPROBAR(!e.ffmpeg.evaluada);

    // --- Lo que --info dice de esta maquina ---------------------------------
    COMPROBAR(c.info.presente);
    COMPROBAR_NOTA(c.info.servidor_grafico == "wayland", c.info.servidor_grafico);
    COMPROBAR(c.info.audio_por_aplicacion);
    COMPROBAR_NOTA(c.info.vendedor_gpu == "intel", c.info.vendedor_gpu);
    // Los codecs del volcado real: h264, h264_software, hevc, vp8. Sin AV1,
    // que esta GPU Intel no trae.
    COMPROBAR_NOTA(c.info.codecs_video.size() == 4,
                   "codecs=" + std::to_string(c.info.codecs_video.size()));
    COMPROBAR(std::find(c.info.codecs_video.begin(), c.info.codecs_video.end(), "hevc") !=
              c.info.codecs_video.end());
    COMPROBAR(c.info.formatos_imagen.size() == 2);

    // h264_software codifica en CPU (commands.c:75-76: sale solo porque
    // existe libx264). El default "mejor codec de hardware" no puede caer ahi.
    COMPROBAR(codec_es_hardware("h264"));
    COMPROBAR(codec_es_hardware("hevc_10bit_vulkan"));
    COMPROBAR(!codec_es_hardware("h264_software"));

    // El orden copia al upstream: h264 primero, como -k auto.
    const auto mejor = mejor_codec_hardware(c.info);
    COMPROBAR_NOTA(mejor && *mejor == "h264", mejor ? *mejor : "ninguno");
}

// Una seccion que no conocemos en --info no rompe, pero tampoco se interpreta
// en silencio. Y una maquina cuyo unico codec es software no tiene "mejor
// codec de hardware": inventarlo seria ofrecer algo que va a decepcionar.
void info_seccion_desconocida_y_solo_software() {
    const std::string texto =
        "### comando: gpu-screen-recorder --info\n"
        "### codigo: 0\n"
        "section=system_info\n"
        "display_server|x11\n"
        "supports_app_audio|no\n"
        "section=seccion_del_futuro\n"
        "dato|raro\n"
        "section=video_codecs\n"
        "h264_software\n"
        "### fin\n";
    const auto c = interpretar_volcado(texto);
    COMPROBAR(c.info.presente);
    COMPROBAR(c.info.servidor_grafico == "x11");
    COMPROBAR(!c.info.audio_por_aplicacion);
    COMPROBAR(c.info.codecs_video.size() == 1);
    COMPROBAR(algun_aviso_contiene(c, "seccion_del_futuro"));
    COMPROBAR(!mejor_codec_hardware(c.info).has_value());

    // Sin --list-capture-options, las fuentes salen de --info; aqui no habia
    // seccion capture_options y la lista queda vacia sin inventar nada.
    COMPROBAR(c.fuentes_captura.empty());
}

// --- Casos sinteticos --------------------------------------------------------

void sin_nada_instalado() {
    const auto texto = fixture("sin-nada-instalado.txt");
    const auto bloques = partir_volcado(texto);
    COMPROBAR_NOTA(bloques.size() == 6, "bloques=" + std::to_string(bloques.size()));

    const auto c = interpretar_volcado(texto);
    COMPROBAR(!c.gsr_respondio);
    COMPROBAR(!c.gsr_cli_respondio);
    COMPROBAR(!c.version_gsr.has_value());
    COMPROBAR(c.fuentes_captura.empty());
    COMPROBAR(c.dispositivos_audio.empty());
    COMPROBAR(c.audio_por_aplicacion.empty());
    // Cuatro salidas que no se pudieron leer (--info y las tres listas),
    // cuatro avisos. Silencio seria mentira.
    COMPROBAR_NOTA(c.avisos.size() == 4, "avisos=" + std::to_string(c.avisos.size()));

    const Entorno e = detectar_desde_volcado(texto);
    COMPROBAR(!e.graba_pantalla());
    COMPROBAR(!e.listo());
    COMPROBAR(std::any_of(e.carencias.begin(), e.carencias.end(),
                          [](const Carencia& x) { return x.bloqueante; }));
}

// El comando del volcado ya no empieza por el nombre del binario cuando GSR
// viene en flatpak. El parser tiene que reconocerlo igual.
void volcado_de_flatpak() {
    const auto texto = fixture("sin-nada-instalado.txt");
    const auto c = interpretar_volcado(texto);
    COMPROBAR(!c.gsr_respondio);  // el fixture es de PATH y sigue funcionando

    const std::string flatpak =
        "### comando: flatpak run --command=gpu-screen-recorder com.dec05eba.gpu_screen_recorder --version\n"
        "### codigo: 0\n6.0.0\n### fin\n"
        "### comando: flatpak run --command=gpu-screen-recorder com.dec05eba.gpu_screen_recorder --list-audio-devices\n"
        "### codigo: 0\ndefault_output|Default output\n### fin\n"
        "### comando: flatpak run --command=gsr-cli com.dec05eba.gpu_screen_recorder --help\n"
        "### codigo: 0\nusage: gsr-cli -ipc <socket_path> <command>\n### fin\n";
    const auto f = interpretar_volcado(flatpak);
    COMPROBAR(f.gsr_respondio);
    COMPROBAR(f.gsr_cli_respondio);
    COMPROBAR(f.version_gsr && *f.version_gsr == (Version{6, 0, 0}));
    COMPROBAR(f.dispositivos_audio.size() == 1);
    COMPROBAR_NOTA(f.avisos.empty(), f.avisos.empty() ? "" : f.avisos.front());
}

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

    // 1.2.3 queda por debajo de la minima real, que ya esta fijada en 6.0.0.
    const auto minima = version_minima_gsr();
    COMPROBAR(minima && c.version_gsr && *c.version_gsr < *minima);

    // Y eso tiene que salir como carencia bloqueante, no como aviso suelto.
    const Entorno e = detectar_desde_volcado(fixture("version-antigua.txt"));
    COMPROBAR(!e.listo());
    COMPROBAR(std::any_of(e.carencias.begin(), e.carencias.end(), [](const Carencia& x) {
        return x.bloqueante && x.que.find("por debajo de la minima") != std::string::npos;
    }));
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
        COMPROBAR(c.fuentes_captura[0].detalle() == "2560x1440");
        // "portal|" trae separador pero nada detras: sin campos, no un campo vacio.
        COMPROBAR(c.fuentes_captura[2].id == "portal");
        COMPROBAR(c.fuentes_captura[2].campos.empty());
        COMPROBAR(c.fuentes_captura[2].detalle().empty());
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
        COMPROBAR(c.dispositivos_audio[0].detalle().empty());
    }
    // --list-audio-devices SIEMPRE imprime «nombre|descripcion». Que no lo haga
    // significa que no entendemos su salida, y eso se dice.
    COMPROBAR(algun_aviso_contiene(c, "formato inesperado"));
}

// El reverso del anterior: aqui la ausencia de separador es lo normal y no
// puede salir ni un aviso.
void audio_por_aplicacion_sin_separador() {
    const auto c = interpretar_volcado(fixture("audio-aplicaciones.txt"));
    COMPROBAR_NOTA(c.audio_por_aplicacion.size() == 3,
                   "apps=" + std::to_string(c.audio_por_aplicacion.size()));
    if (c.audio_por_aplicacion.size() == 3) {
        COMPROBAR(c.audio_por_aplicacion[0].id == "firefox");
        COMPROBAR(c.audio_por_aplicacion[0].campos.empty());
        COMPROBAR(c.audio_por_aplicacion[2].id == "Telegram Desktop");
    }
    COMPROBAR_NOTA(c.avisos.empty(), c.avisos.empty() ? "" : c.avisos.front());
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

// Una lista de captura vacia si es sospechosa: significa que la maquina no
// ofrece ni monitor ni portal ni camara.
void bloque_vacio_pero_correcto() {
    BloqueVolcado b;
    b.comando = "gpu-screen-recorder --list-capture-options";
    b.codigo = 0;
    b.tiene_codigo = true;
    std::vector<std::string> avisos;
    const auto opciones = interpretar_lista(b, avisos, FormatoLista{});
    COMPROBAR(opciones.empty());
    COMPROBAR(avisos.size() == 1);
    COMPROBAR(avisos.size() == 1 && avisos[0].find("ninguna entrada") != std::string::npos);

    // La misma lista vacia, marcada como "vacia es normal": ni un aviso.
    std::vector<std::string> callado;
    COMPROBAR(interpretar_lista(b, callado, FormatoLista{false, true}).empty());
    COMPROBAR(callado.empty());
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

// GSR escribe sus errores por stderr y el volcado los mezcla con la salida.
// Una linea de error no puede acabar como una fuente de captura ofrecida.
void errores_de_gsr_no_son_opciones() {
    BloqueVolcado b;
    b.comando = "gpu-screen-recorder --list-capture-options";
    b.codigo = 0;
    b.tiene_codigo = true;
    b.salida = "gsr error: failed to get /dev/dri/renderDXXX file from /dev/dri/card9\neDP-1|1366x768\n";
    std::vector<std::string> avisos;
    const auto opciones = interpretar_lista(b, avisos, FormatoLista{});
    COMPROBAR_NOTA(opciones.size() == 1, "opciones=" + std::to_string(opciones.size()));
    COMPROBAR(opciones.size() == 1 && opciones[0].id == "eDP-1");
}

}  // namespace

int main() {
    volcado_real();
    info_seccion_desconocida_y_solo_software();
    sin_nada_instalado();
    volcado_de_flatpak();
    entrada_vacia();
    formato_inesperado();
    version_antigua();
    listas_con_separador();
    listas_sin_separador();
    audio_por_aplicacion_sin_separador();
    comando_fallido();
    volcado_truncado();
    bloque_vacio_pero_correcto();
    bloque_sin_codigo();
    errores_de_gsr_no_son_opciones();
    return prueba::resumen("prueba_capacidades");
}
