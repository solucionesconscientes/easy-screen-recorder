// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QIcon>
#include <QString>

#include <utility>

// El reloj de la bandeja: el tiempo de grabacion dibujado para el hueco de un
// icono de panel.
//
// Vive en su propio fichero y no dentro de main.cpp por una razon practica: asi
// un arnes puede compilar este .cpp, pedirle los iconos y mirarle los pixeles.
// Dentro de main.cpp, para probarlo habria que duplicar el dibujo, y una copia
// es una copia que se queda vieja.
namespace esr::ui {

// mm:ss, y h:mm:ss pasada la hora. Va en el tooltip, que es donde cabe entero.
QString tiempoEscrito(int segundos);

// Las DOS lineas del icono, arriba y abajo. Dos y no «01:23» de una porque el
// hueco de un icono en el panel son unos 22 px: cinco caracteres ahi salen a
// cuatro pixeles por cifra y no se leen. Partido, cada cifra se queda con la
// mitad del alto, que es la altura a la que el propio panel escribe su reloj.
//
// Pasada la hora los minutos ya no caben en dos cifras, asi que arriba pasan las
// horas con su «h» y abajo los minutos. Se pierde el segundero, que a esa altura
// ya no es lo que nadie mira.
std::pair<QString, QString> lineasReloj(int segundos);

// El icono, en los tamaños que puede pedir un panel.
QIcon iconoReloj(int segundos, bool pausado);

}  // namespace esr::ui
