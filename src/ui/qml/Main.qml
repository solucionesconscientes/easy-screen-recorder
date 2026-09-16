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
                                     || Controlador.estado === "replay"

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
            if (Controlador.estado === "grabando"
                    || Controlador.estado === "replay") raiz.showMinimized()
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
            bitrateAudio: bitrateAudio.currentValue,
            camara: camaraActiva ? String(camara.currentValue) : "",
            camaraTamano: tamanoCamara.value,
            camaraEsquina: esquinaCamara.currentValue,
            camaraEspejo: espejoCamara.checked,
            modoFotogramas: soloAlCambiar.visible && soloAlCambiar.checked ? "content" : "",
            limiteResolucion: limiteResolucion.currentValue,
            replaySegundos: replay.checked ? segundosReplay.currentValue : 0
        }
        if (region !== "") opciones.region = region
        Controlador.grabar(fuente.currentValue, opciones)
    }

    // La camara superpuesta solo tiene sentido grabando pantalla: sobre una
    // fuente que YA es la camara, o sobre una nota de voz, no pinta nada.
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
                        : qsTr("Grabar")
                    enabled: !ocupado && String(fuente.currentValue) !== ""
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

                    // --- Cámara superpuesta ------------------------------
                    //
                    // GSR la compone EL MISMO, en vivo y en la misma grabacion.
                    // Aqui solo se eligen tamaño y esquina, y se eligen ANTES de
                    // grabar porque despues ya no hay nada que recomponer.
                    QQC2.CheckBox {
                        id: usarCamara
                        visible: raiz.puedeCamara
                        Kirigami.FormData.label: qsTr("Cámara:")
                        Kirigami.FormData.isSection: true
                        text: qsTr("Superponer la cámara sobre la pantalla")
                    }
                    QQC2.ComboBox {
                        id: camara
                        visible: raiz.camaraActiva
                        Kirigami.FormData.label: qsTr("Cuál:")
                        model: Controlador.camaras
                        textRole: "texto"
                        valueRole: "valor"
                    }
                    RowLayout {
                        visible: raiz.camaraActiva
                        Kirigami.FormData.label: qsTr("Tamaño:")
                        QQC2.Slider {
                            id: tamanoCamara
                            // En PORCENTAJE del ancho, no en pixeles: un tamaño
                            // fijo es un cuarto de pantalla en 1366 y un decimo
                            // en 4K. El rango es el que valida el nucleo.
                            from: 5; to: 50; stepSize: 1; value: 25
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                        }
                        QQC2.Label { text: tamanoCamara.value + " %" }
                    }
                    QQC2.ComboBox {
                        id: esquinaCamara
                        visible: raiz.camaraActiva
                        Kirigami.FormData.label: qsTr("Esquina:")
                        model: Controlador.esquinasCamara()
                        textRole: "texto"
                        valueRole: "valor"
                    }
                    QQC2.CheckBox {
                        id: espejoCamara
                        visible: raiz.camaraActiva
                        checked: true
                        text: qsTr("Espejo (te ves como en un espejo)")
                    }
                    // La vista previa, para encuadrarte antes de empezar.
                    //
                    // Va en un Loader porque el fichero solo existe si se
                    // compilo con Qt6Multimedia; sin el, `active` es false y no
                    // se pide nada, asi que no hay import que falle.
                    //
                    // Y se apaga al arrancar la grabacion: la camara admite un
                    // solo cliente, y a partir de ahi es de GSR. Verificado en
                    // esta maquina: un segundo cliente recibe «Device or
                    // resource busy».
                    Loader {
                        Layout.fillWidth: true
                        Kirigami.FormData.label: qsTr("Encuadre:")
                        visible: raiz.camaraActiva && Controlador.hayMultimedia
                        active: visible
                        source: "VistaPreviaCamara.qml"
                        onLoaded: {
                            item.dispositivo = Qt.binding(function() {
                                return String(camara.currentValue)
                            })
                            item.espejo = Qt.binding(function() { return espejoCamara.checked })
                            item.activa = Qt.binding(function() {
                                return Controlador.estado === "listo"
                            })
                        }
                    }

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
                    QQC2.ComboBox {
                        id: limiteResolucion
                        visible: !fuente.esAudio
                        Kirigami.FormData.label: qsTr("Tamaño del vídeo:")
                        textRole: "texto"
                        valueRole: "valor"
                        // GSR escala para caber dentro, respetando la
                        // proporcion: de ahi que sea un limite y no una medida.
                        model: [
                            { texto: qsTr("El de la pantalla"), valor: "" },
                            { texto: qsTr("Como mucho 1080p"), valor: "1920x1080" },
                            { texto: qsTr("Como mucho 720p"), valor: "1280x720" }
                        ]
                        currentIndex: 0
                    }
                    QQC2.CheckBox {
                        id: soloAlCambiar
                        // Solo se enseña donde de verdad hace algo. GSR acepta
                        // «-fm content» siempre y, cuando no puede aplicarlo, lo
                        // dice por stderr y sigue igual: en Wayland sobre un
                        // monitor no hace nada. Ofrecerlo ahi seria prometer un
                        // ahorro que no llega.
                        visible: !fuente.esAudio
                                 && Controlador.modoContentEfectivo(String(fuente.currentValue))
                        text: qsTr("Codificar solo cuando la pantalla cambie")
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
                        // Las aplicaciones que suenan AHORA se añaden al
                        // final: son las que GSR ve en este momento, asi que la
                        // lista cambia de una grabacion a otra. Grabar solo el
                        // sonido de una aplicacion deja fuera notificaciones y
                        // todo lo demas, que es justo lo que se quiere en un
                        // tutorial.
                        model: [
                            { texto: qsTr("Audio del sistema"), valor: "sistema" },
                            { texto: qsTr("Micrófono"), valor: "micro" },
                            { texto: qsTr("Los dos, en una sola pista"), valor: "mezclado" },
                            { texto: qsTr("Los dos, en pistas separadas (para editar)"), valor: "ambos" },
                            { texto: qsTr("Sin audio"), valor: "nada" }
                        ].concat(Controlador.audiosAplicacion.map(function(a) {
                            return { texto: qsTr("Solo %1").arg(a.texto), valor: a.valor }
                        }))
                        currentIndex: 0
                    }

                    // El vumetro del microfono. Se enseña solo cuando se va a
                    // grabar el micro, que es cuando importa: el fallo caro es
                    // descubrir al reproducir que estaba mudo.
                    RowLayout {
                        id: filaVumetro
                        visible: Controlador.hayMultimedia && !fuente.esAudio
                                 && ["micro", "mezclado", "ambos"].indexOf(
                                        String(audio.currentValue)) !== -1
                        Kirigami.FormData.label: qsTr("Nivel del micro:")
                        onVisibleChanged: Controlador.escucharMicro(visible)
                        Component.onCompleted: Controlador.escucharMicro(visible)
                        QQC2.ProgressBar {
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                            from: 0; to: 1
                            // Escala en DECIBELIOS, de -60 dB a 0, que es como
                            // mide un vumetro. Con la escala lineal la barra no
                            // se movia: una voz normal en este microfono da
                            // 0,004-0,05 de RMS, o sea el 0,4 % del recorrido.
                            // En dB ese mismo margen ocupa del 20 % al 57 %.
                            value: {
                                var n = Controlador.nivelMicro
                                if (n <= 0.0001) return 0
                                var db = 20 * Math.log(n) / Math.LN10
                                return Math.max(0, Math.min(1, (db + 60) / 60))
                            }
                        }
                        QQC2.Label {
                            // El umbral distingue «entra algo» de «mudo», no
                            // «se oye bien»: con el microfono al 27 % una voz
                            // normal ya se queda por debajo de 0,01.
                            text: Controlador.nivelMicro > 0.002 ? qsTr("te oigo")
                                                                 : qsTr("sin señal")
                            opacity: 0.7
                        }
                    }

                    // --- Cómo se graba ----------------------------------
                    QQC2.CheckBox {
                        id: cuentaAtrasActiva
                        Kirigami.FormData.label: qsTr("Al empezar:")
                        Kirigami.FormData.isSection: true
                        checked: true
                        text: qsTr("Contar 3 segundos antes de grabar")
                    }
                    QQC2.CheckBox {
                        id: replay
                        visible: !fuente.esAudio
                        // El replay no escribe nada hasta que se lo pides: va
                        // guardando en memoria los ultimos N segundos. Sirve
                        // para lo que ya ha pasado, que es cuando uno se
                        // acuerda de que queria grabarlo.
                        text: qsTr("Modo repetición: guardar solo cuando yo lo pida")
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
                        text: Controlador.estado === "replay" ? qsTr("Terminar")
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
