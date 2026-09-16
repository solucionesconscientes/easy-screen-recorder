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
    // Los segundos de buffer, si esta grabacion es de replay. Existe por lo
    // mismo y por algo mas grave: el socket no dice de que modo es, asi que una
    // UI que se abriera a mitad ofreceria «Parar y guardar» sobre un replay, y
    // eso no guarda nada. Tirarias el buffer creyendo que lo salvabas.
    std::string ruta_replay;  // dir/replay.txt
    // La ruta del fichero que se esta escribiendo. Existe para poder repararlo
    // si el equipo se apaga: sin esto no se sabria ni que fichero mirar.
    std::string ruta_salida;  // dir/salida.txt
};

// Los segundos de buffer de la grabacion en marcha, o 0 si no es de replay.
int replay_en_marcha(const std::string& ruta_replay);

// La grabacion que quedo a medias, si la hay: devuelve su ruta, o vacio.
//
// «A medias» es que hay una salida apuntada y NO hay socket vivo: alguien apago
// el equipo, o el grabador murio. El fichero existe y trae el video, pero en
// mkv y webm le falta el cierre y un reproductor normal no lo abre.
//
// Devuelve vacio tambien cuando el fichero ya esta bien, para no ofrecer una
// reparacion que no hace falta.
std::string grabacion_a_medias(const SesionGrabacion& sesion);

// Rehace el contenedor de un fichero al que le falta el cierre.
//
// No recodifica: copia los flujos tal cual y escribe una cabecera completa, asi
// que no pierde calidad y tarda un segundo. Lo grabado que estuviera escrito se
// conserva; lo que quedo en el aire cuando se corto no existe y no hay magia
// que lo traiga.
//
// El original NO se borra hasta que el reparado existe y tiene duracion: si algo
// sale mal, es preferible quedarse con el fichero raro que con ninguno.
bool reparar_grabacion(const std::string& ruta, std::string& motivo);

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

// Vuelca los ultimos segundos del buffer de replay a un fichero.
//
// La respuesta es diferida y trae la ruta, igual que la de stop, asi que se
// espera SIN limite de tiempo (docs/gsr-ipc.md). Falla con «option -r is
// required» si la grabacion en marcha no es de replay.
ResultadoParada guardar_replay(const SesionGrabacion& sesion);

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
