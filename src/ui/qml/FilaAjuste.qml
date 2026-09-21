// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Una fila de ajuste dentro de una tarjeta: a la izquierda el nombre y su
// explicacion, a la derecha el control.
//
// La explicacion va SIEMPRE a la vista, debajo del nombre. Antes vivia en un
// boton «i» que habia que descubrir y perseguir con el raton, y llego a haber
// diecinueve de esos iconos en la misma ventana: eran la textura mas visible
// de la pagina, o sea ruido puro.
//
// Existe porque los delegados de Kirigami Addons no dejan tocar el control que
// llevan dentro, y aqui hace falta: el desplegable del audio vuelve a preguntar
// que esta sonando al abrirse, y los dos campos de tiempo se escriben a mano.
// Para las casillas y los botones se usan los suyos, que encajan tal cual.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.AbstractFormDelegate {
    id: fila

    property string titulo: ""
    property string explicacion: ""
    // Lo que se ponga dentro va a la derecha, en fila. En columna cuando el
    // control necesita el ancho entero, como una direccion de servidor.
    property bool enColumna: false
    default property alias controles: hueco.data

    // Sin fondo propio: la fila no se pulsa, el control si. Con el fondo de
    // serie se iluminaba toda la fila al pasar por encima y parecia un boton.
    background: null
    focusPolicy: Qt.NoFocus
    Layout.fillWidth: true

    contentItem: GridLayout {
        columns: 2
        columnSpacing: Sistema.r13
        rowSpacing: Sistema.r3

        QQC2.Label {
            visible: text !== ""
            Layout.fillWidth: true
            text: fila.titulo
            wrapMode: Text.Wrap
            color: fila.enabled ? Kirigami.Theme.textColor
                                : Kirigami.Theme.disabledTextColor
        }
        // El control, a la derecha del nombre. En columna cuando necesita el
        // ancho entero, como una direccion de servidor.
        RowLayout {
            id: hueco
            spacing: Sistema.r8
            Layout.row: fila.enColumna ? 1 : 0
            Layout.column: fila.enColumna ? 0 : 1
            Layout.columnSpan: fila.enColumna ? 2 : 1
            Layout.fillWidth: fila.enColumna
            Layout.alignment: fila.enColumna ? Qt.AlignLeft
                                             : Qt.AlignRight | Qt.AlignVCenter
        }
        // La explicacion, SIEMPRE a lo ancho de la fila entera y debajo de
        // todo. Metida en la columna del nombre se quedaba en 250 px y salian
        // parrafos de cuatro lineas: la pagina medida crecia 400 px de puro
        // estrechamiento.
        QQC2.Label {
            visible: text !== ""
            Layout.columnSpan: 2
            Layout.fillWidth: true
            text: fila.explicacion
            wrapMode: Text.Wrap
            // El gris corregido lo pone la tarjeta, y esta fila lo hereda:
            // asi las filas nuestras y las de Kirigami salen iguales.
            color: Kirigami.Theme.disabledTextColor
        }
    }
}
