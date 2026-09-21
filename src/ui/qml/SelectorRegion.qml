// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// El selector de region: una ventana a pantalla completa, oscurecida, donde
// se arrastra el recorte. Soltar NO elige: deja el recorte ajustable, se
// mueve por dentro y se estira por los bordes. Enter graba; Esc cancela. La
// referencia es el selector de Spectacle: sin botones, sin dialogos.
import QtQuick
import es.solucionesconscientes.esr

Window {
    id: selector

    signal elegida(string region)
    signal cancelada()

    // Un clic suelto no es una region. Ocho pixeles es el minimo para
    // distinguir arrastre de temblor.
    readonly property int minimo: 8
    // Lo cerca de un borde que hay que pulsar para arrastrar ESE borde en vez
    // de mover el recorte entero.
    readonly property int agarre: 12

    // El recorte, en coordenadas de esta ventana. Se guarda como rectangulo y
    // no como los dos puntos del arrastre porque ahora tambien se mueve y se
    // estira, y con dos puntos cada borde habria que adivinar cual es cual.
    property real recorteX: 0
    property real recorteY: 0
    property real recorteAncho: 0
    property real recorteAlto: 0
    readonly property bool hayRecorte: recorteAncho >= minimo && recorteAlto >= minimo

    flags: Qt.FramelessWindowHint
    color: "transparent"

    function abrir() {
        visibility = Window.FullScreen
        visible = true
        recorteAncho = 0
        recorteAlto = 0
        zona.accion = ""
        zona.forceActiveFocus()
    }

    // Enter confirma, y con eso arranca la grabacion. Antes confirmaba soltar
    // el raton, y eso daba un solo intento: un recorte torcido obligaba a
    // volver a abrir el selector.
    function confirmar() {
        if (!hayRecorte) return
        // Coordenadas del escritorio combinado, que es lo que espera el
        // -region de GSR. En un solo monitor coinciden con las locales;
        // con varios, virtualX/virtualY desplazan al monitor de esta
        // ventana. Con escala fraccionaria esta SIN VERIFICAR (ESTADO.md).
        var x = Math.round(recorteX + selector.screen.virtualX)
        var y = Math.round(recorteY + selector.screen.virtualY)
        selector.visible = false
        selector.elegida(Math.round(recorteAncho) + "x" + Math.round(recorteAlto)
                         + "+" + x + "+" + y)
    }

    function cancelar() {
        selector.visible = false
        selector.cancelada()
    }

    // El velo, en CUATRO trozos alrededor del recorte y no uno encima de todo.
    // Asi el recorte se ve tal cual, que es lo que se va a grabar; con el velo
    // encima habia que encuadrar a traves de un 45 % de negro, y el comentario
    // de aqui decia «en claro» una cosa que no era verdad.
    //
    // El alfa va en el color y no en `opacity`: cuatro trozos con opacidad de
    // grupo obligan a componer la pantalla entera aparte. Y se tocan sin
    // solaparse, porque solapados el negro se sumaria y la costura se veria.
    //
    // Sin recorte el hueco mide cero y «abajo» tapa la pantalla sola.
    Rectangle {
        color: "#73000000"
        width: parent.width
        height: selector.recorteY
    }
    Rectangle {
        color: "#73000000"
        y: selector.recorteY + selector.recorteAlto
        width: parent.width
        height: parent.height - y
    }
    Rectangle {
        color: "#73000000"
        y: selector.recorteY
        width: selector.recorteX
        height: selector.recorteAlto
    }
    Rectangle {
        color: "#73000000"
        x: selector.recorteX + selector.recorteAncho
        y: selector.recorteY
        width: parent.width - x
        height: selector.recorteAlto
    }

    // El rotulo va sobre una pastilla oscura porque el velo ya no esta debajo:
    // el recorte se ve tal cual y el rotulo puede caer sobre cualquier cosa.
    // Blanco sobre blanco no se lee.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height / 8
        width: rotulo.implicitWidth + 24
        height: rotulo.implicitHeight + 12
        radius: 6
        color: "#c8000000"
        visible: zona.accion === ""

        Text {
            id: rotulo
            anchors.centerIn: parent
            // Dos rotulos, porque las teclas que valen no son las mismas antes
            // y despues de tener recorte: sin recorte, Enter no hace nada.
            text: selector.hayRecorte
                  ? qsTr("Enter graba. Arrastra dentro para mover, los bordes para ajustar. Esc cancela.")
                  : qsTr("Arrastra para elegir la región. Esc cancela.")
            color: "white"
            font.pixelSize: 18
        }
    }

    // El borde del recorte. Lo de dentro lo deja ver el velo, que se abre
    // justo aqui.
    Rectangle {
        id: marco
        visible: selector.hayRecorte || zona.accion !== ""
        x: selector.recorteX
        y: selector.recorteY
        width: selector.recorteAncho
        height: selector.recorteAlto
        color: "transparent"
        // El mismo rojo que todo lo que significa grabacion.
        border.color: Sistema.grabando
        border.width: 2

        Text {
            anchors.bottom: parent.top
            anchors.left: parent.left
            anchors.bottomMargin: 4
            color: "white"
            font.pixelSize: 14
            text: Math.round(marco.width) + "×" + Math.round(marco.height)
        }
    }

    MouseArea {
        id: zona
        anchors.fill: parent
        hoverEnabled: true
        focus: true

        // Que se esta arrastrando: "" nada, "nueva", "mover" o "borde".
        property string accion: ""
        // Para "nueva", la esquina de partida; para "mover", donde se agarro
        // dentro del recorte.
        property point ancla: Qt.point(0, 0)
        // Bordes que se estan estirando: 1 izquierda, 2 derecha, 4 arriba,
        // 8 abajo. Una esquina son dos bits a la vez.
        property int bordes: 0

        cursorShape: {
            var c = accion === "borde" ? bordes
                  : accion === "" ? bordesEn(mouseX, mouseY) : 0
            if (c === 5 || c === 10) return Qt.SizeFDiagCursor
            if (c === 6 || c === 9) return Qt.SizeBDiagCursor
            if (c === 1 || c === 2) return Qt.SizeHorCursor
            if (c === 4 || c === 8) return Qt.SizeVerCursor
            if (accion === "mover") return Qt.SizeAllCursor
            if (accion === "" && dentro(mouseX, mouseY)) return Qt.SizeAllCursor
            return Qt.CrossCursor
        }

        // Que bordes se agarran en ese punto. El margen se recorta a la mitad
        // del lado: en un recorte de 20 px, 12 px de agarre alcanzarian los dos
        // bordes a la vez y estirar uno moveria el otro.
        function bordesEn(px, py) {
            if (!selector.hayRecorte) return 0
            var izq = selector.recorteX
            var der = izq + selector.recorteAncho
            var arr = selector.recorteY
            var aba = arr + selector.recorteAlto
            var mx = Math.min(selector.agarre, selector.recorteAncho / 2)
            var my = Math.min(selector.agarre, selector.recorteAlto / 2)
            if (px < izq - mx || px > der + mx || py < arr - my || py > aba + my) return 0
            var c = 0
            if (Math.abs(px - izq) <= mx) c += 1
            else if (Math.abs(px - der) <= mx) c += 2
            if (Math.abs(py - arr) <= my) c += 4
            else if (Math.abs(py - aba) <= my) c += 8
            return c
        }

        function dentro(px, py) {
            return selector.hayRecorte
                   && px >= selector.recorteX
                   && px <= selector.recorteX + selector.recorteAncho
                   && py >= selector.recorteY
                   && py <= selector.recorteY + selector.recorteAlto
        }

        onPressed: function(raton) {
            var c = bordesEn(raton.x, raton.y)
            if (c !== 0) {
                accion = "borde"
                bordes = c
            } else if (dentro(raton.x, raton.y)) {
                accion = "mover"
                ancla = Qt.point(raton.x - selector.recorteX, raton.y - selector.recorteY)
            } else {
                accion = "nueva"
                bordes = 0
                ancla = Qt.point(raton.x, raton.y)
                selector.recorteX = raton.x
                selector.recorteY = raton.y
                selector.recorteAncho = 0
                selector.recorteAlto = 0
            }
        }

        onPositionChanged: function(raton) {
            if (accion === "") return
            // Dentro de la pantalla siempre, y no al soltar: lo que se ve es
            // lo que se va a grabar, y GSR recortaria en silencio lo que se
            // saliera.
            var px = Math.max(0, Math.min(zona.width, raton.x))
            var py = Math.max(0, Math.min(zona.height, raton.y))
            if (accion === "nueva") {
                selector.recorteX = Math.min(ancla.x, px)
                selector.recorteY = Math.min(ancla.y, py)
                selector.recorteAncho = Math.abs(px - ancla.x)
                selector.recorteAlto = Math.abs(py - ancla.y)
            } else if (accion === "mover") {
                selector.recorteX = Math.max(0, Math.min(zona.width - selector.recorteAncho,
                                                         raton.x - ancla.x))
                selector.recorteY = Math.max(0, Math.min(zona.height - selector.recorteAlto,
                                                         raton.y - ancla.y))
            } else {
                var izq = selector.recorteX
                var der = izq + selector.recorteAncho
                var arr = selector.recorteY
                var aba = arr + selector.recorteAlto
                if (bordes & 1) izq = px
                if (bordes & 2) der = px
                if (bordes & 4) arr = py
                if (bordes & 8) aba = py
                // Pasarse de largo cambia de borde, en vez de dejar el recorte
                // del reves o clavado en cero.
                if (izq > der) { var hx = izq; izq = der; der = hx; bordes ^= 3 }
                if (arr > aba) { var hy = arr; arr = aba; aba = hy; bordes ^= 12 }
                selector.recorteX = izq
                selector.recorteY = arr
                selector.recorteAncho = der - izq
                selector.recorteAlto = aba - arr
            }
        }

        onReleased: {
            // Lo que queda por debajo del minimo no es recorte: se descarta y
            // vuelve el rotulo de arrastrar.
            if (!selector.hayRecorte) {
                selector.recorteAncho = 0
                selector.recorteAlto = 0
            }
            accion = ""
        }

        Keys.onEscapePressed: selector.cancelar()
        Keys.onReturnPressed: selector.confirmar()
        Keys.onEnterPressed: selector.confirmar()
    }
}
