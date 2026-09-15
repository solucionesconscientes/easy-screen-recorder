#include "capturia/configuracion.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "capturia/ajustes.hpp"

namespace capturia {
namespace {

std::string dir_configuracion() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0') {
        return std::string(xdg) + "/capturia";
    }
    const char* hogar = std::getenv("HOME");
    const std::string casa = (hogar != nullptr && hogar[0] != '\0') ? hogar : ".";
    return casa + "/.config/capturia";
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

std::string ruta_configuracion() { return dir_configuracion() + "/capturia.conf"; }

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
    f << "# Configuracion de Capturia. La escribe la aplicacion.\n";
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

}  // namespace capturia
