// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "esr/version.hpp"

namespace esr {

// Una entrada de una lista que GSR enumera: un monitor, un dispositivo de
// audio, una aplicacion.
//
// El numero de campos varia por linea, no por lista. GSR imprime desde una
// linea pelada («region», «portal») hasta tres campos
// («/dev/video0|640x480@30hz|mjpeg»). Por eso `campos` es un vector y no un
// segundo string: forzar todo a «id + detalle» perdia el tercer campo.
struct Opcion {
    std::string id;                   // lo que va antes del primer «|»
    std::vector<std::string> campos;  // lo que va detras, ya partido por «|»

    // Los campos unidos por un espacio, para enseñarselos al usuario. Vacio si
    // la linea solo traia el identificador.
    std::string detalle() const;
};

// Un bloque del volcado: un comando, su codigo de salida y su salida completa.
struct BloqueVolcado {
    std::string comando;
    std::string salida;
    int codigo = -1;
    bool tiene_codigo = false;
};

// Que se espera de cada lista. No es una preferencia nuestra: sale de leer que
// imprime cada comando en src/cli/commands.c de GSR. Sin esto el parser avisaba
// de cosas que son normales, y un aviso falso en --check gasta la confianza del
// usuario igual que un dato inventado.
struct FormatoLista {
    // Toda linea debe traer «|». Cierto para --list-audio-devices, que siempre
    // imprime «nombre|descripcion» (commands.c:281-289). Falso para
    // --list-capture-options, que mezcla lineas de uno, dos y tres campos.
    bool exige_separador = false;
    // Una lista vacia es un estado legitimo, no un fallo. Cierto para
    // --list-application-audio: si no hay ninguna aplicacion sonando no imprime
    // nada y sale con 0 (commands.c:302-316).
    bool vacio_normal = false;
};

// Lo que --info dice de esta maquina. El formato sale de info_command en
// src/cli/commands.c:238-274 de GSR: lineas «section=nombre» y dentro de cada
// seccion o bien «clave|valor» o bien nombres pelados.
struct InfoGsr {
    bool presente = false;  // hubo un bloque --info valido en el volcado
    std::string servidor_grafico;      // display_server: «wayland» o «x11»
    bool audio_por_aplicacion = false; // supports_app_audio|yes
    std::string vendedor_gpu;          // vendor, seccion gpu_info
    // Tal cual los nombra GSR («h264», «h264_software», «hevc», ...). No se
    // normalizan: la UI enseña lo que la maquina dice, no lo que esperamos.
    std::vector<std::string> codecs_video;
    std::vector<std::string> formatos_imagen;
};

// «h264_software» sale de --info solo porque existe libx264
// (src/cli/commands.c:75-76 de GSR): codifica en CPU. Tratarlo como los demas
// haria caer el default "mejor codec de hardware" en software sin avisar.
bool codec_es_hardware(std::string_view nombre);

// El codec de hardware que usara «-k auto» de GSR, o nullopt si no hay
// ninguno. Copia el orden del upstream (select_appropriate_video_codec
// _automatically, src/recorder/codec_select.c): h264, luego hevc, luego av1.
// Alli h264 solo se descarta por resolucion maxima, que aqui no se conoce.
std::optional<std::string> mejor_codec_hardware(const InfoGsr& info);

// Los codecs de video que se le pueden PEDIR a GSR con -k, de entre los que
// esta maquina dice soportar.
//
// No es lo mismo que `info.codecs_video`, y la diferencia costo una opcion rota
// en la interfaz: `--info` imprime «h264_software» solo porque existe libx264
// (commands.c:75-76 de GSR), pero ese nombre NO esta en la tabla de -k
// (args_parser.c:20-38). Medido: pedirlo mata al grabador al arrancar, con
// «-k should either be 'auto', 'h264', ...». La codificacion por CPU en GSR se
// pide con `-encoder cpu`, que es otra opcion y otra conversacion.
std::vector<std::string> codecs_video_ofrecibles(const InfoGsr& info);

struct Capacidades {
    bool gsr_respondio = false;
    bool gsr_cli_respondio = false;
    std::optional<Version> version_gsr;
    InfoGsr info;
    std::vector<Opcion> fuentes_captura;
    std::vector<Opcion> dispositivos_audio;
    std::vector<Opcion> audio_por_aplicacion;
    // Que no se pudo interpretar y por que. --check lo imprime tal cual: el
    // parser nunca inventa una capacidad, prefiere avisar de que no entendio.
    std::vector<std::string> avisos;
};

// Que clase de cosa es una fuente de captura. GSR las lista todas juntas y con
// su identificador interno («eDP-1», «/dev/video0», «portal»), que es correcto y
// no significa nada para quien solo quiere grabar su pantalla.
//
// Aqui se CLASIFICA, no se nombra: las palabras que ve el usuario se componen en
// la interfaz, que es quien tiene el traductor. Asi esta parte —la que tiene
// logica de verdad: agrupar, ordenar, desempatar— se puede probar sin Qt.
enum class TipoFuente {
    Monitor,
    Region,         // «region»: el usuario arrastra un recorte
    Portal,         // «portal»: lo pregunta el escritorio al empezar
    VentanaActiva,  // «focused»
    Camara,         // /dev/videoN
};

// De que tipo de conector cuelga un monitor. El prefijo del identificador lo
// dice, y la convencion no es nuestra: son los nombres que el kernel da a cada
// tipo de conector DRM, los mismos que salen en xrandr y en Preferencias del
// sistema. eDP, LVDS y DSI son el panel de un portatil o una tableta; los demas
// son un cable.
enum class FamiliaMonitor { Interna, Hdmi, DisplayPort, Vga, Dvi, Otra };

// Una fuente lista para enseñar, con todo lo que hace falta para escribir su
// nombre pero sin escribirlo.
struct FuenteAmable {
    std::string id;          // el identificador de GSR, intacto: es lo que viaja
    TipoFuente tipo = TipoFuente::Monitor;
    FamiliaMonitor familia = FamiliaMonitor::Otra;  // solo con tipo Monitor
    std::string resolucion;  // «1366x768» si GSR la dijo; vacio si no
    std::string nombre;      // el de la camara segun el kernel; vacio si no se sabe
    // Si hay otra fuente que se llamaria igual, y por tanto el identificador
    // tiene que aparecer para poder distinguirlas. Dos «Pantalla DisplayPort»
    // en un desplegable no se pueden elegir.
    bool desempatar = false;
};

// Ordena y clasifica lo que GSR lista.
//
// Hace tres cosas que la lista cruda no trae. Una, quita repetidos: una camara
// con ocho modos son ocho lineas y UNA fuente, porque el modo lo elige GSR. Dos,
// agrupa: monitores, luego lo que exige decidir al empezar, luego camaras; GSR
// las saca como le vienen y la camara caia entre «region» y «portal». Tres,
// marca cuales necesitan enseñar su identificador para no confundirse.
// ¿Hay un microfono de verdad en esta maquina?
//
// No basta con mirar si esta «default_input»: ese nombre se lo inventa GSR y
// aparece SIEMPRE, tambien en un equipo sin ninguna entrada. Y tampoco vale
// contar entradas a secas, porque la lista trae los «monitores», que son la
// salida de cada altavoz y no un microfono.
//
// Asi que cuenta lo que no es ni un nombre inventado ni un monitor. Con la lista
// de esta maquina (docs/gsr-capabilities.txt) queda
// «alsa_input.pci-0000_00_1f.3.analog-stereo», que es el microfono del portatil.
bool hay_microfono(const std::vector<Opcion>& dispositivos_audio);

std::vector<FuenteAmable> fuentes_amables(const std::vector<Opcion>& fuentes_captura);

// El nombre que el kernel le da a una camara V4L2, listo para enseñarselo a
// alguien. GSR lista «/dev/video0|1280x720@30hz|mjpeg»: la ruta no le dice nada
// a nadie y el modo lo elige el. El nombre de verdad esta en
// /sys/class/video4linux/videoN/name, que es donde lo pone el driver.
//
// Vacio si no se puede leer. Vacio significa «no se sabe», y quien llame tiene
// que enseñar otra cosa; nunca se inventa un nombre.
std::string nombre_camara(std::string_view ruta_dispositivo);

// La limpieza de ese nombre, aparte para poder probarla sin /sys delante.
//
// El driver escribe lo que declara el dispositivo y el kernel lo trunca a lo
// que cabe: esta maquina responde «Integrated_Webcam_HD: Integrate», con los
// espacios como guiones bajos y el nombre repetido a medias detras de los dos
// puntos. Se corta en los dos puntos y se deshacen los guiones bajos.
std::string limpiar_nombre_camara(std::string_view crudo);

// Marcadores del formato de volcado. Son NUESTROS, no de GSR: el envoltorio lo
// generamos nosotros (scripts/volcar-capacidades.sh y detectar()), asi que este
// nivel del parser es verificable. Lo que va dentro de cada bloque lo escribe
// GSR y ahi el parser es deliberadamente conservador.
inline constexpr std::string_view kMarcaComando = "### comando: ";
inline constexpr std::string_view kMarcaCodigo = "### codigo: ";
inline constexpr std::string_view kMarcaFin = "### fin";

std::vector<BloqueVolcado> partir_volcado(std::string_view volcado);

// Convierte la salida de un bloque en una lista de opciones.
//
// El separador «|» esta verificado contra el codigo de GSR 6.0.0 y contra el
// volcado de docs/gsr-capabilities.txt. Lo que no se da por hecho es cuantos
// campos trae cada linea: se parten todos y el que solo trae identificador se
// queda sin campos, sin aviso.
std::vector<Opcion> interpretar_lista(const BloqueVolcado& bloque,
                                      std::vector<std::string>& avisos,
                                      FormatoLista formato = {});

Capacidades interpretar_volcado(std::string_view volcado);

}  // namespace esr
