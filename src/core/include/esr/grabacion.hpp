// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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
    // El servidor de la emision en marcha, SIN la clave. Existe por lo mismo
    // que ruta_replay: el socket no dice de que modo es la grabacion, y una
    // ventana abierta a mitad ofreceria cosas que ahi no valen.
    std::string ruta_emision;  // dir/emision.txt
    // La carpeta donde se esta guardando la emision, si se pidio. Existe para
    // que parar sepa que antes del stop hay que cerrar ESE fichero: su respuesta
    // trae la ruta, y la del stop de una emision viene vacia.
    std::string ruta_guardado;  // dir/guardado.txt
};

// El servidor de la emision en marcha, o vacio si esta grabacion no emite.
//
// Nunca trae la clave: lo que se guarda es la URL de ingesta, que es publica.
std::string emision_en_marcha(const std::string& ruta_emision);

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

// --- Reducir el tamaño de una grabacion ya hecha -------------------------
//
// Un codificador por HARDWARE esta hecho para ir rapido: tiene un presupuesto
// de tiempo por fotograma y hace lo que le da tiempo. Uno por software puede
// volver sobre los mismos fotogramas y encontrar las repeticiones que la GPU no
// tuvo tiempo de buscar. Por eso esto encoge el fichero sin que se note: no
// tira calidad, recupera el trabajo que la GPU no hizo.
//
// Cuanto encoge depende de lo bueno que fuera el codificador de esa tarjeta, y
// eso cambia de una maquina a otra. Por eso aqui no hay ninguna cifra prometida:
// se devuelve el antes y el despues y que cada maquina hable de lo suyo. Las
// mediciones que sustentan los niveles estan en docs/post-proceso.md.
enum class NivelReduccion {
    Normal,  // CRF 23: practicamente indistinguible
    Maximo,  // CRF 28: mas pequeño, con una perdida pequeña pero real
};

struct ResultadoReduccion {
    bool hecho = false;
    std::uintmax_t bytes_antes = 0;
    std::uintmax_t bytes_despues = 0;
    std::string motivo;  // por que no se hizo, cuando no se hizo
};

// Si este fichero se puede reducir, y si no, por que. La UI lo usa para no
// ofrecer lo que no va a poder hacer.
//
// Se niega con: solo audio, HDR o 10 bits (recomprimirlo destruiria justo lo
// que se fue a buscar) y los codecs cuyo codificador de software es
// impracticable. vp9 quedo fuera midiendo: 15 veces el tiempo del video para
// ahorrar un 7 %.
bool se_puede_reducir(const std::string& ruta, std::string& motivo);

// Recomprime el fichero EN SU SITIO, con el MISMO codec con el que se grabo.
//
// Mismo codec a proposito: cambiarlo traicionaria lo que pidio quien grabo. El
// que eligio webm quiere un webm, y el que grabo en h264 por compatibilidad no
// quiere descubrir que su fichero ya no lo abre el televisor.
//
// El original no se toca hasta que el nuevo existe y tiene duracion, igual que
// en reparar_grabacion.
ResultadoReduccion reducir_grabacion(const std::string& ruta, NivelReduccion nivel);

// --- Quitar el principio y el final --------------------------------------
//
// Todo el mundo graba unos segundos de «¿donde estaba el boton?» al empezar y
// otros tantos de buscar el raton al acabar.
//
// NO recodifica: corta copiando los flujos, asi que es instantaneo y no pierde
// calidad. El precio es la precision: copiando solo se puede cortar en un
// fotograma clave, y GSR pone uno cada dos segundos por defecto, asi que el
// corte cae donde caiga dentro de esos dos segundos. Recodificar daria el corte
// exacto y costaria lo que dure el video; para quitar la cola de un tutorial no
// compensa.
//
// Los segundos son los que se QUITAN de cada punta. Devuelve false y llena
// «motivo» si no se puede, y en ese caso el fichero se queda como estaba.
bool recortar_grabacion(const std::string& ruta, double quitar_del_principio,
                        double quitar_del_final, std::string& motivo);

// Espacio libre en la carpeta que contiene esa ruta, en MB. -1 si no se puede
// saber (una ruta que no existe, un sistema de ficheros que no lo dice).
//
// Quedarse sin espacio a mitad de grabacion es el peor fallo que puede tener un
// grabador: te enteras al final y lo grabado no sirve.
// Lee «1.5» como 1,5 den igual el idioma del sistema. std::stod y atof miran la
// configuracion regional y en español se paran en el punto: eso hizo que el
// recorte funcionara desde el CLI y se negara desde la ventana, que es la que
// pone el idioma al arrancar.
double segundos_de(std::string_view texto);

long espacio_libre_mb(const std::string& ruta);

// Para la grabacion y BORRA el fichero. Para cuando lo que se acaba de grabar
// no vale y no hay que guardarlo.
//
// Devuelve la ruta que se borro, o vacio si no habia nada o no se pudo. El
// borrado es irreversible y no hay papelera de por medio: quien llama a esto ya
// ha preguntado.
std::string descartar_grabacion(const SesionGrabacion& sesion, std::string& motivo);

// --- Comprobar de que es capaz esta tarjeta ------------------------------
//
// Que GSR liste «hevc» solo dice que el driver lo anuncia, no que lo haga bien.
// Medido el 2026-09-21 en una Intel HD 520: el driver no declaraba de que era
// capaz su codificador HEVC, ffmpeg lo conducia a ojo, y el resultado salia
// entre un 18 % y un 37 % MAS grande que h264 y ademas menos fiel.
//
// Hasta ahora eso se descubria despues, leyendo el log al parar. Esto lo
// averigua ANTES, grabando unos segundos con cada codec y comparando. Tarda
// unos diez segundos y graba a un temporal que se borra.
struct VeredictoCodecs {
    bool probado = false;
    bool hevc_ofrecido = false;    // la maquina dice tenerlo
    bool hevc_a_ojo = false;       // ...pero su driver no declara sus capacidades
    double bytes_por_s_h264 = 0.0;
    double bytes_por_s_hevc = 0.0;
    std::string motivo;            // por que no se pudo probar
};

// «fuente» es la misma que se le pasaria a una grabacion normal (un monitor).
// No graba audio: la prueba es del codificador de video y abrir el microfono
// para esto seria de mal gusto.
VeredictoCodecs comprobar_codecs(const std::string& fuente, const SesionGrabacion& sesion,
                                 const std::vector<std::string>& codecs_disponibles);

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
