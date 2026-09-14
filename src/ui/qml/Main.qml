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
    readonly property bool ocupado: Controlador.estado === "arrancando"
                                    || Controlador.estado === "guardando"

    function tiempoBonito(s) {
        var m = Math.floor(s / 60)
        var r = s % 60
        return (m < 10 ? "0" : "") + m + ":" + (r < 10 ? "0" : "") + r
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
                }

                QQC2.Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                    icon.name: "media-record"
                    text: Controlador.estado === "arrancando" ? qsTr("Arrancando…")
                        : Controlador.estado === "guardando" ? qsTr("Guardando…")
                        : qsTr("Grabar")
                    enabled: !ocupado && fuente.currentText !== ""
                    onClicked: Controlador.grabar(fuente.currentText, calidad.currentValue,
                                                  parseInt(fps.currentText), audio.currentValue)
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
                }
                Kirigami.FormLayout {
                    Layout.fillWidth: true
                    visible: avanzado.checked
                    enabled: !ocupado

                    QQC2.ComboBox {
                        id: calidad
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
                        Kirigami.FormData.label: qsTr("Imágenes por segundo:")
                        model: ["30", "60"]
                        currentIndex: 1
                    }
                    QQC2.ComboBox {
                        id: audio
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
