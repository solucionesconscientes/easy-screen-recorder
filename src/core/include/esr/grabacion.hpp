// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "esr/ajustes.hpp"

namespace esr {

// Donde vive la grabacion en marcha. Una sola a la vez: la carpeta es fija y
// el socket dentro de ella. Va bajo ~/.cache y no bajo /tmp a proposito: el
// /tmp de un GSR en flatpak es privado y ni el socket ni nada compartido
// puede ir alli (docs/gsr-ipc.md, "La trampa del flatpak").
struct SesionGrabacion {
    std::string dir;          // ~/.cache/easy-screen-recorder/sesion
    std::string ruta_socket;  // dir/ipc.sock
    std::string ruta_log;     // dir/gsr.log
    std::string ruta_pid;     // dir/gsr.pid
    // Epoch (segundos) de cuando arranco la grabacion. Existe para que una
    // UI que se abra a mitad enseñe el tiempo real y no un reloj a cero.
    std::string ruta_inicio;  // dir/inicio.txt
};

// Lee ese instante. 0 si no hay grabacion o no se puede leer.
long inicio_grabacion(const std::string& ruta_inicio);

SesionGrabacion sesion_por_defecto();

struct ResultadoLanzamiento {
    bool en_marcha = false;
    long pid = -1;
    std::string motivo;  // redactado para el usuario si !en_marcha
};

// Lanza GSR con los ajustes dados y espera a que su socket IPC responda.
// Si GSR muere antes de escuchar, el motivo trae el diagnostico de su log.
ResultadoLanzamiento empezar_grabacion(const AjustesGrabacion& ajustes,
                                       const SesionGrabacion& sesion);

struct ResultadoParada {
    bool parado = false;
    // La ruta que devuelve la respuesta diferida de stop. Puede venir vacia
    // con parado == true: un stop en modo replay no guarda nada
    // (docs/gsr-ipc.md, "Como llega la ruta").
    std::string ruta_fichero;
    std::string motivo;
};

// Pide stop y espera SIN limite de tiempo: la respuesta llega cuando el
// fichero esta escrito. Cortar antes tiraria la peticion.
ResultadoParada parar_grabacion(const SesionGrabacion& sesion);

// Un connect() que entra es la señal de "grabando", igual que el status de
// gsr-cli. No se manda nada.
bool grabacion_en_marcha(const SesionGrabacion& sesion);

// set-paused, no toggle-pause: el resultado no depende de lo que creamos
// que esta pasando (gsr-cli.1, COMMANDS).
bool poner_pausa(const SesionGrabacion& sesion, bool pausada, std::string& motivo);

// Traduce el final del log de GSR a un diagnostico en español. Si no
// reconoce el fallo, devuelve las ultimas lineas tal cual: un log crudo es
// mejor que un diagnostico inventado.
std::string diagnostico_de_log(const std::string& ruta_log);

}  // namespace esr
