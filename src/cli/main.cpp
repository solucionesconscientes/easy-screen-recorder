// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Capa 2: el CLI. Regla dura del proyecto: si algo no funciona por aqui, no
// se toca la UI. Todo lo que hace sale de libesr; este fichero solo
// interpreta ordenes e imprime.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "esr/ajustes.hpp"
#include "esr/configuracion.hpp"
#include "esr/audio.hpp"
#include "esr/entorno.hpp"
#include "esr/grabacion.hpp"
#include "esr/version.hpp"

namespace {

// Convenio de codigos de salida, para que los scripts se fien:
//   0 = bien; 1 = la operacion fallo; 2 = la orden esta mal escrita.
constexpr int kBien = 0;
constexpr int kFallo = 1;
constexpr int kMalUso = 2;

void uso() {
    std::printf(
        "easy-screen-recorder-cli %s: grabador de pantalla sobre gpu-screen-recorder\n"
        "\n"
        "Uso:\n"
        "  easy-screen-recorder-cli grabar [opciones]  empieza a grabar la pantalla y vuelve al instante\n"
        "  easy-screen-recorder-cli audio [opciones]   graba solo audio, sin video (via ffmpeg)\n"
        "  easy-screen-recorder-cli parar              para, guarda e imprime la ruta del fichero\n"
        "  easy-screen-recorder-cli emitir [opciones]  emite la pantalla en directo por RTMP\n"
        "  easy-screen-recorder-cli guardar            vuelca el buffer de replay a un fichero\n"
        "  easy-screen-recorder-cli reparar            arregla la grabacion que quedo a medias\n"
        "                              si el equipo se apago mientras grababa\n"
        "  easy-screen-recorder-cli pausar             pausa la grabacion en marcha\n"
        "  easy-screen-recorder-cli reanudar           reanuda la grabacion pausada\n"
        "  easy-screen-recorder-cli estado             dice si hay una grabacion en marcha\n"
        "  easy-screen-recorder-cli fuentes            lista las fuentes de captura de esta maquina\n"
        "  easy-screen-recorder-cli dispositivos       lista los dispositivos de audio\n"
        "  easy-screen-recorder-cli --check [--volcado FICHERO]\n"
        "                              comprueba el entorno y dice que falta\n"
        "  easy-screen-recorder-cli --version\n"
        "\n"
        "Opciones de grabar (todas con default sensato):\n"
        "  --fuente F        un monitor (p. ej. eDP-1), «region», «portal», «focused»\n"
        "                    o una camara /dev/videoN. Sin ella: el primer monitor\n"
        "  --region WxH+X+Y  el recorte, solo con --fuente region\n"
        "  --salida FICHERO  el destino. Sin el: easy-screen-recorder-FECHA.mkv en Videos\n"
        "  --codec C         codec de video (h264, hevc, vp8...). Sin el decide GSR,\n"
        "                    que ademas mira el contenedor: un .webm solo admite vp8,\n"
        "                    vp9 o av1, y un .mp4 no admite vp8. Mira «--check» para\n"
        "                    saber cuales soporta esta maquina\n"
        "  --codec-audio C   aac, opus o flac. Por defecto opus\n"
        "  --audio DISP      una pista de audio; repetible. Por defecto default_output.\n"
        "                    Con «a|b» las dos fuentes van MEZCLADAS en una sola pista;\n"
        "                    repitiendo --audio va cada una en la suya. Ojo: con pistas\n"
        "                    separadas casi todos los reproductores suenan solo la primera\n"
        "  --sin-audio       grabar sin ninguna pista de audio\n"
        "  --fps N           por defecto 60\n"
        "  --calidad Q       medium, high, very_high o ultra. Por defecto very_high\n"
        "  --camara D        superpone una camara (/dev/videoN) sobre la pantalla, en\n"
        "                    la MISMA grabacion: lo compone GSR en vivo\n"
        "  --camara-tamano N  ancho de la camara en %% del video, del 5 al 50. Por\n"
        "                    defecto 25\n"
        "  --camara-x N      posicion horizontal de su esquina superior izquierda,\n"
        "                    en %% del video. Por defecto 73\n"
        "  --camara-y N      lo mismo en vertical. Por defecto 73, o sea abajo a la\n"
        "                    derecha, que es donde menos tapa\n"
        "  --camara-sin-espejo  la camara sale como la ve ella, no como un espejo.\n"
        "                    Por defecto va espejada, que es como te reconoces\n"
        "  --modo-fotogramas M  cfr, vfr o content. «content» solo codifica cuando la\n"
        "                    pantalla cambia: menos consumo y menos tamaño en un\n"
        "                    tutorial con pausas. Sin el, el default de GSR (vfr)\n"
        "  --limite-resolucion WxH  escala la salida para caber ahi, respetando la\n"
        "                    proporcion. Por ejemplo 1920x1080 grabando en 4K\n"
        "  --replay N        modo replay: guarda en memoria los ultimos N segundos y no\n"
        "                    escribe nada hasta que se lo pides con «guardar». De 2 a\n"
        "                    86400. Ojo: aqui --salida es una CARPETA, no un fichero\n"
        "  --contenedor C    mkv, mp4 o webm. Solo en modo replay; fuera de el sale de\n"
        "                    la extension de --salida\n"
        "\n"
        "Opciones de emitir (acepta tambien las de grabar, menos --salida):\n"
        "  --url U           el servidor de ingesta, por ejemplo\n"
        "                    rtmp://a.rtmp.youtube.com/live2. NO la clave\n"
        "  --clave C         la clave de emision. NO se guarda en ningun sitio;\n"
        "                    hay que darla cada vez\n"
        "  --bitrate N       kbps constantes. Por defecto 4500, que es lo que\n"
        "                    recomienda YouTube para 1080p30\n"
        "\n"
        "  Emitiendo, «pausar» funciona pero deja de mandar imagen, y una\n"
        "  plataforma trata eso como emision caida. Se avisa y se deja hacer.\n"
        "\n"
        "Opciones de audio:\n"
        "  --dispositivo D   default_output (audio del sistema), default_input (el micro)\n"
        "                    o un nombre de «easy-screen-recorder-cli dispositivos».\n"
        "                    Por defecto, el audio del sistema\n"
        "  --formato F       opus, aac, flac, wav o mp3. Por defecto opus\n"
        "  --bitrate N       kbps del audio con perdida. Sin el, lo decide el codec.\n"
        "                    No aplica a flac ni wav, que no pierden nada\n"
        "  --salida FICHERO  el destino. Sin el: easy-screen-recorder-FECHA.<ext> en Musica,\n"
        "                    con la extension del formato elegido\n"
        "\n"
        "Codigos de salida: 0 bien, 1 fallo de la operacion, 2 orden mal escrita.\n",
        std::string(esr::kVersionEsr).c_str());
}

std::string rellenar(std::string s, std::size_t ancho) {
    while (s.size() < ancho) s.push_back(' ');
    return s;
}

void imprimir_herramienta(const esr::Herramienta& h) {
    std::string estado;
    if (!h.evaluada) {
        estado = "sin mirar";
    } else if (!h.presente) {
        estado = "ausente";
    } else {
        estado = "presente";
    }

    std::string cola;
    if (h.presente && h.version) {
        cola = h.version->texto();
        if (!h.ruta.empty()) cola += "  " + h.ruta;
    } else if (h.presente && !h.ruta.empty()) {
        cola = h.ruta;
    }
    // Por que via se encontro. Importa: un GSR en flatpak no ve el /tmp del
    // sistema, asi que el fichero de salida no puede ir alli (docs/gsr-ipc.md).
    if (h.presente && !h.origen.empty() && h.origen != "PATH") {
        cola += "  [" + h.origen + "]";
    }
    if (!h.diagnostico.empty()) {
        if (!cola.empty()) cola += "  ";
        cola += h.diagnostico;
    }

    std::printf("  %s%s%s\n", rellenar(h.nombre, 22).c_str(), rellenar(estado, 11).c_str(),
                cola.c_str());
}

int imprimir_check(const esr::Entorno& e) {
    std::printf("easy-screen-recorder-cli %s: comprobacion del entorno\n\n",
                std::string(esr::kVersionEsr).c_str());

    std::printf("Herramientas\n");
    imprimir_herramienta(e.gsr);
    imprimir_herramienta(e.gsr_cli);
    imprimir_herramienta(e.ffmpeg);
    imprimir_herramienta(e.ffprobe);

    std::printf("\nVersion de gpu-screen-recorder\n");
    if (e.capacidades.version_gsr) {
        std::printf("  detectada: %s\n", e.capacidades.version_gsr->texto().c_str());
    } else {
        std::printf("  detectada: ninguna\n");
    }
    if (const auto minima = esr::version_minima_gsr()) {
        std::printf("  minima soportada: %s\n", minima->texto().c_str());
    }

    std::printf("\nCapacidades leidas del volcado\n");
    std::printf("  %s%zu\n", rellenar("fuentes de captura", 24).c_str(),
                e.capacidades.fuentes_captura.size());
    std::printf("  %s%zu\n", rellenar("dispositivos de audio", 24).c_str(),
                e.capacidades.dispositivos_audio.size());
    std::printf("  %s%zu\n", rellenar("audio por aplicacion", 24).c_str(),
                e.capacidades.audio_por_aplicacion.size());
    if (e.capacidades.info.presente) {
        std::string codecs;
        for (const auto& c : e.capacidades.info.codecs_video) {
            if (!codecs.empty()) codecs += ", ";
            codecs += c;
        }
        std::printf("  %s%s\n", rellenar("codecs de video", 24).c_str(), codecs.c_str());
        if (const auto mejor = esr::mejor_codec_hardware(e.capacidades.info)) {
            std::printf("  %s%s\n", rellenar("por defecto (hardware)", 24).c_str(),
                        mejor->c_str());
        } else {
            std::printf("  %sninguno por hardware: se grabaria por CPU\n",
                        rellenar("por defecto", 24).c_str());
        }
    }

    // Si GSR no esta instalado, los avisos del parser solo repiten esa misma
    // noticia con otras palabras. El diagnostico util es el de mas abajo.
    if (e.gsr.presente && !e.capacidades.avisos.empty()) {
        std::printf("\nEl parser no supo interpretar\n");
        for (const auto& a : e.capacidades.avisos) std::printf("  - %s\n", a.c_str());
    }

    if (e.carencias.empty()) {
        std::printf("\nNo falta nada.\n");
    } else {
        std::printf("\nQue falta\n");
        for (const auto& c : e.carencias) {
            std::printf("  [%s] %s\n", c.bloqueante ? "bloqueante" : "aviso    ", c.que.c_str());
            std::printf("      %s\n", c.consecuencia.c_str());
        }
    }

    std::printf("\nGrabacion de pantalla: %s\n", e.graba_pantalla() ? "posible" : "NO posible");
    // GSR no graba audio sin video: exige -w siempre (docs/gsr-audio-only.md).
    // El modo audio-only sale de ffmpeg o no sale.
    std::printf("Audio sin video:       %s\n",
                e.graba_audio_solo() ? "posible con ffmpeg (backend sin escribir)"
                                     : "NO posible: falta ffmpeg");
    std::printf("\nResultado: %s\n", e.listo() ? "LISTO" : "NO LISTO");

    return e.listo() ? kBien : kFallo;
}

int comprobar(const std::vector<std::string_view>& args) {
    if (args.size() == 3 && args[1] == "--volcado") {
        std::ifstream f{std::string(args[2])};
        if (!f) {
            std::fprintf(stderr, "easy-screen-recorder-cli: no se puede leer %s\n", std::string(args[2]).c_str());
            return kFallo;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        return imprimir_check(esr::detectar_desde_volcado(ss.str()));
    }
    if (args.size() != 1) {
        std::fprintf(stderr, "easy-screen-recorder-cli: --check va solo o con --volcado FICHERO\n");
        return kMalUso;
    }
    return imprimir_check(esr::detectar());
}

// El primer monitor de la lista de fuentes: lo que no es un modo especial ni
// una camara. Es el default de --fuente.
std::string primer_monitor(const esr::Capacidades& c) {
    for (const auto& f : c.fuentes_captura) {
        if (f.id == "region" || f.id == "portal" || f.id == "focused") continue;
        if (f.id.rfind("/dev/", 0) == 0) continue;
        return f.id;
    }
    return {};
}

// `grabar` y `emitir` comparten TODO el parseo: emitir es grabar con la salida
// puesta en una URL y el bitrate constante. Duplicar las quince banderas de
// grabar habria sido garantizar que una de las dos listas se queda vieja.
int grabar(const std::vector<std::string_view>& args, bool emitiendo = false) {
    esr::AjustesGrabacion a;
    a.fuente.clear();
    bool audio_explicito = false;
    std::string url_servidor;
    std::string clave;

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view opcion = args[i];
        const auto valor = [&]() -> std::string_view {
            return i + 1 < args.size() ? args[++i] : std::string_view{};
        };
        if (opcion == "--fuente") a.fuente = std::string(valor());
        else if (opcion == "--region") a.region = std::string(valor());
        else if (opcion == "--salida") a.salida = std::string(valor());
        else if (opcion == "--codec") a.codec_video = std::string(valor());
        else if (opcion == "--codec-audio") a.codec_audio = std::string(valor());
        else if (opcion == "--audio") {
            if (!audio_explicito) a.audios.clear();
            audio_explicito = true;
            a.audios.emplace_back(valor());
        } else if (opcion == "--sin-audio") {
            a.sin_audio = true;
            if (!audio_explicito) a.audios.clear();
        } else if (opcion == "--fps") {
            a.fps = std::atoi(std::string(valor()).c_str());
        } else if (opcion == "--calidad") {
            a.calidad = std::string(valor());
        } else if (opcion == "--camara") {
            a.camara = std::string(valor());
        } else if (opcion == "--camara-tamano") {
            a.camara_ancho_pct = std::atoi(std::string(valor()).c_str());
        } else if (opcion == "--camara-x") {
            a.camara_x_pct = std::atoi(std::string(valor()).c_str());
        } else if (opcion == "--camara-y") {
            a.camara_y_pct = std::atoi(std::string(valor()).c_str());
        } else if (opcion == "--camara-sin-espejo") {
            a.camara_espejo = false;
        } else if (opcion == "--modo-fotogramas") {
            a.modo_fotogramas = std::string(valor());
        } else if (opcion == "--limite-resolucion") {
            a.limite_resolucion = std::string(valor());
        } else if (opcion == "--replay") {
            a.replay_segundos = std::atoi(std::string(valor()).c_str());
        } else if (opcion == "--contenedor") {
            a.contenedor = std::string(valor());
        } else if (opcion == "--url") {
            url_servidor = std::string(valor());
        } else if (opcion == "--clave") {
            clave = std::string(valor());
        } else if (opcion == "--bitrate") {
            a.bitrate_kbps = std::atoi(std::string(valor()).c_str());
        } else {
            std::fprintf(stderr, "easy-screen-recorder-cli: no entiendo «%.*s»\n\n",
                         static_cast<int>(opcion.size()), opcion.data());
            uso();
            return kMalUso;
        }
    }

    // Los dos defaults que necesitan mirar la maquina se resuelven solo si
    // hacen falta: la deteccion cuesta mas de un segundo con el flatpak.
    if (a.fuente.empty()) {
        const esr::Entorno e = esr::detectar();
        a.fuente = primer_monitor(e.capacidades);
        if (a.fuente.empty()) {
            std::fprintf(stderr,
                         "easy-screen-recorder-cli: no se detecta ningun monitor que grabar.\n"
                         "Mira «easy-screen-recorder-cli fuentes» y elige una con --fuente. Si la lista\n"
                         "esta vacia y la pantalla esta encendida, ejecuta «easy-screen-recorder-cli --check»\n");
            return kFallo;
        }
    }
    if (emitiendo) {
        if (url_servidor.empty()) {
            url_servidor = esr::url_emision_recordada();
        }
        if (url_servidor.empty()) {
            std::fprintf(stderr,
                         "easy-screen-recorder-cli: falta --url con el servidor de ingesta.\n"
                         "Para YouTube es rtmp://a.rtmp.youtube.com/live2; la clave va aparte\n"
                         "en --clave y no se guarda\n");
            return kMalUso;
        }
        if (clave.empty()) {
            std::fprintf(stderr, "easy-screen-recorder-cli: falta --clave\n");
            return kMalUso;
        }
        a.salida = esr::url_de_emision(url_servidor, clave);
        a.contenedor = "flv";
        a.modo_bitrate = "cbr";
        // aac y no opus: flv solo respeta aac, y pedir opus acaba en aac sin
        // avisar. Se pone aqui, no se deja al default de AjustesGrabacion, que
        // es opus porque es el bueno para un fichero.
        if (a.codec_audio == "opus") a.codec_audio = "aac";
        // 4500 kbps es lo que YouTube recomienda para 1080p a 30 imagenes por
        // segundo. Se pone aqui y no en AjustesGrabacion porque fuera de una
        // emision este numero no significa nada.
        if (a.bitrate_kbps == 0) a.bitrate_kbps = 4500;
        // La URL del SERVIDOR se recuerda; la clave jamas. Asi la proxima vez
        // solo hay que pegar la clave.
        esr::recordar_url_emision(url_servidor);
    }
    if (a.salida.empty()) {
        const std::string carpeta = esr::carpeta_videos_elegida();
        std::error_code ec;
        std::filesystem::create_directories(carpeta, ec);
        // En replay la salida es la carpeta a secas: el nombre de cada volcado
        // lo pone GSR.
        a.salida = a.replay_segundos != 0 ? carpeta : esr::nombre_por_defecto(carpeta);
    }

    const auto sesion = esr::sesion_por_defecto();
    const auto r = esr::empezar_grabacion(a, sesion);
    if (!r.en_marcha) {
        std::fprintf(stderr, "easy-screen-recorder-cli: no se pudo empezar: %s\n", r.motivo.c_str());
        return kFallo;
    }

    if (emitiendo) {
        // La clave NO se imprime: esta salida acaba en logs y en capturas de
        // pantalla, y con la clave cualquiera emite en tu canal.
        std::printf("emitiendo %s -> %s/… a %d kbps\n", esr::fuente_gsr(a).c_str(),
                    url_servidor.c_str(), a.bitrate_kbps);
        std::printf("termina con: easy-screen-recorder-cli parar\n");
        return kBien;
    }
    std::printf("grabando %s -> %s\n", esr::fuente_gsr(a).c_str(), a.salida.c_str());
    if (a.replay_segundos != 0) {
        std::printf("modo replay: no se escribe nada hasta que pidas\n"
                    "  easy-screen-recorder-cli guardar    vuelca los ultimos %d s\n"
                    "  easy-screen-recorder-cli parar      termina sin guardar nada mas\n",
                    a.replay_segundos);
    } else {
        std::printf("para y guarda con: easy-screen-recorder-cli parar\n");
    }
    return kBien;
}

int guardar() {
    const auto r = esr::guardar_replay(esr::sesion_por_defecto());
    if (!r.parado) {
        std::fprintf(stderr, "easy-screen-recorder-cli: %s\n", r.motivo.c_str());
        return kFallo;
    }
    if (r.ruta_fichero.empty()) {
        std::printf("guardado, pero el grabador no dijo donde\n");
        return kBien;
    }
    std::printf("%s\n", r.ruta_fichero.c_str());
    return kBien;
}

int audio(const std::vector<std::string_view>& args) {
    esr::AjustesAudio a;
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view opcion = args[i];
        const auto valor = [&]() -> std::string_view {
            return i + 1 < args.size() ? args[++i] : std::string_view{};
        };
        if (opcion == "--dispositivo") a.dispositivo = std::string(valor());
        else if (opcion == "--formato") a.formato = std::string(valor());
        else if (opcion == "--salida") a.salida = std::string(valor());
        else if (opcion == "--bitrate") {
            const std::string texto(valor());
            char* fin = nullptr;
            const long kbps = std::strtol(texto.c_str(), &fin, 10);
            // Se valida aqui y no en validar_audio() porque «abc» no es un
            // bitrate fuera de rango, es una orden mal escrita, y eso es un
            // codigo de salida distinto.
            if (texto.empty() || fin == nullptr || *fin != '\0') {
                std::fprintf(stderr, "easy-screen-recorder-cli: --bitrate espera un numero de kbps\n");
                return kMalUso;
            }
            a.bitrate_kbps = static_cast<int>(kbps);
        }
        else {
            std::fprintf(stderr, "easy-screen-recorder-cli: no entiendo «%.*s»\n\n",
                         static_cast<int>(opcion.size()), opcion.data());
            uso();
            return kMalUso;
        }
    }
    if (a.salida.empty()) {
        const std::string carpeta = esr::carpeta_audio_elegida();
        std::error_code ec;
        std::filesystem::create_directories(carpeta, ec);
        a.salida = esr::nombre_por_defecto(carpeta, esr::extension_por_defecto(a.formato));
    }

