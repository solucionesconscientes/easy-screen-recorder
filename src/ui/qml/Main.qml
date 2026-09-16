// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// La ventana de Easy Screen Recorder. El contrato de CLAUDE.md manda aqui: grabar en dos
// clics (Fuente, Grabar), lo demas plegado en "Avanzado", y la sencillez de
// Spectacle como referencia. Nada de paneles: una columna.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import es.solucionesconscientes.esr

Kirigami.ApplicationWindow {
    id: raiz
    title: "Easy Screen Recorder"
    width: Kirigami.Units.gridUnit * 24
    height: Math.max(minimumHeight, altoPlegado)
    minimumWidth: Kirigami.Units.gridUnit * 18

    readonly property bool grabando: Controlador.estado === "grabando"
                                     || Controlador.estado === "grabandoAudio"
                                     || Controlador.estado === "pausado"

    // Al grabar pantalla, la ventana se aparta: si no, sale en el video.
    // Vuelve sola al guardar. En solo-audio se queda, que no estorba a nadie
    // y el reloj se agradece.
    //
    // Se MINIMIZA, no se oculta. Ocultarla la sacaba de la barra de tareas y
    // del conmutador de ventanas, y la unica via de vuelta era el icono de la
    // bandeja... que en el flatpak no llegaba a existir, porque el manifiesto
    // no pedia permiso para hablar con org.kde.StatusNotifierWatcher. Resultado
    // medido: al dar a Grabar la aplicacion se esfumaba y habia que relanzarla
    // desde el menu de inicio para poder pausar. Minimizada sigue donde
    // cualquiera la busca, y la bandeja es la segunda via, no la unica.
    readonly property bool apartada: !raiz.visible
                                     || raiz.visibility === Window.Minimized
    function volver() {
        raiz.show()
        raiz.raise()
        raiz.requestActivate()
    }
    Connections {
        target: Controlador
        function onEstadoCambiado() {
            if (Controlador.estado === "grabando") raiz.showMinimized()
            else if (Controlador.estado === "listo" && raiz.apartada) raiz.volver()
        }
    }
    onClosing: function(cierre) {
        // Cerrar la ventana con una grabacion en marcha no la corta: se aparta.
        // Sin nada en marcha, cerrar es salir, como manda la sencillez: nada de
        // procesos residentes porque si.
        //
        // Aqui tambien se minimiza en vez de ocultar, y por lo mismo: la
        // bandeja no existe en todos los escritorios, y una ventana oculta sin
        // bandeja es una grabacion que no se puede parar.
        if (raiz.grabando || ocupado) {
            cierre.accepted = false
            raiz.showMinimized()
        } else {
            Qt.quit()
        }
    }
    readonly property bool ocupado: Controlador.estado === "arrancando"
                                    || Controlador.estado === "guardando"

    // La ventana crece con el desplegable de Avanzado.
    //
    // Antes crecia a una altura fija de 32 unidades de rejilla, y ese numero no
    // daba: medido en una pantalla de 1366x768, las tres ultimas filas
    // —imagenes por segundo, carpeta y audio— quedaban fuera del borde
    // inferior, sin barra de desplazamiento ni nada que insinuara que seguian
    // ahi. Quien quisiera cambiar el audio tenia que descubrir por su cuenta
    // que la ventana se podia estirar.
    //
    // Ahora se pide lo que el formulario mide de verdad, y nunca mas de lo que
    // cabe en la pantalla. Solo crece: si el usuario ya la ha hecho mas grande,
    // no se le encoge debajo.
    readonly property int altoPlegado: Kirigami.Units.gridUnit * 21
    function ajustarAltura() { Qt.callLater(raiz.ajustarAlturaYa) }
    function ajustarAlturaYa() {
        // Cuanto le falta al contenido para caber. No hace falta saber cuanto
        // ocupan el marco y la cabecera: se mide el hueco que queda corto y se
        // le suma eso a la ventana. Se repite hasta que no falte nada o hasta
        // que la pantalla no de mas de si, y por eso termina siempre.
        var falta = columna.implicitHeight - columna.height
        if (falta <= 0) return
        var nuevo = Math.min(raiz.height + falta, Screen.desktopAvailableHeight)
        if (nuevo === raiz.height) return
        raiz.height = nuevo
        Qt.callLater(raiz.ajustarAlturaYa)
    }

    function lanzarGrabacion(region) {
        var opciones = {
            calidad: calidad.currentValue,
            fps: parseInt(fps.currentText),
            audio: audio.currentValue,
            contenedor: contenedor.currentText,
            codecVideo: codecVideo.currentText,
            codecAudio: codecAudio.currentText,
            formatoAudio: formatoAudio.currentText,
            bitrateAudio: bitrateAudio.currentValue
        }
        if (region !== "") opciones.region = region
        Controlador.grabar(fuente.currentValue, opciones)
    }

    function tiempoBonito(s) {
        var m = Math.floor(s / 60)
        var r = s % 60
        return (m < 10 ? "0" : "") + m + ":" + (r < 10 ? "0" : "") + r
    }

    FolderDialog {
        id: dialogoCarpeta
        property bool paraAudio: false
        title: qsTr("¿Dónde se guardan las grabaciones?")
        onAccepted: Controlador.elegirCarpeta(paraAudio, selectedFolder)
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
            id: columna
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
                    textRole: "texto"
                    valueRole: "valor"
                    enabled: !ocupado
                    // Por el IDENTIFICADOR, no por el texto. Antes era
                    // `currentText.indexOf("Solo audio") === 0`, y con la
                    // interfaz traducida ese prefijo no casa: el formulario
                    // ensenaria los controles de video en modo solo-audio.
                    readonly property bool esAudio:
                        String(currentValue).indexOf("audio:") === 0
                }

                QQC2.Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 3
                    icon.name: "media-record"
                    text: Controlador.estado === "arrancando" ? qsTr("Arrancando…")
                        : Controlador.estado === "guardando" ? qsTr("Guardando…")
                        : qsTr("Grabar")
                    enabled: !ocupado && String(fuente.currentValue) !== ""
                    onClicked: {
                        // «region» es el identificador de GSR, no texto visible:
                        // por eso se compara con currentValue y sigue valiendo en
                        // cualquier idioma.
                        if (String(fuente.currentValue) === "region") {
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
                    onCheckedChanged: checked ? raiz.ajustarAltura()
                                              : raiz.height = raiz.altoPlegado
                }
                Kirigami.FormLayout {
                    id: formulario
                    Layout.fillWidth: true
                    visible: avanzado.checked
                    enabled: !ocupado
                    // El formulario cambia de alto al cambiar de fuente: en
                    // solo-audio se esconden los controles de video y aparecen
                    // los de audio. La ventana le sigue.
                    onImplicitHeightChanged: if (avanzado.checked) raiz.ajustarAltura()

                    QQC2.ComboBox {
                        id: formatoAudio
                        visible: fuente.esAudio
                        Kirigami.FormData.label: qsTr("Formato:")
                        // La lista viene del nucleo y no escrita aqui: tenerla
                        // en dos sitios es como se queda una desactualizada.
                        // El orden es el de conveniencia, con opus primero.
                        model: Controlador.formatosAudio()
                    }
                    QQC2.ComboBox {
                        id: bitrateAudio
                        // Se ESCONDE en flac y wav, no se deshabilita: un
                        // control en gris invita a preguntarse por que, y ahi
                        // la respuesta es que ese ajuste no existe porque no
                        // pierden informacion.
                        visible: fuente.esAudio
                                 && !Controlador.formatoAudioSinPerdida(formatoAudio.currentText)
                        Kirigami.FormData.label: qsTr("Calidad del audio:")
                        textRole: "texto"
                        valueRole: "valor"
                        // Todas con cifra. Habia una «Automática (recomendada)»
                        // que no era un nivel de calidad: era el default del
                        // codificador, y medido coincide con una opcion que ya
                        // estaba en la lista —96 kbps en opus, 128 en aac—, asi
                        // que solo aportaba vaguedad y una entrada repetida.
                        //
                        // Se queda 96 preseleccionada porque es exactamente lo
                        // que entregaba «Automática» en opus, que es el formato
                        // por defecto: nadie recibe algo distinto de lo de ayer.
                        //
                        // Y no hay escalones por encima de 192 a proposito. El
                        // nucleo llega hasta 512 y el CLI los acepta, pero opus
                        // es transparente bastante antes: por encima de ~192 los
                        // bits de mas no se oyen. Quien quiere mas no quiere mas
                        // kbps, quiere no perder nada, y eso es flac, que esta
                        // en el selector de Formato, dos filas mas arriba.
                        model: [
                            { texto: qsTr("96 kbps · voz"), valor: 96 },
                            { texto: qsTr("128 kbps · general"), valor: 128 },
                            { texto: qsTr("192 kbps · música (más: flac)"), valor: 192 }
                        ]
                        currentIndex: 0
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
                        // Lo que la maquina soporta de verdad Y cabe en el
                        // formato elegido. Sin el segundo filtro se podia pedir
                        // h264 en un .webm, y eso no graba NADA: el grabador
                        // muere al escribir la cabecera y no deja fichero.
                        // La lista de la maquina se pasa como argumento a
                        // proposito: asi el binding depende de
                        // `Controlador.codecsVideo`, que avisa cuando termina la
                        // deteccion. Sin esa dependencia el selector se quedaba
                        // VACIO, porque la deteccion acaba despues de pintar la
                        // ventana y una llamada a funcion no se reevalua sola.
                        model: Controlador.codecsVideoPara(contenedor.currentText,
                                                           Controlador.codecsVideo)
                        onModelChanged: currentIndex = 0
                    }
                    QQC2.ComboBox {
                        id: codecAudio
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Códec de audio:")
                        // Solo los que GSR respeta en el formato elegido Y en el
                        // reparto de pistas elegido: una pareja invalida ni se
                        // puede pedir. Mezclando, flac no sale de la lista
                        // porque GSR lo cambiaria a opus por detras.
                        model: Controlador.codecsAudioPara(
                                   contenedor.currentText,
                                   String(audio.currentValue) === "mezclado")
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
                    RowLayout {
                        Kirigami.FormData.label: qsTr("Guardar en:")
                        Layout.fillWidth: true
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: fuente.esAudio ? Controlador.carpetaAudio
                                                 : Controlador.carpetaVideos
                            elide: Text.ElideMiddle
                        }
                        QQC2.Button {
                            icon.name: "folder-open"
                            text: qsTr("Cambiar…")
                            onClicked: {
                                dialogoCarpeta.paraAudio = fuente.esAudio
                                dialogoCarpeta.currentFolder = "file://" +
                                    (fuente.esAudio ? Controlador.carpetaAudio
                                                    : Controlador.carpetaVideos)
                                dialogoCarpeta.open()
                            }
                        }
                    }

                    QQC2.ComboBox {
                        id: audio
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Audio:")
                        textRole: "texto"
                        valueRole: "valor"
                        // Mezclado va ANTES que separado a proposito. Con dos
                        // pistas, casi todos los reproductores suenan solo la
                        // primera, asi que quien elige «los dos» sin pensarlo
                        // se encuentra el microfono mudo al reproducir. Las
                        // pistas separadas siguen ahi para quien va a editar,
                        // que es cuando compensan.
                        model: [
                            { texto: qsTr("Audio del sistema"), valor: "sistema" },
                            { texto: qsTr("Micrófono"), valor: "micro" },
                            { texto: qsTr("Los dos, en una sola pista"), valor: "mezclado" },
                            { texto: qsTr("Los dos, en pistas separadas (para editar)"), valor: "ambos" },
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
