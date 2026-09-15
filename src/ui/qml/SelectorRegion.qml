// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// El selector de region: una ventana a pantalla completa, oscurecida, donde
// se arrastra el recorte. Soltar elige; Esc cancela. La referencia es el
// selector de Spectacle: sin botones, sin dialogos.
import QtQuick

Window {
    id: selector

    signal elegida(string region)
    signal cancelada()

    flags: Qt.FramelessWindowHint
    color: "transparent"

    function abrir() {
        visibility = Window.FullScreen
        visible = true
        zona.desde = Qt.point(-1, -1)
        zona.hasta = Qt.point(-1, -1)
        zona.forceActiveFocus()
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.45
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height / 8
        text: qsTr("Arrastra para elegir la región. Esc cancela.")
        color: "white"
        font.pixelSize: 18
        visible: !zona.arrastrando
    }

    // El recorte en claro sobre el fondo oscurecido.
    Rectangle {
        id: marco
        visible: zona.arrastrando
        x: Math.min(zona.desde.x, zona.hasta.x)
        y: Math.min(zona.desde.y, zona.hasta.y)
        width: Math.abs(zona.hasta.x - zona.desde.x)
        height: Math.abs(zona.hasta.y - zona.desde.y)
        color: "transparent"
        border.color: "#da4453"
        border.width: 2

        Text {
            anchors.bottom: parent.top
            anchors.left: parent.left
            anchors.bottomMargin: 4
            color: "white"
            font.pixelSize: 14
            text: marco.width + "×" + marco.height
        }
    }

    MouseArea {
        id: zona
        anchors.fill: parent
        cursorShape: Qt.CrossCursor
        focus: true

        property point desde: Qt.point(-1, -1)
        property point hasta: Qt.point(-1, -1)
        readonly property bool arrastrando: desde.x >= 0

        onPressed: function(raton) {
            desde = Qt.point(raton.x, raton.y)
            hasta = desde
        }
        onPositionChanged: function(raton) {
            if (arrastrando) hasta = Qt.point(raton.x, raton.y)
        }
        onReleased: {
            if (!arrastrando) return
            var ancho = Math.abs(hasta.x - desde.x)
            var alto = Math.abs(hasta.y - desde.y)
            // Un clic suelto no es una region. Ocho pixeles es el minimo
            // para distinguir arrastre de temblor.
            if (ancho < 8 || alto < 8) {
                desde = Qt.point(-1, -1)
                return
            }
            // Coordenadas del escritorio combinado, que es lo que espera el
            // -region de GSR. En un solo monitor coinciden con las locales;
            // con varios, virtualX/virtualY desplazan al monitor de esta
            // ventana. Con escala fraccionaria esta SIN VERIFICAR (ESTADO.md).
            var x = Math.round(Math.min(desde.x, hasta.x) + selector.screen.virtualX)
            var y = Math.round(Math.min(desde.y, hasta.y) + selector.screen.virtualY)
            selector.visible = false
            selector.elegida(Math.round(ancho) + "x" + Math.round(alto) + "+" + x + "+" + y)
        }
        Keys.onEscapePressed: {
            selector.visible = false
            selector.cancelada()
        }
    }
}
