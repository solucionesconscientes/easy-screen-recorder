// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// El selector de region: una ventana a pantalla completa, oscurecida, donde
// se arrastra el recorte. Soltar NO elige: deja el recorte ajustable, se
// mueve por dentro y se estira por los bordes. Enter graba; Esc cancela. La
// referencia es el selector de Spectacle: sin botones, sin dialogos.
//
// El tamaño tambien se escribe. Arrastrando sale un recorte a ojo, y quien
// graba para algo con medidas fijas necesita 1280x720, no 1277x719.
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

    // Pixeles de video por cada pixel de esta ventana. El recorte va en
    // pixeles logicos, que es lo que espera -region, y GSR los multiplica por
    // la escala del monitor (capture_setup.c:81 y :213-218 de su 6.0.0). Lo
    // que se escribe y lo que se enseña es el tamaño del video, porque ese es
    // el numero que se busca. Con escala 1 coinciden; con otra esta SIN
    // VERIFICAR, como el resto del selector (ESTADO.md).
    readonly property real escala: Screen.devicePixelRatio
    readonly property int anchoVideo: Math.round(recorteAncho * escala)
    readonly property int altoVideo: Math.round(recorteAlto * escala)

    // Lo que hay que decir junto al campo del tamaño: que se ajusto a la
    // pantalla, o que no se entendio lo escrito.
    property string aviso: ""

    flags: Qt.FramelessWindowHint
    color: "transparent"

    function abrir() {
        visibility = Window.FullScreen
        visible = true
        recorteAncho = 0
        recorteAlto = 0
        zona.accion = ""
        campo.text = ""
        aviso = ""
        // El foco vive en el campo desde el principio: asi basta con empezar
        // a escribir, sin buscar donde hacer clic.
        campo.forceActiveFocus()
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

    // «1280x720», con espacios o sin ellos, y tambien con × o *. La × es la
    // que enseña el rotulo del marco, y quien copia lo que ve no deberia
    // tropezar con ella.
    function leerTamano(texto) {
        var m = /^\s*(\d+)\s*[xX×*\s]\s*(\d+)\s*$/.exec(texto)
        return m ? { ancho: parseInt(m[1], 10), alto: parseInt(m[2], 10) } : null
    }

    // El recorte al tamaño escrito, en pixeles de video. Sin recorte sale
    // centrado en la pantalla. Con uno ya puesto crece desde SU centro, porque
    // quien escribe despues de arrastrar esta afinando ese encuadre y no otro.
    function ponerTamano(ancho, alto) {
        var w = Math.max(minimo, ancho / escala)
        var h = Math.max(minimo, alto / escala)
        // Lo que no cabe se ajusta y se dice, igual que al arrastrar: GSR
        // recortaria en silencio lo que se saliera.
        var cabe = w <= selector.width && h <= selector.height
        w = Math.min(w, selector.width)
        h = Math.min(h, selector.height)
        var cx = hayRecorte ? recorteX + recorteAncho / 2 : selector.width / 2
        var cy = hayRecorte ? recorteY + recorteAlto / 2 : selector.height / 2
        recorteX = Math.max(0, Math.min(selector.width - w, Math.round(cx - w / 2)))
        recorteY = Math.max(0, Math.min(selector.height - h, Math.round(cy - h / 2)))
        recorteAncho = w
        recorteAlto = h
        sincronizarCampo()
        if (!cabe) aviso = qsTr("Ajustado al tamaño de la pantalla.")
    }

    // El campo enseña el tamaño del recorte, tambien el arrastrado, y lo deja
    // seleccionado: lo siguiente que se escriba lo sustituye en vez de
    // añadirse detras.
    function sincronizarCampo() {
        campo.text = hayRecorte ? anchoVideo + "x" + altoVideo : ""
        campo.selectAll()
        aviso = ""
    }

    // Enter con lo escrito igual al recorte que ya hay graba, como Enter en
    // cualquier otro momento. Por eso tras escribir un tamaño el primer Enter
    // lo pone y el segundo graba, y quien arrastro sin tocar el campo graba al
    // primero.
    function enterEnCampo() {
        var texto = campo.text.trim()
        var t = leerTamano(texto)
        if (texto === "" || (t && hayRecorte && t.ancho === anchoVideo && t.alto === altoVideo)) {
            confirmar()
        } else if (t) {
            ponerTamano(t.ancho, t.alto)
        } else {
            aviso = qsTr("Escribe el ancho y el alto, por ejemplo 1280x720.")
        }
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
        // Por encima de «zona», que tapa la ventana entera: si no, el clic en
        // el campo empezaria un recorte. El resto de la pastilla deja pasar el
        // raton, porque un Rectangle no lo recoge.
        z: 1
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height / 8
        width: pastilla.implicitWidth + 24
        height: pastilla.implicitHeight + 16
        radius: 6
        color: "#c8000000"
        // Se desvanece al arrastrar en vez de ocultarse. Oculto, el campo
        // perderia el foco y con el las teclas: Enter dejaria de grabar.
        opacity: zona.accion === "" ? 1 : 0

        Column {
            id: pastilla
            anchors.centerIn: parent
            spacing: 8

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                // Dos rotulos, porque las teclas que valen no son las mismas
                // antes y despues de tener recorte: sin recorte, Enter no graba.
                text: selector.hayRecorte
                      ? qsTr("Enter graba. Arrastra dentro para mover, los bordes para ajustar. Esc cancela.")
                      : qsTr("Arrastra para elegir la región. Esc cancela.")
                color: "white"
                font.pixelSize: 18
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: selector.hayRecorte ? qsTr("Tamaño:") : qsTr("O escribe su tamaño:")
                    color: "white"
                    font.pixelSize: 18
                }

                // Blanco al 60 %: sobre la pastilla da 5,43 de contraste en el
                // peor caso, con la pastilla encima de blanco, y 7,37 encima de
                // negro. AA pide 4,5.
                Rectangle {
                    width: Math.max(campo.contentWidth, ejemplo.implicitWidth) + 16
                    height: campo.implicitHeight + 8
                    radius: 4
                    color: "transparent"
                    border.color: "#99ffffff"
                    border.width: 1

                    TextInput {
                        id: campo
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        color: "white"
                        // Seleccion en video inverso y no con el resaltado del
                        // sistema: blanco sobre el azul de Breeze da 2,49, y
                        // el campo pasa seleccionado casi todo el rato.
                        selectionColor: "white"
                        selectedTextColor: "black"
                        font.pixelSize: 18
                        selectByMouse: true
                        maximumLength: 15
                        // Solo lo que puede formar un tamaño. Una letra no
                        // llega a entrar, que es mejor que explicar despues
                        // por que no vale.
                        validator: RegularExpressionValidator {
                            regularExpression: /[0-9xX×* ]*/
                        }
                        onTextEdited: selector.aviso = ""

                        // Las teclas viven aqui porque el foco no sale de aqui
                        // mientras el selector esta abierto.
                        Keys.onEscapePressed: selector.cancelar()
                        Keys.onReturnPressed: selector.enterEnCampo()
                        Keys.onEnterPressed: selector.enterEnCampo()

                        Text {
                            id: ejemplo
                            anchors.verticalCenter: parent.verticalCenter
                            visible: campo.text === ""
                            text: qsTr("ancho x alto")
                            color: "#99ffffff"
                            font.pixelSize: 18
                        }
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: selector.aviso !== ""
                    text: selector.aviso
                    color: "white"
                    font.pixelSize: 18
                }
            }
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
            // El del video, el mismo numero que el campo.
            text: selector.anchoVideo + "×" + selector.altoVideo
        }
    }

    MouseArea {
        id: zona
        anchors.fill: parent
        hoverEnabled: true

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
            selector.sincronizarCampo()
        }
    }
}
