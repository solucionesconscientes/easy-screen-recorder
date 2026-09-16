// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Donde va la camara y de que tamaño, arrastrandola.
//
// Sustituye a un desplegable de cuatro esquinas. Las esquinas se quedaban
// cortas en cuanto la pantalla tiene una barra de tareas, un panel lateral o
// una ventana fija en una punta: las cuatro opciones fallaban a la vez y no
// habia una quinta. GSR admite posicion libre en porcentaje, asi que aqui se
// coloca donde sea.
//
// Es un mapa, no una vista previa del video: el rectangulo grande es la
// pantalla y el pequeño es la camara, los dos a su proporcion real. Lo que se
// ve DENTRO de la camara lo pone VistaPreviaCamara, que es otra cosa.
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: raiz

    // Proporcion (ancho/alto) de la pantalla que se va a grabar y de la camara.
    property real proporcionPantalla: 16 / 9
    property real proporcionCamara: 16 / 9
    // Ancho de la camara en % del ancho del video. Lo manda el deslizador.
    property int anchoPct: 25
    // Lo que sale de aqui: la esquina superior izquierda, en % del video.
    property int xPct: 73
    property int yPct: 73

    implicitHeight: Kirigami.Units.gridUnit * 7

    // El rectangulo de la pantalla, centrado y a su proporcion.
    Rectangle {
        id: pantalla
        anchors.centerIn: parent
        height: Math.min(parent.height, parent.width / raiz.proporcionPantalla)
        width: height * raiz.proporcionPantalla
        // Un gris claro propio y un borde marcado. El fondo alterno del tema
        // casi no se distingue del panel, y un mapa que no se ve no es un mapa.
        color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                       Kirigami.Theme.textColor.b, 0.10)
        border.color: Kirigami.Theme.disabledTextColor
        border.width: 1
        radius: Kirigami.Units.smallSpacing

        // La camara. El alto sale del ancho y de su proporcion, igual que hara
        // GSR: a el solo se le pasa el ancho, para que no la deforme.
        Rectangle {
            id: recuadro
            width: pantalla.width * raiz.anchoPct / 100
            height: width / raiz.proporcionCamara
            x: pantalla.width * raiz.xPct / 100
            y: pantalla.height * raiz.yPct / 100
            color: Kirigami.Theme.highlightColor
            opacity: arrastre.active ? 0.85 : 0.6
            radius: Kirigami.Units.smallSpacing / 2

            Kirigami.Icon {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height) * 0.5
                height: width
                source: "camera-web-symbolic"
                color: Kirigami.Theme.highlightedTextColor
            }

            DragHandler {
                id: arrastre
                // Los limites se ponen aqui y no al soltar: asi el recuadro no
                // llega a salirse ni un momento, y lo que se ve es siempre lo
                // que se va a grabar.
                xAxis.minimum: 0
                xAxis.maximum: pantalla.width - recuadro.width
                yAxis.minimum: 0
                yAxis.maximum: pantalla.height - recuadro.height
                onActiveChanged: if (!active) raiz.apuntarPosicion()
                onTranslationChanged: if (active) raiz.apuntarPosicion()
            }
        }
    }

    // De pixeles del dibujo a porcentaje del video, que es lo que entiende GSR.
    function apuntarPosicion() {
        if (pantalla.width <= 0 || pantalla.height <= 0) return
        raiz.xPct = Math.round(recuadro.x / pantalla.width * 100)
        raiz.yPct = Math.round(recuadro.y / pantalla.height * 100)
    }

    // Si el tamaño crece hasta sacar la camara por un borde, se la trae de
    // vuelta en vez de dejar que GSR la recorte en silencio.
    onAnchoPctChanged: Qt.callLater(function() {
        if (pantalla.width <= 0) return
        var maxX = Math.round((1 - recuadro.width / pantalla.width) * 100)
        var maxY = Math.round((1 - recuadro.height / pantalla.height) * 100)
        if (raiz.xPct > maxX) raiz.xPct = Math.max(0, maxX)
        if (raiz.yPct > maxY) raiz.yPct = Math.max(0, maxY)
    })

    QQC2.Label {
        // Arriba y no abajo: la camara suele ir abajo a la derecha, que es
        // justo donde estaba el rotulo, y se pisaban.
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        text: qsTr("Arrástrala")
        opacity: 0.6
        font: Kirigami.Theme.smallFont
        visible: !arrastre.active
    }
}