    const auto r = esr::empezar_audio(a, esr::sesion_audio_por_defecto());
    if (!r.bien) {
        std::fprintf(stderr, "easy-screen-recorder-cli: no se pudo empezar: %s\n", r.motivo.c_str());
        return kFallo;
    }
    std::printf("grabando audio (%s) -> %s\n", a.dispositivo.c_str(), r.ruta_fichero.c_str());
    std::printf("para y guarda con: easy-screen-recorder-cli parar\n");
    return kBien;
}

int reparar() {
    const auto sesion = esr::sesion_por_defecto();
    const std::string ruta = esr::grabacion_a_medias(sesion);
    if (ruta.empty()) {
        std::printf("no hay ninguna grabacion a medias\n");
        return kBien;
    }
    std::printf("rehaciendo %s...\n", ruta.c_str());
    std::string motivo;
    if (!esr::reparar_grabacion(ruta, motivo)) {
        std::fprintf(stderr, "easy-screen-recorder-cli: %s\n", motivo.c_str());
        return kFallo;
    }
    std::printf("%s\n", ruta.c_str());
    return kBien;
}

int parar() {
    // Puede haber una grabacion de pantalla o una de audio; se para la que
    // este. Las dos a la vez tambien: primero la pantalla.
    const auto sesion_audio = esr::sesion_audio_por_defecto();
    if (!esr::grabacion_en_marcha(esr::sesion_por_defecto()) &&
        esr::audio_en_marcha(sesion_audio)) {
        const auto ra = esr::parar_audio(sesion_audio);
        if (!ra.bien) {
            std::fprintf(stderr, "easy-screen-recorder-cli: %s\n", ra.motivo.c_str());
            return kFallo;
        }
        std::printf("%s\n", ra.ruta_fichero.c_str());
        return kBien;
    }

    const auto r = esr::parar_grabacion(esr::sesion_por_defecto());
    if (!r.parado) {
        std::fprintf(stderr, "easy-screen-recorder-cli: %s\n", r.motivo.c_str());
        return kFallo;
    }
    if (r.ruta_fichero.empty()) {
        // El stop de un replay no guarda fichero (docs/gsr-ipc.md).
        std::printf("parado; esta grabacion no guardaba fichero\n");
    } else {
        std::printf("%s\n", r.ruta_fichero.c_str());
    }
    return kBien;
}

