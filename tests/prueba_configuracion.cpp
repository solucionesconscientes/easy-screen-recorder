// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "esr/configuracion.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "comprobar.hpp"

using namespace esr;

int main() {
    // Sandbox: la configuracion de verdad del usuario no se toca.
    const std::string sandbox = std::filesystem::temp_directory_path() / "easy-screen-recorder-prueba-conf";
    std::filesystem::remove_all(sandbox);
    setenv("XDG_CONFIG_HOME", sandbox.c_str(), 1);
    setenv("HOME", "/home/prueba-inexistente", 1);

    // Sin fichero: mapa vacio y defaults XDG (que aqui caen al home, porque
    // el HOME de prueba no tiene user-dirs.dirs).
    COMPROBAR(leer_configuracion().empty());
    COMPROBAR(carpeta_videos_elegida() == "/home/prueba-inexistente");
    COMPROBAR(carpeta_audio_elegida() == "/home/prueba-inexistente");

    // Guardar y releer: viaje de ida y vuelta.
    COMPROBAR(guardar_ajuste("carpeta_videos", "/una/que/no/existe"));
    COMPROBAR(leer_configuracion().at("carpeta_videos") == "/una/que/no/existe");

    // Una carpeta elegida que no existe se ignora: default, sin fallar.
    COMPROBAR(carpeta_videos_elegida() == "/home/prueba-inexistente");

    // Una que si existe, manda.
    const std::string real = sandbox + "/videos-del-usuario";
    std::filesystem::create_directories(real);
    COMPROBAR(guardar_ajuste("carpeta_videos", real));
    COMPROBAR(carpeta_videos_elegida() == real);
    // Y la de audio sigue sin verse afectada.
    COMPROBAR(carpeta_audio_elegida() == "/home/prueba-inexistente");

    // Dos claves conviven; los comentarios y la basura no rompen.
    COMPROBAR(guardar_ajuste("carpeta_audio", real));
    {
        std::ofstream f(ruta_configuracion(), std::ios::app);
        f << "# comentario\nlinea sin igual\n=sin_clave\n";
    }
    const auto conf = leer_configuracion();
    COMPROBAR(conf.size() == 2);
    COMPROBAR(conf.at("carpeta_audio") == real);

    // El reloj de la bandeja: apagado si no hay nada escrito, ida y vuelta, y
    // un valor que no se entiende no lo enciende.
    COMPROBAR(!reloj_bandeja_activo());
    COMPROBAR(recordar_reloj_bandeja(true));
    COMPROBAR(reloj_bandeja_activo());
    COMPROBAR(recordar_reloj_bandeja(false));
    COMPROBAR(!reloj_bandeja_activo());
    COMPROBAR(guardar_ajuste("reloj_bandeja", "true"));
    COMPROBAR(!reloj_bandeja_activo());

    // Lo que esta maquina ha medido al reducir. Sin nada escrito, 0; y un
    // porcentaje imposible se ignora en vez de enseñarse.
    COMPROBAR(reduccion_recordada(false) == 0);
    COMPROBAR(recordar_reduccion(false, 59));
    COMPROBAR(reduccion_recordada(false) == 59);
    // Cada nivel lleva la suya: «al maximo» encoge mas, y enseñar la del otro
    // seria una cifra que no corresponde.
    COMPROBAR(reduccion_recordada(true) == 0);
    COMPROBAR(recordar_reduccion(true, 34));
    COMPROBAR(reduccion_recordada(true) == 34);
    COMPROBAR(reduccion_recordada(false) == 59);
    COMPROBAR(!recordar_reduccion(false, 0));
    COMPROBAR(!recordar_reduccion(false, 140));
    COMPROBAR(reduccion_recordada(false) == 59);
    COMPROBAR(guardar_ajuste("reduccion_ultima_normal", "ochenta"));
    COMPROBAR(reduccion_recordada(false) == 0);

    std::filesystem::remove_all(sandbox);
    return prueba::resumen("prueba_configuracion");
}
