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

}  // namespace esr