int pausar(bool pausada) {
    // Emitiendo, GSR acepta la pausa —comprobado: responde «Paused»— pero deja
    // de mandar imagen, y YouTube o Twitch tratan eso como emision caida. Se
    // avisa y se deja hacer: quien lo pide por linea de comandos sabra por que.
    if (pausada && !esr::emision_en_marcha(esr::sesion_por_defecto().ruta_emision).empty()) {
        std::fprintf(stderr,
                     "easy-screen-recorder-cli: ojo, estas emitiendo. Pausar corta la imagen que\n"
                     "sale y la plataforma puede dar la emision por caida\n");
    }
    std::string motivo;
    if (!esr::poner_pausa(esr::sesion_por_defecto(), pausada, motivo)) {
        std::fprintf(stderr, "easy-screen-recorder-cli: %s\n", motivo.c_str());
        return kFallo;
    }
    std::printf("%s\n", pausada ? "pausada" : "grabando otra vez");
    return kBien;
}

int estado() {
    const bool pantalla = esr::grabacion_en_marcha(esr::sesion_por_defecto());
    const bool audio_solo = esr::audio_en_marcha(esr::sesion_audio_por_defecto());
    if (pantalla && audio_solo) {
        std::printf("grabando pantalla y audio\n");
        return kBien;
    }
    if (pantalla) {
        std::printf("grabando\n");
        return kBien;
    }
    if (audio_solo) {
        std::printf("grabando audio\n");
        return kBien;
    }
    std::printf("sin grabacion\n");
    return kFallo;
}

