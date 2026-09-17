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
    width: anchoPlegado
    height: Math.max(minimumHeight, altoPlegado)
    minimumWidth: Kirigami.Units.gridUnit * 18

    readonly property bool grabando: Controlador.estado === "grabando"
                                     || Controlador.estado === "grabandoAudio"
                                     || Controlador.estado === "pausado"
                                     || Controlador.estado === "replay"
                                     || Controlador.estado === "emitiendo"

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
            if (Controlador.estado === "grabando" || Controlador.estado === "replay"
                    || Controlador.estado === "emitiendo") raiz.showMinimized()
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
    readonly property int anchoPlegado: Kirigami.Units.gridUnit * 24
    // Al abrir «Avanzado» la ventana tambien se ENSANCHA. Plegada es una columna
    // estrecha, que es lo que pide «grabar en dos clics»; abierta son dos
    // columnas y el mapa de la camara, y en 24 unidades de rejilla no caben.
    // Sin esto, las dos columnas se apretarian y el problema de las filas que no
    // se ven volveria por otro lado.
    readonly property int anchoAbierto: Kirigami.Units.gridUnit * 46
    // Maximizada o a pantalla completa, el tamaño lo manda el gestor de
    // ventanas y no nosotros. Sin esta comprobacion, abrir «Avanzado» en una
    // ventana maximizada le daba un ancho y un alto propios y la dejaba en un
    // estado raro: ni maximizada ni del tamaño que pedia.
    readonly property bool mandaElGestor: raiz.visibility === Window.Maximized
                                          || raiz.visibility === Window.FullScreen
    function ajustarAltura() { Qt.callLater(raiz.ajustarAlturaYa) }
    function ajustarAlturaYa() {
        if (raiz.mandaElGestor) return
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
            bitrateAudio: bitrateAudio.currentValue,
            camara: camaraActiva ? String(camara.currentValue) : "",
            camaraTamano: tamanoCamara.value,
            camaraX: colocacion.xPct,
            camaraY: colocacion.yPct,
            camaraEspejo: espejoCamara.checked,
            modoFotogramas: soloAlCambiar.visible && soloAlCambiar.checked ? "content" : "",
            limiteResolucion: limiteResolucion.currentValue,
            replaySegundos: replay.checked ? segundosReplay.currentValue : 0
        }
        if (region !== "") opciones.region = region
        if (raiz.emitiendo) {
            Controlador.emitir(fuente.currentValue, servidorEmision.text.trim(),
                               claveEmision.text.trim(),
                               parseInt(bitrateEmision.currentValue), opciones)
        } else {
            Controlador.grabar(fuente.currentValue, opciones)
        }
    }

    // La camara superpuesta solo tiene sentido grabando pantalla: sobre una
    // fuente que YA es la camara, o sobre una nota de voz, no pinta nada.
    // Emitir es un MODO, no una opcion mas: cambia el boton principal, quita la
    // pausa y convierte la calidad en un bitrate. Por eso tiene su propia
    // casilla y no se cuela como un ajuste cualquiera.
    readonly property bool emitiendo: emitirEnDirecto.visible && emitirEnDirecto.checked

    readonly property bool puedeCamara: !fuente.esAudio
                                        && String(fuente.currentValue).indexOf("/dev/") !== 0
                                        && Controlador.camaras.length > 0
    readonly property bool camaraActiva: puedeCamara && usarCamara.checked

    // Cuenta atras antes de empezar. No es decoracion: da tiempo a quitar el
    // raton de encima del boton y a colocarse delante de la camara, y evita que
    // el primer segundo de cada grabacion sea siempre el puntero sobre
    // «Grabar». Ocurre ANTES de arrancar, asi que no sale en el video.
    property int cuentaAtras: 0
    property string regionPendiente: ""
    Timer {
        id: relojCuentaAtras
        interval: 1000
        repeat: true
        onTriggered: {
            raiz.cuentaAtras -= 1
            if (raiz.cuentaAtras <= 0) {
                stop()
                raiz.lanzarGrabacion(raiz.regionPendiente)
            }
        }
    }
    function empezarCon(region) {
        raiz.regionPendiente = region
        if (cuentaAtrasActiva.checked) {
            raiz.cuentaAtras = 3
            relojCuentaAtras.start()
        } else {
            raiz.lanzarGrabacion(region)
        }
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
        onElegida: function(region) { raiz.empezarCon(region) }
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
                        : raiz.emitiendo ? qsTr("Emitir en directo")
                        : qsTr("Grabar")
                    // Emitiendo hace falta la clave: sin ella el servidor
                    // rechaza la conexion y el usuario solo ve que no pasa nada.
                    enabled: !ocupado && String(fuente.currentValue) !== ""
                             && (!raiz.emitiendo || claveEmision.text.trim() !== "")
                    onClicked: {
                        // «region» es el identificador de GSR, no texto visible:
                        // por eso se compara con currentValue y sigue valiendo en
                        // cualquier idioma.
                        if (String(fuente.currentValue) === "region") {
                            selectorRegion.abrir()
                        } else {
                            raiz.empezarCon("")
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
                    onCheckedChanged: {
                        if (raiz.mandaElGestor) return
                        if (checked) {
                            raiz.width = Math.min(raiz.anchoAbierto,
                                                  Screen.desktopAvailableWidth)
                            raiz.ajustarAltura()
                        } else {
                            raiz.width = raiz.anchoPlegado
                            raiz.height = raiz.altoPlegado
                        }
                    }
                }
                // «Avanzado», en DOS COLUMNAS.
                //
                // Era una sola y no cabia: con nueve funciones nuevas las
                // ultimas filas quedaban fuera de la ventana y, a la vez,
                // sobraba la mitad derecha, porque un formulario de Kirigami
                // pone la etiqueta a la izquierda y el control en medio. Dos
                // columnas usan el hueco que ya estaba ahi.
                ColumnLayout {
                    id: formulario
                    Layout.fillWidth: true
                    visible: avanzado.checked
                    enabled: !ocupado
                    spacing: Kirigami.Units.largeSpacing
                    onImplicitHeightChanged: if (avanzado.checked) raiz.ajustarAltura()

                    // --- Emitir en directo ---------------------------------
                    //
                    // Va arriba y a lo ancho porque es un MODO: mientras esta
                    // puesto, no se graba a fichero, no hay pausa y la calidad
                    // es un bitrate. Mezclarlo entre los ajustes de vídeo habria
                    // hecho que se activara sin querer.
                    QQC2.CheckBox {
                        id: emitirEnDirecto
                        visible: !fuente.esAudio
                        text: qsTr("Emitir en directo en vez de guardar un fichero")
                        QQC2.ToolTip.text: qsTr(
                            "Manda la pantalla a YouTube, Twitch o cualquier servidor RTMP. " +
                            "No se guarda nada en el disco.\n\nNo se puede pausar: dejar de " +
                            "mandar imagen hace que la plataforma dé la emisión por caída.")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: 400
                    }
                    RowLayout {
                        visible: raiz.emitiendo
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.largeSpacing
                        QQC2.Label { text: qsTr("Servidor:") }
                        QQC2.TextField {
                            id: servidorEmision
                            Layout.fillWidth: true
                            // Se recuerda entre sesiones: es una URL publica, la
                            // misma que YouTube pone en su propia pagina.
                            text: Controlador.urlEmision
                            placeholderText: "rtmp://a.rtmp.youtube.com/live2"
                        }
                        QQC2.Label { text: qsTr("Bitrate:") }
                        QQC2.ComboBox {
                            id: bitrateEmision
                            textRole: "texto"
                            valueRole: "valor"
                            // Los tres que recomienda YouTube para 30 imagenes
                            // por segundo. Emitiendo no hay «calidad»: hay un
                            // caudal, y lo fija la plataforma, no nosotros.
                            model: [
                                { texto: qsTr("1080p · 4500 kbps"), valor: 4500 },
                                { texto: qsTr("720p · 2500 kbps"), valor: 2500 },
                                { texto: qsTr("1440p · 9000 kbps"), valor: 9000 }
                            ]
                            currentIndex: 0
                        }
                    }
                    RowLayout {
                        visible: raiz.emitiendo
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.largeSpacing
                        QQC2.Label { text: qsTr("Clave:") }
                        QQC2.TextField {
                            id: claveEmision
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: qsTr("pégala aquí; no se guarda")
                        }
                        QQC2.Label {
                            // Dicho donde se pega, no en la documentacion: es una
                            // credencial y el usuario tiene derecho a saber que
                            // no se queda en ningun sitio.
                            text: qsTr("La clave no se guarda: hay que pegarla cada vez")
                            opacity: 0.7
                            font: Kirigami.Theme.smallFont
                        }
                    }

                    // --- Camara, a lo ancho: el mapa necesita sitio ---------
                    QQC2.CheckBox {
                        id: usarCamara
                        visible: raiz.puedeCamara
                        text: qsTr("Superponer la cámara sobre la pantalla")
                        QQC2.ToolTip.text: qsTr(
                            "Pantalla y cámara en la misma grabación y el mismo " +
                            "fichero, sin nada que montar después.\n\nLa vista " +
                            "previa se apaga al empezar a grabar: una cámara solo " +
                            "admite un programa a la vez, y a partir de ahí es " +
                            "del grabador. Por eso el encuadre se elige antes.")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: 400
                    }
                    // Los ajustes de la camara en UNA fila: el alto de esta
                    // ventana es lo escaso, no el ancho. Antes iban en columna
                    // y, con la vista previa y el mapa debajo, el bloque no
                    // cabia en una pantalla de 768 px y se comia el final del
                    // formulario.
                    RowLayout {
                        visible: raiz.camaraActiva
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.largeSpacing

                        QQC2.ComboBox {
                            id: camara
                            Layout.fillWidth: true
                            Layout.maximumWidth: Kirigami.Units.gridUnit * 14
                            model: Controlador.camaras
                            textRole: "texto"
                            valueRole: "valor"
                        }
                        QQC2.Label { text: qsTr("Tamaño:") }
                        QQC2.Slider {
                            id: tamanoCamara
                            // En PORCENTAJE del ancho, no en pixeles: un tamaño
                            // fijo es un cuarto de pantalla en 1366 y un decimo
                            // en 4K.
                            from: 5; to: 50; stepSize: 1; value: 25
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                        }
                        QQC2.Label {
                            text: tamanoCamara.value + " %"
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        }
                        QQC2.CheckBox {
                            id: espejoCamara
                            checked: true
                            text: qsTr("Espejo")
                        }
                        Item { Layout.fillWidth: true }
                    }

                    // Y debajo, las dos ayudas a la vez: a la izquierda tu cara,
                    // a la derecha donde va a quedar. Las dos hacen falta y
                    // responden a preguntas distintas.
                    RowLayout {
                        visible: raiz.camaraActiva
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.largeSpacing

                        // Verte la cara. Va en un Loader porque el fichero solo
                        // existe si se compilo con Qt6Multimedia. Se apaga al
                        // grabar: la camara admite un solo cliente y a partir de
                        // ahi es de GSR.
                        Loader {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                            visible: Controlador.hayMultimedia
                            active: visible
                            source: "VistaPreviaCamara.qml"
                            onLoaded: {
                                item.dispositivo = Qt.binding(function() {
                                    return String(camara.currentValue)
                                })
                                item.espejo = Qt.binding(function() {
                                    return espejoCamara.checked
                                })
                                item.activa = Qt.binding(function() {
                                    return Controlador.estado === "listo"
                                })
                            }
                        }

                        // El mapa: donde va a quedar dentro del video.
                        ColocarCamara {
                            id: colocacion
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                            anchoPct: tamanoCamara.value
                            proporcionPantalla: Screen.width / Screen.height
                            proporcionCamara: Controlador.proporcionCamara(
                                                  String(camara.currentValue))
                        }
                    }

                    // --- Y el resto, en dos columnas -----------------------
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.gridUnit * 2

                        Kirigami.FormLayout {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            // NO se usa `twinFormLayouts` para igualar el ancho
                            // de las etiquetas entre las dos columnas: con
                            // «Códec de audio:» mandando, las de la izquierda se
                            // salian por el borde y «Imágenes por segundo» se
                            // leia «ágenes por segundo». Son dos columnas
                            // independientes; cada una se mide sola.
                            //
                            // Etiqueta AL LADO del control, no encima. Kirigami
                            // decide solo segun el ancho que le toque y aqui
                            // elegia «encima», que dobla el alto de cada fila:
                            // con once filas eso son casi 200 px de mas y el
                            // final del formulario quedaba fuera de la pantalla.
                            // Lo que sobra es ancho, asi que se usa el ancho.
                            wideMode: true

                            QQC2.ComboBox {
                                id: contenedor
                                // Emitiendo el contenedor es flv y el codec aac:
                                // no hay nada que elegir, asi que no se enseña.
                                visible: !fuente.esAudio && !raiz.emitiendo
                                Kirigami.FormData.label: qsTr("Formato:")
                                model: Controlador.contenedores
                            }
                            QQC2.ComboBox {
                                id: codecVideo
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Códec de vídeo:")
                                // Lo que la maquina soporta Y cabe en el
                                // formato: sin el segundo filtro se podia pedir
                                // h264 en un .webm, y eso no graba nada.
                                model: Controlador.codecsVideoPara(contenedor.currentText,
                                                                   Controlador.codecsVideo)
                                onModelChanged: currentIndex = 0
                            }
                            QQC2.ComboBox {
                                id: calidad
                                // Emitiendo se fija un bitrate, no una calidad.
                                visible: !fuente.esAudio && !raiz.emitiendo
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
                                // La pregunta que hace todo el mundo aqui es
                                // «¿cuantos kbps son?». Ninguno: GSR usa calidad
                                // CONSTANTE (QP 35/30/25/22, video_codec.c:9-17),
                                // asi que el tamaño lo decide lo que pase en
                                // pantalla. Las cifras son de una medicion real
                                // en esta maquina, con el escritorio poco movido.
                                QQC2.ToolTip.text: qsTr(
                                    "No es un bitrate fijo: es calidad constante, " +
                                    "así que el tamaño depende de lo que se mueva " +
                                    "en pantalla.\n\nCon una pantalla poco movida, " +
                                    "medido: Media 1,5 MB/min · Alta 2,4 · Muy alta " +
                                    "3,0 · Ultra 4,6. Con vídeo o desplazamiento " +
                                    "sube bastante.")
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: 400
                            }
                            QQC2.ComboBox {
                                id: fps
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Imágenes por segundo:")
                                model: ["30", "60"]
                                currentIndex: 1
                            }
                            QQC2.ComboBox {
                                id: limiteResolucion
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Tamaño del vídeo:")
                                textRole: "texto"
                                valueRole: "valor"
                                model: [
                                    { texto: qsTr("El de la pantalla"), valor: "" },
                                    { texto: qsTr("Como mucho 1080p"), valor: "1920x1080" },
                                    { texto: qsTr("Como mucho 720p"), valor: "1280x720" }
                                ]
                                currentIndex: 0
                            }
                            QQC2.ComboBox {
                                id: formatoAudio
                                visible: fuente.esAudio
                                Kirigami.FormData.label: qsTr("Formato:")
                                model: Controlador.formatosAudio()
                            }
                            QQC2.ComboBox {
                                id: bitrateAudio
                                // Se ESCONDE en flac y wav, no se deshabilita:
                                // ahi ese ajuste no existe porque no pierden
                                // informacion.
                                visible: fuente.esAudio
                                         && !Controlador.formatoAudioSinPerdida(
                                                formatoAudio.currentText)
                                Kirigami.FormData.label: qsTr("Calidad del audio:")
                                textRole: "texto"
                                valueRole: "valor"
                                model: [
                                    { texto: qsTr("96 kbps · voz"), valor: 96 },
                                    { texto: qsTr("128 kbps · general"), valor: 128 },
                                    { texto: qsTr("192 kbps · música (más: flac)"), valor: 192 }
                                ]
                                currentIndex: 0
                            }
                        }

                        Kirigami.FormLayout {
                            id: columnaDerecha
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            wideMode: true

                            QQC2.ComboBox {
                                id: audio
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Audio:")
                                textRole: "texto"
                                valueRole: "valor"
                                // Mezclado va ANTES que separado: con dos pistas
                                // casi todos los reproductores suenan solo la
                                // primera y el microfono parece mudo. Detras,
                                // las aplicaciones que suenan AHORA.
                                model: [
                                    { texto: qsTr("Audio del sistema"), valor: "sistema" },
                                    { texto: qsTr("Micrófono"), valor: "micro" },
                                    { texto: qsTr("Los dos, en una sola pista"), valor: "mezclado" },
                                    { texto: qsTr("Los dos, en pistas separadas (para editar)"), valor: "ambos" },
                                    { texto: qsTr("Sin audio"), valor: "nada" }
                                ].concat(Controlador.audiosAplicacion.map(function(a) {
                                    // «Solo smplayer» no decia de QUE: se leia
                                    // como «solamente smplayer» sin mas. Lo que
                                    // hace es grabar el sonido de esa aplicacion
                                    // y dejar fuera todo lo demas.
                                    return { texto: qsTr("Solo el sonido de %1").arg(a.texto),
                                             valor: a.valor }
                                }))
                                currentIndex: 0
                                // Al desplegarlo se vuelve a preguntar que esta
                                // sonando. La lista calculada al arrancar casi
                                // nunca sirve: entre abrir esto y darle a grabar,
                                // el usuario abre justo lo que queria grabar.
                                onPressedChanged: if (pressed) {
                                    Controlador.refrescarAplicacionesSonando()
                                }
                                QQC2.ToolTip.text: qsTr(
                                    "«En una sola pista» mezcla los dos y se oye " +
                                    "todo en cualquier reproductor.\n\n«En pistas " +
                                    "separadas» deja cada uno por su lado para " +
                                    "poder equilibrarlos al editar, pero casi " +
                                    "todos los reproductores suenan solo la " +
                                    "primera: el micrófono parecerá mudo.")
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: 400
                            }
                            QQC2.ComboBox {
                                id: codecAudio
                                visible: !fuente.esAudio && !raiz.emitiendo
                                Kirigami.FormData.label: qsTr("Códec de audio:")
                                // Solo los que GSR respeta en ese formato y en
                                // ese reparto de pistas.
                                model: Controlador.codecsAudioPara(
                                           contenedor.currentText,
                                           String(audio.currentValue) === "mezclado")
                                onModelChanged: currentIndex = 0
                            }
                            RowLayout {
                                id: filaVumetro
                                visible: Controlador.hayMultimedia && !fuente.esAudio
                                         && ["micro", "mezclado", "ambos"].indexOf(
                                                String(audio.currentValue)) !== -1
                                Kirigami.FormData.label: qsTr("Nivel del micro:")
                                onVisibleChanged: Controlador.escucharMicro(visible)
                                Component.onCompleted: Controlador.escucharMicro(visible)
                                QQC2.ProgressBar {
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 6
                                    from: 0; to: 1
                                    // Escala en DECIBELIOS, de -60 dB a 0. Con
                                    // la lineal la barra no se movia: una voz
                                    // normal en este microfono da 0,004 de RMS.
                                    value: {
                                        var n = Controlador.nivelMicro
                                        if (n <= 0.0001) return 0
                                        var db = 20 * Math.log(n) / Math.LN10
                                        return Math.max(0, Math.min(1, (db + 60) / 60))
                                    }
                                }
                                QQC2.Label {
                                    text: Controlador.nivelMicro > 0.002 ? qsTr("te oigo")
                                                                         : qsTr("sin señal")
                                    opacity: 0.7
                                }
                            }
                            QQC2.Label {
                                // Los atajos existian desde hace tandas y la
                                // aplicacion no los mencionaba en ningun sitio,
                                // asi que para el usuario no existian. Un atajo
                                // que no se anuncia es codigo muerto.
                                visible: Controlador.hayAtajos
                                Kirigami.FormData.label: qsTr("Atajos:")
                                text: qsTr("%1 graba y para").arg(Controlador.atajoGrabar)
                                      + (Controlador.atajoPausa !== ""
                                         ? "\n" + qsTr("%1 pausa y reanuda")
                                                   .arg(Controlador.atajoPausa)
                                         : "")
                                opacity: 0.8
                                QQC2.ToolTip.text: qsTr(
                                    "Funcionan con la ventana cerrada o minimizada, que es "
                                    + "para lo que sirven: pausar sin que la ventana salga "
                                    + "en el vídeo.\n\nSe pueden cambiar en Preferencias del "
                                    + "sistema, en Atajos de teclado.")
                                // Una etiqueta no tiene «hovered» propio, asi
                                // que el raton lo vigila un HoverHandler.
                                HoverHandler { id: sobreAtajos }
                                QQC2.ToolTip.visible: sobreAtajos.hovered
                                QQC2.ToolTip.delay: 400
                            }
                            QQC2.CheckBox {
                                id: cuentaAtrasActiva
                                Kirigami.FormData.label: qsTr("Al empezar:")
                                checked: true
                                text: qsTr("Contar 3 segundos")
                            }
                            QQC2.CheckBox {
                                id: soloAlCambiar
                                // Solo donde de verdad hace algo: en Wayland
                                // sobre un monitor GSR lo acepta, avisa por
                                // stderr y lo ignora.
                                visible: !fuente.esAudio
                                         && Controlador.modoContentEfectivo(
                                                String(fuente.currentValue))
                                text: qsTr("Codificar solo al cambiar la pantalla")
                                QQC2.ToolTip.text: qsTr(
                                    "Mientras la pantalla esté quieta no gasta " +
                                    "GPU ni ocupa sitio. Va bien en un tutorial " +
                                    "con pausas.\n\nSolo aparece donde funciona " +
                                    "de verdad; en Wayland grabando un monitor no " +
                                    "hace nada, así que ahí no se ofrece.")
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: 400
                            }
                            QQC2.CheckBox {
                                id: replay
                                // Repetir y emitir se excluyen: emitiendo no hay
                                // buffer que guardar, lo que sale ya se ha ido.
                                visible: !fuente.esAudio && !raiz.emitiendo
                                text: qsTr("Modo repetición")
                                QQC2.ToolTip.text: qsTr(
                                    "Graba sin escribir nada: va guardando en " +
                                    "memoria los últimos minutos y solo los " +
                                    "vuelca a un fichero cuando pulsas «Guardar " +
                                    "lo último».\n\nSirve para lo que YA ha " +
                                    "pasado: te das cuenta de que querías " +
                                    "grabarlo cuando ya ha ocurrido, y todavía " +
                                    "estás a tiempo.")
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: 400
                            }
                            QQC2.ComboBox {
                                id: segundosReplay
                                visible: !fuente.esAudio && replay.checked
                                Kirigami.FormData.label: qsTr("Guardar los últimos:")
                                textRole: "texto"
                                valueRole: "valor"
                                model: [
                                    { texto: qsTr("30 segundos"), valor: 30 },
                                    { texto: qsTr("1 minuto"), valor: 60 },
                                    { texto: qsTr("5 minutos"), valor: 300 },
                                    { texto: qsTr("15 minutos"), valor: 900 }
                                ]
                                currentIndex: 1
                            }
                        }
                    }

                    // La carpeta, a lo ancho: una ruta larga no cabe en media.
                    // Emitiendo no se guarda nada, asi que ni se enseña.
                    //
                    // El hueco sobrante va DESPUES del boton, no entre la ruta y
                    // el boton. Con la ruta estirandose, «Cambiar…» acababa
                    // pegado al borde derecho, a media ventana de la ruta que
                    // cambia: parecian dos cosas distintas.
                    RowLayout {
                        visible: !raiz.emitiendo
                        Layout.fillWidth: true
                        QQC2.Label { text: qsTr("Guardar en:") }
                        QQC2.Label {
                            Layout.maximumWidth: Kirigami.Units.gridUnit * 22
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
                        Item { Layout.fillWidth: true }
                    }
                }

                // La cuenta atras, donde se estaba mirando: justo debajo
                // del boton que se acaba de pulsar.
                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: raiz.cuentaAtras > 0
                    type: Kirigami.MessageType.Information
                    text: qsTr("Empieza en %1…").arg(raiz.cuentaAtras)
                    actions: Kirigami.Action {
                        text: qsTr("Cancelar")
                        icon.name: "dialog-cancel"
                        onTriggered: {
                            relojCuentaAtras.stop()
                            raiz.cuentaAtras = 0
                        }
                    }
                }

                // Lo que quedo a medias la ultima vez. Se ofrece arreglarlo
                // aqui y no en un dialogo al arrancar: un dialogo obliga a
                // decidir antes de saber de que va, y esto puede esperar.
                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: Controlador.grabacionAMedias !== ""
                    type: Kirigami.MessageType.Warning
                    text: qsTr("Una grabación quedó a medias: %1.\nEl vídeo está "
                               + "dentro, pero le falta el cierre y muchos "
                               + "reproductores no lo abrirán tal cual.")
                          .arg(Controlador.grabacionAMedias)
                    actions: [
                        Kirigami.Action {
                            text: qsTr("Arreglarlo")
                            icon.name: "tools-wizard"
                            onTriggered: Controlador.repararGrabacion()
                        },
                        Kirigami.Action {
                            text: qsTr("Dejarlo así")
                            icon.name: "dialog-cancel"
                            onTriggered: Controlador.olvidarGrabacionAMedias()
                        }
                    ]
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
                        : Controlador.estado === "replay" ? qsTr("En memoria, sin guardar")
                        : Controlador.estado === "emitiendo" ? qsTr("Emitiendo en directo")
                        : qsTr("Grabando")
                    opacity: 0.7
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Kirigami.Units.largeSpacing

                    QQC2.Button {
                        // En modo repeticion el boton principal no es parar: es
                        // GUARDAR lo que hay en el buffer, y seguir. Parar ahi
                        // no guarda nada, asi que si fuera lo unico a mano
                        // acabarias tirando lo que querias salvar.
                        visible: Controlador.estado === "replay"
                        icon.name: "document-save"
                        text: qsTr("Guardar lo último")
                        onClicked: Controlador.guardarReplay()
                    }
                    QQC2.Button {
                        // GSR no pausa el modo audio-only: la pausa es del IPC
                        // de la pantalla. En audio y en repeticion no se ofrece.
                        visible: Controlador.estado !== "grabandoAudio"
                                 && Controlador.estado !== "replay"
                                 && Controlador.estado !== "emitiendo"
                        icon.name: Controlador.estado === "pausado"
                                   ? "media-playback-start" : "media-playback-pause"
                        text: Controlador.estado === "pausado" ? qsTr("Reanudar") : qsTr("Pausa")
                        onClicked: Controlador.estado === "pausado"
                                   ? Controlador.reanudar() : Controlador.pausar()
                    }
                    QQC2.Button {
                        icon.name: "media-playback-stop"
                        // En repeticion no se guarda nada al parar, y decir
                        // «Parar y guardar» ahi seria mentir.
                        text: Controlador.estado === "replay"
                                  || Controlador.estado === "emitiendo" ? qsTr("Terminar")
                                                                        : qsTr("Parar y guardar")
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
