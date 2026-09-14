#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "capturia/version.hpp"

namespace capturia {

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

}  // namespace capturia
