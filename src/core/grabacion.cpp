// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/configuracion.hpp"
#include "esr/grabacion.hpp"

#include <unistd.h>

#include <algorithm>
#include <string_view>
#include <charconv>
#include <array>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <thread>

#include "esr/entorno.hpp"
#include "esr/ipc.hpp"
#include "esr/proceso.hpp"

#include <stdexcept>
#include <string>

namespace esr {
namespace {

// Cuanto se espera a que el socket IPC aparezca tras lanzar GSR. Con el
// flatpak frio la primera sonda de esta maquina tardo hasta 746 ms; el portal
// ademas abre un dialogo que el usuario tiene que aceptar, asi que corto no
// puede ser.
constexpr int kEsperaSocketMs = 15000;
constexpr int kPasoEsperaMs = 100;

// Limite para los comandos con respuesta inmediata. gsr-cli usa 10 s
// (tools/gsr-cli/main.c:18-19); se copia.
constexpr int kLimiteInmediatoMs = 10000;

// La duracion que ffprobe le saca a un fichero, o 0 si no se la saca.
//
// Es el criterio para saber si un fichero esta bien cerrado: al que le falta el
// cierre, ffprobe le devuelve «N/A». Se usa ffprobe y no una lectura propia
// porque es lo que hace cualquier reproductor y aqui lo que importa es si el
// fichero se va a poder abrir.
// Numeros con punto decimal SIEMPRE, den igual el idioma del sistema.
//
// Esto no es puntillismo: costo un rato encontrarlo. ffprobe escribe «2.967» y
// ffmpeg espera «2.967», pero std::stod y std::to_string miran la configuracion
// regional, y Qt la pone al arrancar la ventana. En un sistema en español,
// std::stod("2.967") se para en el punto y devuelve DOS, y std::to_string(0.5)
// escribe «0,500000», que ffmpeg no entiende.
//
// Resultado: el recorte funcionaba desde el CLI —que no toca la configuracion
// regional— y se negaba desde la ventana, diciendo que la grabacion duraba dos
// segundos cuando duraba tres. from_chars y to_chars no miran el idioma: por eso
// estan aqui y no la pareja de siempre.
double texto_a_double(std::string_view texto) {
    double valor = 0.0;
    const auto* fin = texto.data() + texto.size();
    const auto r = std::from_chars(texto.data(), fin, valor);
    return r.ec == std::errc() ? valor : 0.0;
}

std::string double_a_texto(double valor) {
    std::array<char, 32> buf{};
    const auto r = std::to_chars(buf.data(), buf.data() + buf.size(), valor,
                                 std::chars_format::fixed, 3);
    return r.ec == std::errc() ? std::string(buf.data(), r.ptr) : std::string("0");
}

double duracion_de(const std::string& ruta) {
    const auto r = ejecutar("ffprobe", {"-v", "error", "-show_entries", "format=duration",
                                        "-of", "csv=p=0", ruta});
    if (!r.ejecutado || r.codigo != 0) return 0.0;
    try {
        return texto_a_double(r.salida);
    } catch (const std::exception&) {
        return 0.0;  // «N/A» y cualquier otra cosa que no sea un numero
    }
}

std::string leer_pid(const std::string& ruta) {
    std::ifstream f(ruta);
    std::string linea;
    std::getline(f, linea);
    return linea;
}

// Los motivos de error que el IPC devuelve en ingles, traducidos. Los textos
// exactos salen de src/cli/ipc.c:424-432 y 643-660 de GSR 6.0.0.
std::string traducir_error_ipc(const std::string& data) {
    if (data.find("already stopping") != std::string::npos) {
        return "la grabacion ya se esta parando; la primera peticion sigue su curso";
    }
    if (data.find("exited before the request finished") != std::string::npos) {
        return "el grabador termino antes de completar la peticion; la grabacion puede no haberse guardado";
    }
    if (data.find("failed to save") != std::string::npos) {
        return "no se pudo guardar la grabacion (respuesta de GSR: " + data + ")";
    }
    if (data.find("option -r is required") != std::string::npos) {
        return "eso solo vale en modo replay, y esta grabacion no lo es";
    }
    return "el grabador respondio con un error: " + data;
}

}  // namespace

long inicio_grabacion(const std::string& ruta_inicio) {
    std::ifstream f(ruta_inicio);
    long t = 0;
    f >> t;
    return f ? t : 0;
}

SesionGrabacion sesion_por_defecto() {
    const char* hogar = std::getenv("HOME");
    const std::string casa = (hogar != nullptr && hogar[0] != '\0') ? hogar : ".";
    SesionGrabacion s;
    s.dir = casa + "/.cache/easy-screen-recorder/sesion";
    s.ruta_socket = s.dir + "/ipc.sock";
    s.ruta_log = s.dir + "/gsr.log";
    s.ruta_pid = s.dir + "/gsr.pid";
    s.ruta_inicio = s.dir + "/inicio.txt";
    s.ruta_replay = s.dir + "/replay.txt";
    s.ruta_salida = s.dir + "/salida.txt";
    s.ruta_emision = s.dir + "/emision.txt";
    s.ruta_guardado = s.dir + "/guardado.txt";
    return s;
}

std::string emision_en_marcha(const std::string& ruta_emision) {
    std::ifstream f(ruta_emision);
    std::string servidor;
    if (!std::getline(f, servidor)) return {};
    return servidor;
}

int replay_en_marcha(const std::string& ruta_replay) {
    std::ifstream f(ruta_replay);
    int segundos = 0;
    f >> segundos;
    return f ? segundos : 0;
}

std::string diagnostico_de_log(const std::string& ruta_log) {
    std::ifstream f(ruta_log);
    if (!f) return "y su log no se puede leer (" + ruta_log + ")";

    std::string todo((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    // Fallos con causa conocida, comprobados en esta maquina (ESTADO.md de la
    // Tanda 2 y docs/gsr-audio-only.md).
    if (todo.find("no /dev/dri/card") != std::string::npos) {
        return "no hay ningun plano de video activo. Suele significar pantalla apagada o "
               "suspendida: enciendela y repite. Si persiste, la fuente pedida no existe";
    }
    if (todo.find("missing argument") != std::string::npos) {
        return "GSR rechazo los argumentos, y eso es un fallo de easy-screen-recorder-cli, no tuyo. "
               "Copia el log de " + ruta_log + " en un informe de error";
    }
    if (todo.find("No space left") != std::string::npos) {
        return "no queda espacio en disco para grabar";
    }

    // Sin causa conocida: las ultimas lineas del log tal cual, que es mejor
    // que un diagnostico inventado.
    std::string cola;
    std::size_t desde = todo.size();
    int lineas = 0;
    while (desde > 0 && lineas < 5) {
        --desde;
        if (todo[desde] == '\n' && desde + 1 < todo.size()) ++lineas;
    }
    cola = todo.substr(desde == 0 ? 0 : desde + 1);
    while (!cola.empty() && (cola.back() == '\n' || cola.back() == ' ')) cola.pop_back();
    if (cola.empty()) return "y su log quedo vacio: murio antes de decir nada";
    return "esto es el final de su log:\n" + cola;
}

namespace {

// Enciende por IPC la grabacion a fichero que corre en paralelo a la emision.
// Devuelve false y llena «motivo» si el grabador dice que no.
bool empezar_guardado(const SesionGrabacion& sesion, std::string& motivo) {
    std::string fallo;
    auto conexion = ConexionIpc::conectar(sesion.ruta_socket, &fallo);
    if (!conexion) {
        motivo = "no se pudo pedir que se guardara la emision: " + fallo;
        return false;
    }
    // Respuesta inmediata, no diferida: esta solo dice si empezo.
    const auto respuesta = conexion->pedir("start-replay-recording", kEsperaSocketMs, &fallo);
    if (!respuesta) {
        motivo = "no se pudo pedir que se guardara la emision: " + fallo;
        return false;
    }
    if (!respuesta->ok) {
        motivo = "el grabador no guarda la emision: " + traducir_error_ipc(respuesta->data);
        return false;
    }
    return true;
}

}  // namespace

ResultadoLanzamiento empezar_grabacion(const AjustesGrabacion& ajustes,
                                       const SesionGrabacion& sesion) {
    ResultadoLanzamiento r;

    if (grabacion_en_marcha(sesion)) {
        r.motivo = "ya hay una grabacion en marcha; parala con «easy-screen-recorder-cli parar»";
        return r;
    }

    const auto problemas = validar(ajustes);
    if (!problemas.empty()) {
        r.motivo = "los ajustes no valen:";
        for (const auto& p : problemas) r.motivo += "\n  - " + p;
        return r;
    }

    const auto inv = localizar_gsr("gpu-screen-recorder");
    if (!inv) {
        r.motivo = dentro_de_sandbox()
                       ? "gpu-screen-recorder no esta en el anfitrion. Instalalo ahi, no "
                         "dentro del sandbox: la captura la hace un proceso del anfitrion"
                       : "gpu-screen-recorder no esta ni en PATH ni como flatpak";
        return r;
    }

    // La trampa de /tmp, que ahora tiene tres versiones del mismo problema:
    // el fichero lo escribe un proceso que NO comparte /tmp con nosotros. Con
    // GSR en flatpak, su /tmp es suyo; desde dentro de un sandbox, el /tmp que
    // ve GSR es el del anfitrion y el nuestro es privado. En los dos casos el
    // video acaba en un /tmp que nadie va a mirar.
    // La trampa de /tmp no aplica a una URL: no hay fichero que acabe en el
    // sitio equivocado.
    if (!es_emision(ajustes.salida) && escribe_fuera_de_nuestro_sandbox(inv->origen) &&
        ajustes.salida.rfind("/tmp/", 0) == 0) {
        r.motivo = "el fichero no puede ir a /tmp: lo escribe otro proceso y su /tmp no es "
                   "el mismo que el tuyo, asi que el video se quedaria donde no lo ves. "
                   "Usa una ruta bajo tu home";
        return r;
    }

    std::error_code ec;
    std::filesystem::create_directories(sesion.dir, ec);
    if (ec) {
        r.motivo = "no se puede crear " + sesion.dir + ": " + ec.message();
        return r;
    }
    // Una URL no tiene carpeta que comprobar. Sin esta guarda, «rtmp:» se
    // interpretaba como un directorio inexistente y la emision no arrancaba.
    if (!es_emision(ajustes.salida)) {
        const auto dir_salida = std::filesystem::path(ajustes.salida).parent_path();
        if (!dir_salida.empty() && !std::filesystem::is_directory(dir_salida, ec)) {
            r.motivo = "la carpeta de destino no existe: " + dir_salida.string();
            return r;
        }
    }

    // El guion que GSR ejecutara al terminar, comprobado ANTES de lanzar: con un
    // -sc que no existe el grabador ni arranca, y el error sale en su log, que
    // es donde nadie mira.
    if (!ajustes.guion_al_terminar.empty()) {
        if (!std::filesystem::is_regular_file(ajustes.guion_al_terminar, ec)) {
            r.motivo = "el guion que se ejecuta al terminar no existe: " +
                       ajustes.guion_al_terminar;
            return r;
        }
        if (::access(ajustes.guion_al_terminar.c_str(), X_OK) != 0) {
            r.motivo = "el guion que se ejecuta al terminar no tiene permiso de ejecucion: " +
                       ajustes.guion_al_terminar;
            return r;
        }
    }

    // Y el espacio libre. Quedarse sin sitio a mitad de grabacion es el peor
    // fallo posible: te enteras al final y lo grabado no sirve. Aqui solo se
    // rechaza el caso indiscutible —menos de 100 MB, donde no cabe ni un minuto
    // de nada—; avisar a partir de ahi es cosa de la interfaz, que es quien
    // puede enseñar la cifra sin abortar.
    if (!es_emision(ajustes.salida) && !ajustes.salida.empty()) {
        const long libres = espacio_libre_mb(ajustes.salida);
        if (libres >= 0 && libres < 100) {
            r.motivo = "quedan " + std::to_string(libres) +
                       " MB libres donde ibas a grabar: no cabe una grabacion";
            return r;
        }
    }

    // La carpeta donde se va a guardar la emision, comprobada antes de lanzar:
    // GSR con un -ro que no existe arranca igual y el fallo solo sale en su log,
    // asi que el usuario emitiria creyendo que se esta guardando.
    if (!ajustes.carpeta_guardado.empty() &&
        !std::filesystem::is_directory(ajustes.carpeta_guardado, ec)) {
        r.motivo = "la carpeta donde guardar la emision no existe: " + ajustes.carpeta_guardado;
        return r;
    }

    // Un socket huerfano de un GSR muerto lo limpia GSR solo; cualquier otro
    // fichero en esa ruta le impide arrancar (src/cli/ipc.c:767-784). Mejor
    // decirlo aqui que dejar que muera con su error criptico en el log.
    if (std::filesystem::exists(sesion.ruta_socket, ec) &&
        !std::filesystem::is_socket(sesion.ruta_socket, ec)) {
        r.motivo = "en " + sesion.ruta_socket + " hay un fichero que no es un socket; quitalo";
        return r;
    }

    AjustesGrabacion con_ipc = ajustes;
    con_ipc.ruta_socket = sesion.ruta_socket;
    std::vector<std::string> args = inv->prefijo;
    const auto args_gsr = argumentos_gsr(con_ipc);
    args.insert(args.end(), args_gsr.begin(), args_gsr.end());

    const auto lanzado = lanzar_desatendido(inv->programa, args, sesion.ruta_log);
    if (!lanzado.ejecutado) {
        r.motivo = lanzado.motivo;
        return r;
    }

    std::ofstream(sesion.ruta_pid) << lanzado.pid << "\n";
    std::ofstream(sesion.ruta_inicio) << std::time(nullptr) << "\n";
    if (ajustes.replay_segundos != 0) {
        std::ofstream(sesion.ruta_replay) << ajustes.replay_segundos << "\n";
    } else {
        std::filesystem::remove(sesion.ruta_replay, ec);
    }
    // En replay la salida es una carpeta y cada volcado se cierra solo, y
    // emitiendo no hay fichero: en los dos casos no hay nada que reparar.
    if (ajustes.replay_segundos == 0 && !es_emision(ajustes.salida)) {
        std::ofstream(sesion.ruta_salida) << ajustes.salida << "\n";
    } else {
        std::filesystem::remove(sesion.ruta_salida, ec);
    }
    if (es_emision(ajustes.salida)) {
        // Solo el servidor, cortando por la ultima barra: lo que va detras es la
        // clave y no se escribe en ningun fichero.
        const auto barra = ajustes.salida.find_last_of('/');
        std::ofstream(sesion.ruta_emision)
            << (barra == std::string::npos ? ajustes.salida : ajustes.salida.substr(0, barra))
            << "\n";
    } else {
        std::filesystem::remove(sesion.ruta_emision, ec);
    }
    if (!ajustes.carpeta_guardado.empty()) {
        std::ofstream(sesion.ruta_guardado) << ajustes.carpeta_guardado << "\n";
    } else {
        std::filesystem::remove(sesion.ruta_guardado, ec);
    }

    // Esperar a que el socket escuche. Si GSR muere antes, el log dice por que.
    for (int esperado = 0; esperado < kEsperaSocketMs; esperado += kPasoEsperaMs) {
        if (grabacion_en_marcha(sesion)) {
            // -ro solo abre la puerta; el fichero no empieza hasta que se pide.
            // Si no se puede pedir, se para todo y se dice: quien marco
            // «guardar la emision» no puede acabar sin fichero y enterarse al
            // final, que es cuando ya no tiene arreglo.
            if (!ajustes.carpeta_guardado.empty() && !empezar_guardado(sesion, r.motivo)) {
                parar_grabacion(sesion);
                return r;
            }
            r.en_marcha = true;
            r.pid = lanzado.pid;
            return r;
        }
        if (!proceso_vivo(lanzado.pid)) {
            r.motivo = "el grabador murio al arrancar; " + diagnostico_de_log(sesion.ruta_log);
            std::filesystem::remove(sesion.ruta_pid, ec);
            return r;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPasoEsperaMs));
    }

    r.motivo = "el grabador no abrio su socket IPC en " + std::to_string(kEsperaSocketMs / 1000) +
               " s; sigue vivo (pid " + std::to_string(lanzado.pid) +
               "). Si era el portal, puede estar esperando a que aceptes el dialogo";
    return r;
}

namespace {

// ¿Dice el log que el driver conduce el codificador HEVC a ojo? Esa frase la
// escribe ffmpeg desde dentro del grabador cuando el driver no declara de que
// es capaz, y ahi el resultado es peor que h264: medido, entre un 18 % y un
// 37 % mas grande y menos fiel.
bool anota_hevc_poco_fiable(const std::string& ruta_log) {
    std::ifstream f(ruta_log);
    if (!f) return false;
    std::string linea;
    bool hevc = false;
    bool a_ojo = false;
    while (std::getline(f, linea)) {
        if (linea.find("hevc") != std::string::npos) hevc = true;
        if (linea.find("does not advertise encoder features") != std::string::npos) a_ojo = true;
    }
    return hevc && a_ojo;
}

}  // namespace

ResultadoParada parar_grabacion(const SesionGrabacion& sesion) {
    ResultadoParada r;

    std::string motivo;
    auto conexion = ConexionIpc::conectar(sesion.ruta_socket, &motivo);
    if (!conexion) {
        r.motivo = "no hay ninguna grabacion en marcha";
        // Si quedo un pid apuntado y el proceso existe, decirlo: socket caido
        // con grabador vivo es un estado que el usuario debe conocer.
        const std::string pid_texto = leer_pid(sesion.ruta_pid);
        if (!pid_texto.empty() && proceso_vivo(std::atol(pid_texto.c_str()))) {
            r.motivo = "el socket IPC no responde pero el grabador (pid " + pid_texto +
                       ") sigue vivo. Algo va mal: mira " + sesion.ruta_log;
        }
        return r;
    }

    // Si la emision se estaba guardando, primero se cierra ESE fichero. Su
    // respuesta trae la ruta y la del stop de una emision viene vacia, porque
    // una emision no deja fichero: sin esto, el usuario no sabria donde quedo.
    std::string guardado;
    std::error_code ec_guardado;
    if (std::filesystem::exists(sesion.ruta_guardado, ec_guardado)) {
        const auto rg = conexion->pedir("stop-replay-recording", -1, &motivo);
        if (rg && rg->ok && rg->tiene_data) guardado = rg->data;
    }

    // Sin limite de tiempo: la respuesta llega con el fichero ya escrito.
    const auto respuesta = conexion->pedir("stop", -1, &motivo);
    if (!respuesta) {
        r.motivo = "no se pudo hablar con el grabador: " + motivo;
        return r;
    }
    if (!respuesta->ok) {
        r.motivo = traducir_error_ipc(respuesta->data);
        return r;
    }

    r.parado = true;
    // Guardando una emision manda la ruta del fichero, no lo que responde el
    // stop: ahi lo que viene es la URL del servidor, que no es ningun fichero y
    // ademas lleva la clave pegada detras.
    r.ruta_fichero = !guardado.empty()            ? guardado
                     : respuesta->tiene_data      ? respuesta->data
                                                  : "";

    std::error_code ec;
    std::filesystem::remove(sesion.ruta_pid, ec);
    std::filesystem::remove(sesion.ruta_inicio, ec);
    std::filesystem::remove(sesion.ruta_replay, ec);
    std::filesystem::remove(sesion.ruta_salida, ec);
    std::filesystem::remove(sesion.ruta_emision, ec);
    std::filesystem::remove(sesion.ruta_guardado, ec);

    // Lo que esta maquina acaba de demostrar sobre su codificador HEVC. El
    // grabador lo escribe en su log y es la unica fuente honesta: nadie puede
    // saber de antemano si un driver declara sus capacidades.
    if (anota_hevc_poco_fiable(sesion.ruta_log)) recordar_hevc_poco_fiable(true);
    return r;
}

std::string grabacion_a_medias(const SesionGrabacion& sesion) {
    std::ifstream f(sesion.ruta_salida);
    std::string ruta;
    if (!std::getline(f, ruta) || ruta.empty()) return {};
    // Con el socket vivo la grabacion sigue: no es que quedara a medias.
    if (grabacion_en_marcha(sesion)) return {};

    std::error_code ec;
    if (!std::filesystem::exists(ruta, ec) || std::filesystem::file_size(ruta, ec) == 0) {
        return {};
    }
    // Si ya se abre bien, no hay nada que ofrecer. Es el caso de mp4, que GSR
    // escribe fragmentado y sobrevive sin ayuda.
    if (duracion_de(ruta) > 0.0) return {};
    return ruta;
}

namespace {

// El codec de video de un fichero, tal como lo nombra ffprobe.
std::string codec_de(const std::string& ruta) {
    const auto r = ejecutar("ffprobe", {"-v", "error", "-select_streams", "v:0",
                                        "-show_entries", "stream=codec_name",
                                        "-of", "csv=p=0", ruta});
    if (!r.ejecutado || r.codigo != 0) return {};
    std::string codec = r.salida;
    while (!codec.empty() && (codec.back() == '\n' || codec.back() == '\r')) codec.pop_back();
    return codec;
}

// El formato de pixel, para reconocer 10 bits y HDR sin adivinar por el nombre
// del codec: «hevc» a secas puede ser de 8 o de 10 bits.
std::string formato_pixel_de(const std::string& ruta) {
    const auto r = ejecutar("ffprobe", {"-v", "error", "-select_streams", "v:0",
                                        "-show_entries", "stream=pix_fmt",
                                        "-of", "csv=p=0", ruta});
    if (!r.ejecutado || r.codigo != 0) return {};
    std::string fmt = r.salida;
    while (!fmt.empty() && (fmt.back() == '\n' || fmt.back() == '\r')) fmt.pop_back();
    return fmt;
}

// El codificador de software de la familia de ese codec. Vacio si no hay uno
// que merezca la pena.
//
// vp8 y vp9 NO estan, y no es un olvido: medido el 2026-09-21, recomprimir una
// grabacion vp8 con libvpx-vp9 costo QUINCE veces la duracion del video para
// dejarla en el 93 %. Ofrecer eso seria una trampa.
// av1 tampoco, hasta medir SVT-AV1.
std::string codificador_para(const std::string& codec) {
    if (codec == "h264") return "libx264";
    if (codec == "hevc") return "libx265";
    return {};
}

}  // namespace

bool se_puede_reducir(const std::string& ruta, std::string& motivo) {
    std::error_code ec;
    if (!std::filesystem::exists(ruta, ec)) {
        motivo = "ese fichero no existe";
        return false;
    }
    const std::string codec = codec_de(ruta);
    if (codec.empty()) {
        motivo = "no tiene video: una grabacion de solo audio ya esta comprimida";
        return false;
    }
    const std::string fmt = formato_pixel_de(ruta);
    // 10 bits y HDR se quedan como estan: recomprimirlos por el camino normal
    // destruiria justo lo que alguien fue a buscar al elegirlos.
    if (fmt.find("10") != std::string::npos || fmt.find("12") != std::string::npos) {
        motivo = "esta grabado a mas de 8 bits y recomprimirlo perderia lo que lo hace especial";
        return false;
    }
    if (codificador_para(codec).empty()) {
        motivo = "no hay forma razonable de recomprimir " + codec + " en esta maquina";
        return false;
    }
    if (!localizar("ffmpeg")) {
        motivo = "hace falta ffmpeg";
        return false;
    }
    return true;
}

ResultadoReduccion reducir_grabacion(const std::string& ruta, NivelReduccion nivel) {
    ResultadoReduccion r;
    if (!se_puede_reducir(ruta, r.motivo)) return r;

    std::error_code ec;
    r.bytes_antes = std::filesystem::file_size(ruta, ec);
    const std::string codec = codec_de(ruta);
    const std::string enc = codificador_para(codec);
    // Los dos niveles salen de medir, no de elegir un numero redondo: 23 deja
    // el fichero practicamente indistinguible y 28 baja bastante mas con una
    // perdida pequeña pero real (docs/post-proceso.md).
    const std::string crf = nivel == NivelReduccion::Maximo ? "28" : "23";

    const std::string ext = extension_de(ruta);
    const std::string temporal =
        ruta.substr(0, ruta.size() - (ext.empty() ? 0 : ext.size() + 1)) +
        ".reduciendo." + (ext.empty() ? std::string("mkv") : ext);
    std::filesystem::remove(temporal, ec);

    // -fps_mode passthrough: la grabacion tiene fotogramas a ritmo VARIABLE
    // porque GSR no codifica los repetidos, y eso es media ventaja del
    // programa. Comprobado que ffmpeg los conserva igualmente, pero pedirlo no
    // cuesta nada y protege de que un dia deje de hacerlo.
    // -c:a copy: el audio ya esta comprimido y recodificarlo solo perderia.
    // El limite de ejecutar() son cinco segundos por defecto, pensados para las
    // sondas de --check, y aqui matarian la recompresion de cualquier grabacion
    // de mas de diez segundos: medido, eso fue lo primero que paso. El margen se
    // calcula sobre la duracion del video, que es de lo que depende el trabajo:
    // diez veces, con un suelo de un minuto. Sigue habiendo tope, porque un
    // ffmpeg colgado no puede dejar el programa esperando para siempre.
    const double segundos = duracion_de(ruta);
    const int limite_ms =
        static_cast<int>(std::max(60.0, segundos * 10.0) * 1000.0);
    const auto res = ejecutar("ffmpeg", {"-v", "error", "-y", "-i", ruta,
                                         "-c:v", enc, "-crf", crf, "-preset", "medium",
                                         "-fps_mode", "passthrough", "-c:a", "copy",
                                         temporal},
                              limite_ms);
    if (!res.ejecutado || res.codigo != 0) {
        r.motivo = res.expirado ? "la reduccion tardo demasiado y se corto"
                                : "no se pudo reducir: " + res.motivo;
        std::filesystem::remove(temporal, ec);
        return r;
    }
    // Igual que al reparar: lo que decide no es el codigo de salida, es que lo
    // que sale tenga duracion.
    if (duracion_de(temporal) <= 0.0) {
        r.motivo = "lo reducido no se puede leer; se deja la grabacion como estaba";
        std::filesystem::remove(temporal, ec);
        return r;
    }
    const auto despues = std::filesystem::file_size(temporal, ec);
    // Si no encoge, no se toca. Pasa con grabaciones ya muy comprimidas, y
    // sustituir un fichero por otro igual o mayor solo añade una recompresion
    // que nadie gana.
    if (despues >= r.bytes_antes) {
        r.motivo = "no encoge: esta grabacion ya estaba bien comprimida";
        std::filesystem::remove(temporal, ec);
        return r;
    }

    std::filesystem::rename(temporal, ruta, ec);
    if (ec) {
        r.motivo = "no se pudo sustituir el fichero: " + ec.message();
        std::filesystem::remove(temporal, ec);
        return r;
    }
    r.bytes_despues = despues;
    r.hecho = true;
    // La maquina se mide a si misma: se recuerda en que porcentaje quedo, para
    // que la proxima vez la interfaz pueda decir lo que pasa AQUI en vez de una
    // cifra medida en otro equipo.
    recordar_reduccion(nivel == NivelReduccion::Maximo,
                       static_cast<int>((despues * 100) / r.bytes_antes));
    return r;
}

double segundos_de(std::string_view texto) { return texto_a_double(texto); }

long espacio_libre_mb(const std::string& ruta) {
    std::error_code ec;
    auto carpeta = std::filesystem::path(ruta);
    if (!std::filesystem::is_directory(carpeta, ec)) carpeta = carpeta.parent_path();
    if (carpeta.empty()) return -1;
    const auto info = std::filesystem::space(carpeta, ec);
    if (ec) return -1;
    return static_cast<long>(info.available / (1024 * 1024));
}

std::string descartar_grabacion(const SesionGrabacion& sesion, std::string& motivo) {
    // Se para por el camino de siempre: hay que dejar el fichero cerrado antes
    // de borrarlo, y ademas es lo que limpia la sesion.
    const auto r = parar_grabacion(sesion);
    if (!r.parado) {
        motivo = r.motivo;
        return {};
    }
    if (r.ruta_fichero.empty()) {
        motivo = "no habia ningun fichero que descartar";
        return {};
    }
    std::error_code ec;
    if (!std::filesystem::remove(r.ruta_fichero, ec)) {
        motivo = "no se pudo borrar " + r.ruta_fichero +
                 (ec ? ": " + ec.message() : std::string());
        return {};
    }
    return r.ruta_fichero;
}

namespace {

// Graba unos segundos con ese codec a un temporal y devuelve los bytes por
// segundo, o -1 si no se pudo. Deja el log de GSR intacto para quien quiera
// leerlo despues.
double sondear_codec(const std::string& fuente, const std::string& codec,
                     const SesionGrabacion& sesion, const std::string& destino) {
    AjustesGrabacion a;
    a.fuente = fuente;
    a.salida = destino;
    a.codec_video = codec;
    a.sin_audio = true;
    a.audios.clear();
    // Pocos fotogramas y poca resolucion no: se sondea como se graba, que es de
    // lo que va la pregunta. Lo unico que se recorta es la duracion.
    std::error_code ec;
    std::filesystem::remove(destino, ec);
    const auto arranque = empezar_grabacion(a, sesion);
    if (!arranque.en_marcha) return -1.0;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    const auto parada = parar_grabacion(sesion);
    if (!parada.parado) return -1.0;
    const double dur = duracion_de(destino);
    const auto tam = std::filesystem::file_size(destino, ec);
    if (ec || dur <= 0.0) return -1.0;
    return static_cast<double>(tam) / dur;
}

}  // namespace

VeredictoCodecs comprobar_codecs(const std::string& fuente, const SesionGrabacion& sesion,
                                 const std::vector<std::string>& codecs_disponibles) {
    VeredictoCodecs v;
    if (grabacion_en_marcha(sesion)) {
        v.motivo = "hay una grabacion en marcha: esta prueba graba, asi que espera a que acabe";
        return v;
    }
    const bool hay_h264 = std::find(codecs_disponibles.begin(), codecs_disponibles.end(),
                                    "h264") != codecs_disponibles.end();
    v.hevc_ofrecido = std::find(codecs_disponibles.begin(), codecs_disponibles.end(),
                                "hevc") != codecs_disponibles.end();
    if (!hay_h264) {
        v.motivo = "esta maquina no ofrece h264, que es contra lo que se compara";
        return v;
    }

    std::error_code ec;
    const std::string base = sesion.dir + "/sonda";
    v.bytes_por_s_h264 = sondear_codec(fuente, "h264", sesion, base + "-h264.mkv");
    if (v.bytes_por_s_h264 < 0.0) {
        v.motivo = "no se pudo grabar la prueba con h264";
        std::filesystem::remove(base + "-h264.mkv", ec);
        return v;
    }
    if (v.hevc_ofrecido) {
        v.bytes_por_s_hevc = sondear_codec(fuente, "hevc", sesion, base + "-hevc.mkv");
        // El aviso del driver se lee del log de ESTA sonda, que es la ultima
        // que escribio. Es la señal decisiva: lo demas son indicios.
        v.hevc_a_ojo = anota_hevc_poco_fiable(sesion.ruta_log);
        std::filesystem::remove(base + "-hevc.mkv", ec);
    }
    std::filesystem::remove(base + "-h264.mkv", ec);
    // Lo que se aprende se recuerda, igual que cuando se descubre grabando.
    if (v.hevc_ofrecido && (v.hevc_a_ojo || (v.bytes_por_s_hevc > v.bytes_por_s_h264 * 1.05))) {
        recordar_hevc_poco_fiable(true);
    } else if (v.hevc_ofrecido && v.bytes_por_s_hevc > 0.0) {
        recordar_hevc_poco_fiable(false);
    }
    v.probado = true;
    return v;
}

bool recortar_grabacion(const std::string& ruta, double quitar_del_principio,
                        double quitar_del_final, std::string& motivo) {
    if (quitar_del_principio < 0.0 || quitar_del_final < 0.0) {
        motivo = "los segundos que quitar no pueden ser negativos";
        return false;
    }
    if (quitar_del_principio == 0.0 && quitar_del_final == 0.0) return true;

    std::error_code ec;
    if (!std::filesystem::exists(ruta, ec)) {
        motivo = "ese fichero no existe";
        return false;
    }
    const double duracion = duracion_de(ruta);
    if (duracion <= 0.0) {
        motivo = "no se puede leer la duracion de esa grabacion";
        return false;
    }
    // Se exige que quede algo, y con margen: recortar hasta dejar dos segundos
    // es tirar la grabacion con pasos extra.
    const double restante = duracion - quitar_del_principio - quitar_del_final;
    if (restante < 2.0) {
        motivo = "recortando eso no quedaria grabacion: dura " +
                 std::to_string(static_cast<int>(duracion)) + " s";
        return false;
    }
    if (!localizar("ffmpeg")) {
        motivo = "hace falta ffmpeg";
        return false;
    }

    const std::string ext = extension_de(ruta);
    const std::string temporal =
        ruta.substr(0, ruta.size() - (ext.empty() ? 0 : ext.size() + 1)) +
        ".recortando." + (ext.empty() ? std::string("mkv") : ext);
    std::filesystem::remove(temporal, ec);

    // -ss DESPUES de -i, y esto costo medirlo: con -ss delante, que es lo que
    // recomienda todo el mundo por rapido, ffmpeg salta pero NO reajusta las
    // marcas de tiempo de estos mkv, asi que el hueco del principio sigue
    // contando y el fichero «recortado» dura lo mismo. Medido: de 7,97 s
    // quitando 2 y 2 salian 5,90 en vez de 3,97. Detras de -i sale 3,907.
    //
    // Lo que cuesta es leer y tirar la parte saltada, que sin descodificar es
    // barato y ademas solo afecta a lo que se quita, no al fichero entero.
    std::vector<std::string> args{"-v", "error", "-y", "-i", ruta};
    if (quitar_del_principio > 0.0) {
        args.push_back("-ss");
        args.push_back(double_a_texto(quitar_del_principio));
    }
    if (quitar_del_final > 0.0) {
        args.push_back("-t");
        args.push_back(double_a_texto(restante));
    }
    args.insert(args.end(), {"-c", "copy", "-map", "0", temporal});

    const int limite_ms = static_cast<int>(std::max(60.0, duracion * 2.0) * 1000.0);
    const auto res = ejecutar("ffmpeg", args, limite_ms);
    if (!res.ejecutado || res.codigo != 0) {
        motivo = res.expirado ? "el recorte tardo demasiado y se corto"
                              : "no se pudo recortar: " + res.motivo;
        std::filesystem::remove(temporal, ec);
        return false;
    }
    // Lo de siempre: lo que decide no es el codigo de salida, es que lo que
    // sale tenga duracion.
    if (duracion_de(temporal) <= 0.0) {
        motivo = "lo recortado no se puede leer; se deja la grabacion como estaba";
        std::filesystem::remove(temporal, ec);
        return false;
    }
    std::filesystem::rename(temporal, ruta, ec);
    if (ec) {
        motivo = "no se pudo sustituir el fichero: " + ec.message();
        std::filesystem::remove(temporal, ec);
        return false;
    }
    return true;
}

bool reparar_grabacion(const std::string& ruta, std::string& motivo) {
    // El temporal CONSERVA la extension: ffmpeg deduce el formato de salida de
    // ella, y un «.mkv.reparando» le hace decir que no sabe que escribir. Se
    // descubrio con el fichero delante, no leyendo el manual.
    const std::string ext = extension_de(ruta);
    const std::string temporal =
        ruta.substr(0, ruta.size() - (ext.empty() ? 0 : ext.size() + 1)) +
        ".reparando." + (ext.empty() ? std::string("mkv") : ext);
    std::error_code ec;
    std::filesystem::remove(temporal, ec);

    // -c copy: se copian los flujos tal cual. No se recodifica nada, asi que no
    // se pierde calidad y tarda lo que tarde en leer el fichero.
    const auto r = ejecutar("ffmpeg", {"-v", "error", "-y", "-i", ruta,
                                       "-c", "copy", temporal});
    if (!r.ejecutado) {
        motivo = "hace falta ffmpeg para rehacer el fichero: " + r.motivo;
        std::filesystem::remove(temporal, ec);
        return false;
    }
    // El codigo de salida de ffmpeg no basta: leer un fichero truncado siempre
    // da error aunque el resultado sea bueno. Lo que decide es si lo que sale
    // tiene duracion.
    if (duracion_de(temporal) <= 0.0) {
        motivo = "no se pudo rehacer el fichero: lo que hay dentro no se puede leer";
        std::filesystem::remove(temporal, ec);
        return false;
    }

    std::filesystem::rename(temporal, ruta, ec);
    if (ec) {
        motivo = "no se pudo sustituir el fichero: " + ec.message();
        return false;
    }
    return true;
}

ResultadoParada guardar_replay(const SesionGrabacion& sesion) {
    ResultadoParada r;

    std::string motivo;
    auto conexion = ConexionIpc::conectar(sesion.ruta_socket, &motivo);
    if (!conexion) {
        r.motivo = "no hay ninguna grabacion en marcha";
        return r;
    }

    // Sin data: los dos campos que admite («seconds» y «restart-replay») son
    // opcionales y sus defaults son los que queremos —volcar el buffer entero y
    // respetar lo que diga -restart-replay-on-save—. Sin limite de tiempo: la
    // respuesta llega con el fichero ya escrito.
    const auto respuesta = conexion->pedir("save-replay", -1, &motivo);
    if (!respuesta) {
        r.motivo = "no se pudo hablar con el grabador: " + motivo;
        return r;
    }
    if (!respuesta->ok) {
        r.motivo = traducir_error_ipc(respuesta->data);
        return r;
    }

    r.parado = true;  // aqui significa «hecho», no «terminado»: el replay sigue
    r.ruta_fichero = respuesta->tiene_data ? respuesta->data : "";
    return r;
}

bool grabacion_en_marcha(const SesionGrabacion& sesion) {
    return ConexionIpc::conectar(sesion.ruta_socket).has_value();
}

bool poner_pausa(const SesionGrabacion& sesion, bool pausada, std::string& motivo) {
    auto conexion = ConexionIpc::conectar(sesion.ruta_socket, &motivo);
    if (!conexion) {
        motivo = "no hay ninguna grabacion en marcha";
        return false;
    }
    const auto respuesta = conexion->pedir("set-paused", pausada, kLimiteInmediatoMs, &motivo);
    if (!respuesta) {
        motivo = "no se pudo hablar con el grabador: " + motivo;
        return false;
    }
    if (!respuesta->ok) {
        motivo = traducir_error_ipc(respuesta->data);
        return false;
    }
    return true;
}

}  // namespace esr
