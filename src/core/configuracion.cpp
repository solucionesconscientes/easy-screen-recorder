// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/configuracion.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "esr/ajustes.hpp"

namespace esr {
namespace {

std::string dir_configuracion() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0') {
        return std::string(xdg) + "/easy-screen-recorder";
    }
    const char* hogar = std::getenv("HOME");
    const std::string casa = (hogar != nullptr && hogar[0] != '\0') ? hogar : ".";
    return casa + "/.config/easy-screen-recorder";
}

std::string carpeta_de(const std::string& clave, std::string por_defecto) {
    const auto conf = leer_configuracion();
    const auto elegida = conf.find(clave);
    if (elegida != conf.end() && !elegida->second.empty()) {
        std::error_code ec;
        if (std::filesystem::is_directory(elegida->second, ec)) return elegida->second;
        // Elegida pero desaparecida: se cae al default sin borrar el ajuste,
        // por si la carpeta vuelve (un disco externo, por ejemplo).
    }
    return por_defecto;
}

}  // namespace

std::string ruta_configuracion() { return dir_configuracion() + "/easy-screen-recorder.conf"; }

std::map<std::string, std::string> leer_configuracion() {
    std::map<std::string, std::string> valores;
    std::ifstream f(ruta_configuracion());
    std::string linea;
    while (std::getline(f, linea)) {
        if (linea.empty() || linea.front() == '#') continue;
        const auto corte = linea.find('=');
        if (corte == std::string::npos || corte == 0) continue;
        valores[linea.substr(0, corte)] = linea.substr(corte + 1);
    }
    return valores;
}

bool escribir_configuracion(const std::map<std::string, std::string>& valores) {
    std::error_code ec;
    std::filesystem::create_directories(dir_configuracion(), ec);
    std::ofstream f(ruta_configuracion(), std::ios::trunc);
    if (!f) return false;
    f << "# Configuracion de Easy Screen Recorder. La escribe la aplicacion.\n";
    for (const auto& [clave, valor] : valores) {
        f << clave << "=" << valor << "\n";
    }
    return f.good();
}

bool guardar_ajuste(const std::string& clave, const std::string& valor) {
    auto valores = leer_configuracion();
    valores[clave] = valor;
    return escribir_configuracion(valores);
}

std::string carpeta_videos_elegida() { return carpeta_de("carpeta_videos", carpeta_videos()); }

std::string carpeta_audio_elegida() { return carpeta_de("carpeta_audio", carpeta_musica()); }

std::string url_emision_recordada() {
    const auto valores = leer_configuracion();
    const auto i = valores.find("url_emision");
    return i == valores.end() ? std::string() : i->second;
}

bool recordar_url_emision(const std::string& servidor) {
    // Se niega a guardar algo que parezca una clave pegada detras. Es una
    // salvaguarda, no una comprobacion de formato: si alguien pega la URL
    // completa de YouTube con la clave incluida, aqui se corta antes de que
    // acabe en un fichero de texto plano.
    if (servidor.find("/live2/") != std::string::npos) return false;
    return guardar_ajuste("url_emision", servidor);
}

bool reloj_bandeja_activo() {
    const auto valores = leer_configuracion();
    const auto i = valores.find("reloj_bandeja");
    // Solo «1» enciende. Cualquier otra cosa, incluido un fichero editado a
    // mano con «true», deja la bandeja como estaba: un valor que no se entiende
    // no se interpreta a favor de añadir un icono que nadie pidio.
    return i != valores.end() && i->second == "1";
}

bool recordar_reloj_bandeja(bool activo) {
    return guardar_ajuste("reloj_bandeja", activo ? "1" : "0");
}

namespace {
std::string clave_reduccion(bool maximo) {
    return maximo ? "reduccion_ultima_maximo" : "reduccion_ultima_normal";
}
}  // namespace

int reduccion_recordada(bool maximo) {
    const auto valores = leer_configuracion();
    const auto i = valores.find(clave_reduccion(maximo));
    if (i == valores.end()) return 0;
    try {
        const int p = std::stoi(i->second);
        // Fuera de rango es un fichero tocado a mano: se ignora en vez de
        // enseñar un porcentaje imposible.
        return (p > 0 && p < 100) ? p : 0;
    } catch (const std::exception&) {
        return 0;
    }
}

bool recordar_reduccion(bool maximo, int porcentaje) {
    if (porcentaje <= 0 || porcentaje >= 100) return false;
    return guardar_ajuste(clave_reduccion(maximo), std::to_string(porcentaje));
}

bool hevc_poco_fiable() {
    const auto valores = leer_configuracion();
    const auto i = valores.find("hevc_poco_fiable");
    return i != valores.end() && i->second == "1";
}

bool recordar_hevc_poco_fiable(bool si) {
    // Solo se escribe cuando cambia: esto se comprueba al final de cada
    // grabacion y reescribir el fichero cada vez seria trabajo para nada.
    if (hevc_poco_fiable() == si) return true;
    return guardar_ajuste("hevc_poco_fiable", si ? "1" : "0");
}

int mb_por_minuto_recordado() {
    const auto valores = leer_configuracion();
    const auto i = valores.find("mb_por_minuto");
    if (i == valores.end()) return 0;
    try {
        const int v = std::stoi(i->second);
        return (v > 0 && v < 100000) ? v : 0;
    } catch (const std::exception&) {
        return 0;
    }
}

bool recordar_mb_por_minuto(int mb) {
    if (mb <= 0 || mb >= 100000) return false;
    return guardar_ajuste("mb_por_minuto", std::to_string(mb));
}

}  // namespace esr
