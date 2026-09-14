// La ventana de Capturia. El contrato de CLAUDE.md manda aqui: grabar en dos
// clics (Fuente, Grabar), lo demas plegado en "Avanzado", y la sencillez de
// Spectacle como referencia. Nada de paneles: una columna.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.capturia

Kirigami.ApplicationWindow {
    id: raiz
    title: "Capturia"
    width: Kirigami.Units.gridUnit * 24
    height: Math.max(minimumHeight, Kirigami.Units.gridUnit * 21)
    minimumWidth: Kirigami.Units.gridUnit * 18

    readonly property bool grabando: Controlador.estado === "grabando"
                                     || Controlador.estado === "grabandoAudio"
                                     || Controlador.estado === "pausado"

    // Al grabar pantalla, la ventana se aparta: si no, sale en el video.
    // Vuelve sola al guardar. En solo-audio se queda, que no estorba a nadie
    // y el reloj se agradece. La bandeja la recupera en cualquier momento.
    Connections {
        target: Controlador
        function onEstadoCambiado() {
            if (Controlador.estado === "grabando") raiz.hide()
            else if (Controlador.estado === "listo" && !raiz.visible) raiz.show()
        }
    }
    onClosing: function(cierre) {
        // Cerrar la ventana con una grabacion en marcha no la corta: se va a
        // la bandeja. Sin nada en marcha, cerrar es salir, como manda la
        // sencillez: nada de procesos residentes porque si.
        if (raiz.grabando || ocupado) {
            cierre.accepted = false
            raiz.hide()
        } else {
            Qt.quit()
        }
    }
    readonly property bool ocupado: Controlador.estado === "arrancando"
                                    || Controlador.estado === "guardando"

    function lanzarGrabacion(region) {
        var opciones = {
            calidad: calidad.currentValue,
            fps: parseInt(fps.currentText),
            audio: audio.currentValue,
            contenedor: contenedor.currentText,
            codecVideo: codecVideo.currentText,
            codecAudio: codecAudio.currentText,
            formatoAudio: formatoAudio.currentText
        }
        if (region !== "") opciones.region = region
        Controlador.grabar(fuente.currentText, opciones)
    }

    function tiempoBonito(s) {
        var m = Math.floor(s / 60)
        var r = s % 60
        return (m < 10 ? "0" : "") + m + ":" + (r < 10 ? "0" : "") + r
    }

    SelectorRegion {
        id: selectorRegion
        onElegida: function(region) { raiz.lanzarGrabacion(region) }
    }

    // Arnes de capturas: sin inyeccion de entrada no hay clic que abra estos
    // estados, asi que dos argumentos ocultos los abren para fotografiarlos.
    Component.onCompleted: {
        if (Qt.application.arguments.indexOf("--avanzado") !== -1) {
            avanzado.checked = true
        }
        if (Qt.application.arguments.indexOf("--selector") !== -1) {
            selectorRegion.abrir()
        }
    }

    pageStack.initialPage: Kirigami.Page {
        padding: Kirigami.Units.largeSpacing * 2

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing

            // --- Detectando: la ventana pinta al instante y esto dura menos
            // de un segundo (medido). No bloquea nada.
            Kirigami.PlaceholderMessage {
                visible: Controlador.estado === "detectando"
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.name: "search"
                text: qsTr("Mirando qué sabe hacer esta máquina…")
            }

            // --- Sin GSR y sin ffmpeg no hay nada que ofrecer: se dice claro.
            Kirigami.PlaceholderMessage {
                visible: Controlador.estado === "sinGsr"
                Layout.fillWidth: true
                Layout.fillHeight: true
                icon.name: "dialog-error"
                text: qsTr("Falta gpu-screen-recorder")
                explanation: Controlador.diagnostico
            }

            // --- Lo normal: fuente, grabar, y ya.
            ColumnLayout {
                visible: Controlador.estado === "listo" || ocupado
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                QQC2.Label {
                    text: qsTr("Fuente")
                }
                QQC2.ComboBox {
                    id: fuente
                    Layout.fillWidth: true
                    model: Controlador.fuentes
                    enabled: !ocupado
                    readonly property bool esAudio: currentText.indexOf("Solo audio") === 0
                }

                QQC2.Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                    icon.name: "media-record"
                    text: Controlador.estado === "arrancando" ? qsTr("Arrancando…")
                        : Controlador.estado === "guardando" ? qsTr("Guardando…")
                        : qsTr("Grabar")
                    enabled: !ocupado && fuente.currentText !== ""
                    onClicked: {
                        if (fuente.currentText === "region") {
                            selectorRegion.abrir()
                        } else {
                            raiz.lanzarGrabacion("")
                        }
                    }
                }

                // Todo lo que no es Fuente y Grabar vive aqui, plegado. Es
                // contrato de CLAUDE.md, no una preferencia.
                QQC2.Button {
                    id: avanzado
                    Layout.fillWidth: true
                    flat: true
                    checkable: true
                    icon.name: checked ? "collapse" : "expand"
                    text: qsTr("Avanzado")
                    // La ventana crece con el desplegable: sin esto, las
                    // ultimas filas quedaban cortadas y ni se veian.
                    onCheckedChanged: raiz.height = checked
                        ? Kirigami.Units.gridUnit * 32 : Kirigami.Units.gridUnit * 21
                }
                Kirigami.FormLayout {
                    Layout.fillWidth: true
                    visible: avanzado.checked
                    enabled: !ocupado

                    QQC2.ComboBox {
                        id: formatoAudio
                        visible: fuente.esAudio
                        Kirigami.FormData.label: qsTr("Formato:")
                        // opus para el caso general, flac para calidad. La
                        // decision y el porque, en ESTADO.md de la Tanda 5.
                        model: ["opus", "flac"]
                    }
                    QQC2.ComboBox {
                        id: contenedor
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Formato:")
                        model: Controlador.contenedores
                    }
                    QQC2.ComboBox {
                        id: codecVideo
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Códec de vídeo:")
                        // Lo que la maquina soporta de verdad, detectado; el
                        // sufijo _software es CPU y GSR lo nombra asi.
                        model: Controlador.codecsVideo
                    }
                    QQC2.ComboBox {
                        id: codecAudio
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Códec de audio:")
                        // Solo los que GSR respeta en el formato elegido: una
                        // pareja invalida ni se puede pedir.
                        model: Controlador.codecsAudioPara(contenedor.currentText)
                        onModelChanged: currentIndex = 0
                    }
                    QQC2.ComboBox {
                        id: calidad
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Calidad:")
                        textRole: "texto"
                        valueRole: "valor"
                        model: [
                            { texto: qsTr("Media"), valor: "medium" },
                            { texto: qsTr("Alta"), valor: "high" },
                            { texto: qsTr("Muy alta"), valor: "very_high" },
                            { texto: qsTr("Ultra"), valor: "ultra" }
                        ]
                        currentIndex: 2  // very_high, el default de GSR
                    }
                    QQC2.ComboBox {
                        id: fps
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Imágenes por segundo:")
                        model: ["30", "60"]
                        currentIndex: 1
                    }
                    QQC2.ComboBox {
                        id: audio
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Audio:")
                        textRole: "texto"
                        valueRole: "valor"
                        model: [
                            { texto: qsTr("Lo que suena"), valor: "sistema" },
                            { texto: qsTr("Micrófono"), valor: "micro" },
                            { texto: qsTr("Los dos, en pistas separadas"), valor: "ambos" },
                            { texto: qsTr("Sin audio"), valor: "nada" }
                        ]
                        currentIndex: 0
                    }
                }

                // La ruta de lo ultimo guardado, clicable de palabra.
                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: Controlador.rutaGuardada !== ""
                    type: Kirigami.MessageType.Positive
                    text: qsTr("Guardado en %1").arg(Controlador.rutaGuardada)
                    actions: Kirigami.Action {
                        text: qsTr("Abrir carpeta")
                        icon.name: "folder-open"
                        onTriggered: Qt.openUrlExternally(
                            "file://" + Controlador.rutaGuardada.substring(
                                0, Controlador.rutaGuardada.lastIndexOf("/")))
                    }
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: Controlador.error !== ""
                    type: Kirigami.MessageType.Error
                    text: Controlador.error
                }

                // Los avisos de --check, sin bloquear: informan, no regañan.
                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: Controlador.diagnostico !== "" && !grabando
                    type: Kirigami.MessageType.Warning
                    text: Controlador.diagnostico
                }
            }

            // --- Grabando: un reloj y dos botones. Nada mas que mirar.
            ColumnLayout {
                visible: grabando
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Kirigami.Units.largeSpacing

                Item { Layout.fillHeight: true }

                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: tiempoBonito(Controlador.segundos)
                    font.pixelSize: Kirigami.Units.gridUnit * 3
                    font.features: { "tnum": 1 }
                }
                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: Controlador.estado === "pausado" ? qsTr("En pausa")
                        : Controlador.estado === "grabandoAudio" ? qsTr("Grabando audio")
                        : qsTr("Grabando")
                    opacity: 0.7
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Kirigami.Units.largeSpacing

                    QQC2.Button {
                        // GSR no pausa el modo audio-only: la pausa es del IPC
                        // de la pantalla. En audio no se ofrece.
                        visible: Controlador.estado !== "grabandoAudio"
                        icon.name: Controlador.estado === "pausado"
                                   ? "media-playback-start" : "media-playback-pause"
                        text: Controlador.estado === "pausado" ? qsTr("Reanudar") : qsTr("Pausa")
                        onClicked: Controlador.estado === "pausado"
                                   ? Controlador.reanudar() : Controlador.pausar()
                    }
                    QQC2.Button {
                        icon.name: "media-playback-stop"
                        text: qsTr("Parar y guardar")
                        onClicked: Controlador.parar()
                    }
                }

                Item { Layout.fillHeight: true }
            }

            Item {
                visible: !grabando && Controlador.estado !== "detectando"
                Layout.fillHeight: true
            }
        }
    }
}
