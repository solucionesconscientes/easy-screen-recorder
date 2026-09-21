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

// En que porcentaje quedo la ultima grabacion reducida en esta maquina, o 0 si
// todavia no se ha reducido ninguna.
//
// Existe porque la cifra no se puede escribir en la interfaz: cuanto encoge una
// grabacion depende de lo bueno que fuera el codificador de esa tarjeta, y eso
// cambia de un equipo a otro. Prometer «la mitad» seria vender una medicion
// hecha en otra maquina. Asi que cada equipo guarda lo suyo y la interfaz cuenta
// lo que ha pasado aqui, no lo que pasa en el portatil de quien programo esto.
// El nivel importa: «al maximo» deja el fichero bastante mas pequeño que
// «reducir», asi que una sola cifra para los dos enseñaria la del ultimo nivel
// usado al elegir el otro. Una cifra que no corresponde es peor que ninguna.
int reduccion_recordada(bool maximo);
bool recordar_reduccion(bool maximo, int porcentaje);

// ¿Ha demostrado esta maquina que su codificador HEVC no es de fiar?
//
// El grabador lo dice en su log cuando el driver no declara de que es capaz su
// codificador y ffmpeg tiene que conducirlo a ojo («Driver does not advertise
// encoder features»). Medido el 2026-09-21 en la maquina de desarrollo: con ese
// aviso, hevc salio entre un 18 % y un 37 % MAS grande que h264 y ademas menos
// fiel, en los cuatro niveles de calidad (docs/post-proceso.md).
//
// No es una lista de tarjetas escrita a mano, que seria justo lo que el proyecto
// prohibe: es el propio equipo diciendo lo que le pasa. En una tarjeta que si
// declare sus capacidades, esto nunca se enciende.
bool hevc_poco_fiable();
bool recordar_hevc_poco_fiable(bool si);

// Cuantos MB por minuto ocupa lo que graba esta maquina, o 0 si todavia no se
// ha grabado nada. Se apunta al guardar cada grabacion.
//
// Sirve para lo unico que no se puede saber de antemano: cuanta RAM va a costar
// un buffer de repeticion. Quince minutos de escritorio quieto son unos 36 MB;
// quince minutos de juego, medio giga. La diferencia la marca lo que grabes tu,
// no una tabla escrita aqui.
int mb_por_minuto_recordado();
bool recordar_mb_por_minuto(int mb);

}  // namespace esr