int fuentes() {
    const esr::Entorno e = esr::detectar();
    if (!e.gsr.presente) {
        std::fprintf(stderr, "easy-screen-recorder-cli: gpu-screen-recorder no esta; ejecuta «easy-screen-recorder-cli --check»\n");
        return kFallo;
    }
    if (e.capacidades.fuentes_captura.empty()) {
        std::printf("ninguna fuente. Si la pantalla estaba apagada, enciendela y repite:\n"
                    "sin plano de video activo GSR no lista el monitor\n");
        return kFallo;
    }
    for (const auto& f : e.capacidades.fuentes_captura) {
        std::printf("  %s%s\n", rellenar(f.id, 16).c_str(), f.detalle().c_str());
    }
    return kBien;
}

int dispositivos() {
    const esr::Entorno e = esr::detectar();
    if (!e.gsr.presente) {
        // Sin GSR el modo audio-only sigue en pie, asi que la lista sale de
        // pactl y sirve para --dispositivo.
        std::string motivo;
        const auto fuentes_pactl = esr::fuentes_audio_pactl(motivo);
        if (fuentes_pactl.empty()) {
            std::fprintf(stderr, "easy-screen-recorder-cli: sin GSR y sin pactl no hay lista: %s\n",
                         motivo.c_str());
            return kFallo;
        }
        std::printf("dispositivos de audio segun pactl (GSR no esta):\n");
        for (const auto& f : fuentes_pactl) {
            std::printf("  %s%s\n", rellenar(f.nombre, 52).c_str(), f.detalle.c_str());
        }
        return kBien;
    }
    std::printf("dispositivos de audio (--audio):\n");
    for (const auto& d : e.capacidades.dispositivos_audio) {
        std::printf("  %s%s\n", rellenar(d.id, 52).c_str(), d.detalle().c_str());
    }
    if (!e.capacidades.audio_por_aplicacion.empty()) {
        std::printf("\naplicaciones sonando ahora (--audio app:NOMBRE):\n");
        for (const auto& d : e.capacidades.audio_por_aplicacion) {
            std::printf("  %s\n", d.id.c_str());
        }
    }
    return kBien;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    if (args.empty()) {
        uso();
        return kBien;
    }

    const std::string_view orden = args[0];
    if (orden == "--version" || orden == "-v") {
        std::printf("easy-screen-recorder-cli %s\n", std::string(esr::kVersionEsr).c_str());
        return kBien;
    }
    if (orden == "--help" || orden == "-h") {
        uso();
        return kBien;
    }
    if (orden == "--check") return comprobar(args);
    if (orden == "grabar") return grabar(args);
    if (orden == "emitir") return grabar(args, true);
    if (orden == "audio") return audio(args);
    if (orden == "parar") return parar();
    if (orden == "guardar") return guardar();
    if (orden == "reparar") return reparar();
    if (orden == "pausar") return pausar(true);
    if (orden == "reanudar") return pausar(false);
    if (orden == "estado") return estado();
    if (orden == "fuentes") return fuentes();
    if (orden == "dispositivos") return dispositivos();

    std::fprintf(stderr, "easy-screen-recorder-cli: no entiendo esa orden\n\n");
    uso();
    return kMalUso;
}
