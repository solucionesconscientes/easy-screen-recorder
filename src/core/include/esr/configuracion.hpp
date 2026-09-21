// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include <string>

namespace esr {

// La configuracion que se recuerda entre sesiones. Existe desde que la UI
// dejo elegir carpeta de destino: elegirla en cada arranque seria un castigo.
// Antes no existia a proposito (ESTADO.md, Tanda 4): un fichero que nadie
// escribia era codigo muerto.
//
// Formato: lineas «clave=valor» en ~/.config/easy-screen-recorder/easy-screen-recorder.conf. Sin
// secciones ni escapes: las claves son nuestras y los valores son rutas.

std::string ruta_configuracion();

std::map<std::string, std::string> leer_configuracion();

// Reescribe el fichero entero con el mapa dado. Devuelve false si no pudo.
bool escribir_configuracion(const std::map<std::string, std::string>& valores);

// Cambia una clave y guarda. Lo que usan la UI y el CLI.
bool guardar_ajuste(const std::string& clave, const std::string& valor);

// Las carpetas de destino que rigen de verdad: lo elegido por el usuario si
// existe, y si no el default XDG (Videos / Musica, con el home de ultimo
// recurso). Una carpeta elegida que ya no existe se ignora con honestidad:
// mejor caer al default que fallar cada grabacion.
std::string carpeta_videos_elegida();
std::string carpeta_audio_elegida();

// El servidor de ingesta de la ultima emision, para no tener que escribirlo
// cada vez. Vacio si no hay ninguno recordado.
//
// Se guarda el SERVIDOR y jamas la clave. La distincion no es cosmetica: la URL
// de ingesta de YouTube es publica y aparece en su propia documentacion, y la
// clave da permiso para emitir en tu canal. Este fichero es texto plano, asi que
// lo unico correcto es que la clave no pase por el.
std::string url_emision_recordada();
bool recordar_url_emision(const std::string& servidor);

// ¿Se enseña el tiempo de grabacion en la bandeja, en un segundo icono al lado
// del de la aplicacion? Apagado por defecto: es un item mas en el panel de
// alguien, y eso se pide, no se impone.
//
// Se recuerda entre sesiones porque es una preferencia y no una opcion de una
// grabacion: quien lo quiere, lo quiere siempre, y volver a marcarlo en cada
// arranque seria el mismo castigo que elegir la carpeta cada vez.
bool reloj_bandeja_activo();
bool recordar_reloj_bandeja(bool activo);

}  // namespace esr
