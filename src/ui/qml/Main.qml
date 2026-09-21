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
    // Nace con el ancho de las dos columnas y con el alto que pida el contenido:
    // ajustarAltura lo sube nada mas abrir, hasta donde de la pantalla.
    width: Math.min(anchoFormulario, Screen.desktopAvailableWidth)
    height: Math.max(minimumHeight, altoInicial)
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
            // Esto es la RED, no el mecanismo. Apartarse ocurre antes de
            // arrancar (ver apartarseYArrancar), porque minimizarse al llegar
            // el estado «grabando» era minimizarse cuando GSR ya estaba
            // capturando: se grababa la animacion de minimizado y, en frio,
            // hasta los 15 s que el grabador puede tardar en abrir su socket
            // (grabacion.cpp, kEsperaSocketMs). Se deja porque es idempotente
            // y cubre cualquier camino que no pase por ahi.
            if (Controlador.estado === "grabando" || Controlador.estado === "replay"
                    || Controlador.estado === "emitiendo") raiz.showMinimized()
            else if (Controlador.estado === "listo" && raiz.apartada) raiz.volver()
        }
        function onPideGrabarPantalla(fuente) {
            raiz.apartarseYArrancar(function() { Controlador.grabar(fuente, {}) })
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
    readonly property int altoInicial: Kirigami.Units.gridUnit * 21
    // El ancho de las dos columnas con el mapa de la camara. En 24 unidades de
    // rejilla, que era el ancho de la ventana plegada, las columnas se aprietan
    // y las filas vuelven a no caber.
    // 46 se quedo corto al poner una «i» en cada fila: la columna de la
    // izquierda volvia a cortar «Imágenes por segundo:», que es exactamente el
    // fallo que ya obligo a no igualar el ancho de las dos columnas.
    readonly property int anchoFormulario: Kirigami.Units.gridUnit * 51
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
        // Se mide en el flickable y no en la columna: dentro de una pagina
        // desplazable la columna ya mide lo que ocupa su contenido, asi que
        // restarle su propio alto daria cero siempre y la ventana no creceria.
        var f = pagina.flickable
        if (!f) return
        var falta = f.contentHeight - f.height
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
            codecVideo: String(codecVideo.currentValue),
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
            replaySegundos: replay.checked ? segundosReplay.currentValue : 0,
            reducir: String(reducirAlTerminar.currentValue)
        }
        if (region !== "") opciones.region = region
        if (raiz.emitiendo) {
            opciones.guardarEmision = guardarEmision.checked
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
    // Publicada para que la bandeja la pinte, que es donde se ve ahora.
    onCuentaAtrasChanged: Controlador.cuentaAtras = raiz.cuentaAtras
    Connections {
        target: Controlador
        function onCancelarCuentaAtras() {
            relojCuentaAtras.stop()
            raiz.cuentaAtras = 0
        }
    }
    property string regionPendiente: ""
    Timer {
        id: relojCuentaAtras
        interval: 1000
        repeat: true
        onTriggered: {
            raiz.cuentaAtras -= 1
            if (raiz.cuentaAtras <= 0) {
                stop()
                // La ventana ya esta apartada desde antes de empezar a contar,
                // asi que aqui solo queda grabar.
                raiz.lanzarGrabacion(raiz.regionPendiente)
            }
        }
    }

    // Apartarse y arrancar, el unico camino por el que se empieza a grabar.
    //
    // Se espera la señal de que la ventana se fue y ADEMAS un margen: en
    // Wayland `visibility` dice que el
    // compositor acepto el cambio de estado, no que haya terminado de dibujar
    // su animacion. El margen esta dimensionado por arriba a proposito, porque
    // lo unico que cuesta es empezar un tercio de segundo mas tarde; la
    // duracion real de la animacion de KWin quedo SIN MEDIR.
    //
    // El tope existe porque la señal puede no llegar nunca en un escritorio que
    // no minimice. Sin el, el boton se quedaria muerto para siempre.
    property var arranquePendiente: null
    Timer { id: margenApartarse; interval: 350; onTriggered: raiz.arrancarYa() }
    Timer { id: topeApartarse; interval: 900; onTriggered: raiz.arrancarYa() }
    function arrancarYa() {
        if (!raiz.arranquePendiente) return
        margenApartarse.stop()
        topeApartarse.stop()
        var accion = raiz.arranquePendiente
        raiz.arranquePendiente = null
        accion()
    }
    function apartarseYArrancar(accion) {
        // Gana la primera peticion. Durante la espera el estado sigue siendo
        // «listo», asi que el boton no se deshabilita solo, y dos arranques
        // pedidos a la vez —volver de la barra de tareas y pulsar otra vez—
        // lanzarian dos grabaciones.
        if (raiz.arranquePendiente) return
        if (raiz.apartada) { accion(); return }
        raiz.arranquePendiente = accion
        topeApartarse.start()
        raiz.showMinimized()
    }
    onVisibilityChanged: {
        if (raiz.arranquePendiente && raiz.apartada) margenApartarse.start()
    }

    function empezarCon(region) {
        raiz.regionPendiente = region
        if (cuentaAtrasActiva.checked) {
            // La ventana se aparta YA y la cuenta se ve en la bandeja.
            //
            // Antes se contaba con la ventana delante y solo despues se
            // apartaba, porque minimizar a mitad de cuenta dejaba el ultimo
            // segundo a ciegas. Eso deja de ser cierto en cuanto la bandeja
            // enseña el numero: ahora se ve igual, y ademas los tres segundos
            // sirven para lo que sirven, para que la ventana ya no este.
            raiz.apartarseYArrancar(function() {
                raiz.cuentaAtras = 3
                relojCuentaAtras.start()
            })
        } else {
            raiz.apartarseYArrancar(function() { raiz.lanzarGrabacion(region) })
        }
    }

    // El texto de cada codec.
    //
    // El identificador es el de GSR y no se toca NUNCA: es lo que viaja en -k.
    // Lo que se le añade al lado es lo unico que decide la eleccion, que es
    // siempre lo mismo: tamaño contra compatibilidad. Un identificador que no
    // conozcamos sale tal cual, sin inventarle una descripcion, porque la lista
    // la da la maquina y GSR puede añadir nombres nuevos cuando quiera.
    // --- Recordar lo elegido, para no tener que volver a elegirlo -------
    //
    // Lo que se guarda es lo que la persona ELIGE, no lo que el programa
    // decide: por eso los desplegables lo apuntan en «onActivated», que solo se
    // dispara con un clic, y no en «currentIndexChanged», que salta tambien
    // cuando el modelo cambia por detras.
    function recordar(clave, valor) {
        Controlador.recordarAjuste(clave, String(valor))
    }
    // Se llama al cambiar el modelo y no solo al arrancar: las listas de
    // fuentes y de codecs llegan despues de la deteccion, asi que restaurar al
    // arrancar se encontraria una lista vacia.
    function restaurarCombo(combo, clave) {
        var v = Controlador.ajusteRecordado(clave, "")
        if (v === "") return
        var i = combo.indexOfValue(v)
        if (i >= 0) combo.currentIndex = i
    }
    function restaurarCasilla(casilla, clave) {
        casilla.checked = Controlador.ajusteRecordado(clave, "") === "1"
    }

    function etiquetaCodec(id) {
        var notas = {
            "h264": qsTr("lo reproduce todo, hasta un televisor viejo"),
            "hevc": Controlador.hevcPocoFiable
                    // Ya no es un «puede»: esta maquina lo ha demostrado al
                    // grabar, y decirlo flojito seria dejar que tropiece otra vez.
                    ? qsTr("tu tarjeta lo hace mal: sale más grande y peor que h264")
                    : qsTr("según la tarjeta puede salir peor que h264; mira la «i»"),
            "av1": qsTr("sin patentes; hace falta un equipo reciente para verlo"),
            "vp9": qsTr("sin patentes, pensado para la web; fuera del navegador, irregular"),
            "vp8": qsTr("el veterano de .webm; solo si necesitas ese formato"),
            "hevc_hdr": qsTr("hevc con HDR; el reproductor tiene que entenderlo"),
            "av1_hdr": qsTr("av1 con HDR; el reproductor tiene que entenderlo"),
            "hevc_10bit": qsTr("hevc a 10 bits: menos bandas en los degradados"),
            "av1_10bit": qsTr("av1 a 10 bits: menos bandas en los degradados")
        }
        var nota = notas[id]
        if (nota === undefined && id.indexOf("vulkan") !== -1) nota = qsTr("experimental")
        return nota === undefined ? id : id + " · " + nota
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
        raiz.ajustarAltura()
        if (Qt.application.arguments.indexOf("--selector") !== -1) {
            selectorRegion.abrir()
        }
    }

    // Desplazable, y no por capricho: con el formulario siempre desplegado, el
    // alto del contenido depende del tipo de letra, del panel y de la pantalla
    // de cada uno. Sin barra, las ultimas filas se salen y NO hay forma de
    // llegar a ellas, que es un fallo que este formulario ya tuvo una vez.
    pageStack.initialPage: Kirigami.ScrollablePage {
        id: pagina
        padding: Kirigami.Units.largeSpacing * 2

        // La ventana crece cuando cambia lo que hay que enseñar, y se entera POR
        // EL FLICKABLE. Colgarlo del arranque no basta: cuando la ventana acaba
        // de construirse, la pagina todavia tiene un flickable vacio de relleno
        // (asi lo dice el propio Kirigami), asi que la primera medida sale cero
        // y ya no vuelve a haber otra. Resultado: ventana corta y barra de
        // desplazamiento con media pantalla libre al lado.
        Connections {
            target: pagina.flickable
            function onContentHeightChanged() { raiz.ajustarAltura() }
        }

        ColumnLayout {
            id: columna
            // Dentro de una ScrollablePage el ancho lo pone uno mismo (asi lo
            // documenta Kirigami) y el alto manda sobre el desplazamiento. El
            // minimo es el hueco visible para que los mensajes centrados sigan
            // centrados cuando sobra sitio.
            width: pagina.width - pagina.leftPadding - pagina.rightPadding
            height: Math.max(implicitHeight, pagina.flickable ? pagina.flickable.height : 0)
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
                    onActivated: raiz.recordar("fuente", currentValue)
                    onModelChanged: Qt.callLater(function() {
                        raiz.restaurarCombo(fuente, "fuente")
                    })
                    Component.onCompleted: raiz.restaurarCombo(fuente, "fuente")
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

                // Las opciones, SIN plegar.
                //
                // Estuvieron detras de un boton «Avanzado» hasta la 0.9.0, por
                // contrato: dos clics para grabar y lo demas escondido. Se quita
                // porque escondido no se encontraba: la primera opcion que se
                // busco de verdad se busco aqui y no estaba. Fuente y Grabar
                // siguen los primeros y siguen siendo dos clics; lo que cambia es
                // que lo demas ya no hay que descubrirlo.
                //
                // En DOS COLUMNAS.
                //
                // Era una sola y no cabia: con nueve funciones nuevas las
                // ultimas filas quedaban fuera de la ventana y, a la vez,
                // sobraba la mitad derecha, porque un formulario de Kirigami
                // pone la etiqueta a la izquierda y el control en medio. Dos
                // columnas usan el hueco que ya estaba ahi.
                ColumnLayout {
                    id: formulario
                    Layout.fillWidth: true
                    enabled: !ocupado
                    spacing: Kirigami.Units.largeSpacing
                    onImplicitHeightChanged: raiz.ajustarAltura()

                    // --- Emitir en directo ---------------------------------
                    //
                    // Va arriba y a lo ancho porque es un MODO: mientras esta
                    // puesto, no se graba a fichero, no hay pausa y la calidad
                    // es un bitrate. Mezclarlo entre los ajustes de vídeo habria
                    // hecho que se activara sin querer.
                    RowLayout {
                        visible: !fuente.esAudio
                        QQC2.CheckBox {
                            id: emitirEnDirecto
                            // Decia «en vez de guardar un fichero», y desde que se
                            // puede guardar la emision eso es falso.
                            text: qsTr("Emitir en directo")
                        }
                        Kirigami.ContextualHelpButton {
                            Layout.alignment: Qt.AlignVCenter
                            toolTipText: qsTr(
                                "Manda la pantalla a YouTube, Twitch o cualquier servidor RTMP." +
                                "\n\nNo se puede pausar: dejar de mandar imagen hace que la " +
                                "plataforma dé la emisión por caída.")
                        }
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
                            // RTMPS y no RTMP: es el que YouTube recomienda, y
                            // una clave de emision por un canal sin cifrar es
                            // una credencial viajando en claro.
                            placeholderText: "rtmps://a.rtmps.youtube.com/live2"
                        }
                        QQC2.Label { text: qsTr("Bitrate:") }
                        RowLayout {
                            QQC2.ComboBox {
                                id: bitrateEmision
                                onActivated: raiz.recordar("bitrate", currentValue)
                                onModelChanged: Qt.callLater(function() {
                                    raiz.restaurarCombo(bitrateEmision, "bitrate")
                                })
                                Component.onCompleted: raiz.restaurarCombo(bitrateEmision, "bitrate")
                                textRole: "texto"
                                valueRole: "valor"
                                // Los valores que recomienda YouTube, tal cual, con
                                // su fuente en docs/emision.md. Emitiendo no hay
                                // «calidad»: hay un caudal, y lo fija la plataforma.
                                //
                                // Mis primeras cifras (4500 para 1080p) estaban MAL,
                                // sacadas de memoria. YouTube pide 10 Mbps para
                                // 1080p30 y 12 para 1080p60.
                                model: [
                                    { texto: qsTr("Hasta 720p, 30 fps · 4 Mbps"), valor: 4000 },
                                    { texto: qsTr("1080p, 30 fps · 10 Mbps"), valor: 10000 },
                                    { texto: qsTr("1080p, 60 fps · 12 Mbps"), valor: 12000 },
                                    { texto: qsTr("1440p, 30 fps · 15 Mbps"), valor: 15000 }
                                ]
                                // Se preselecciona segun la pantalla y las imagenes
                                // por segundo que haya puestas: ofrecer 10 Mbps a
                                // quien graba una pantalla de 768 px es gastarle la
                                // subida para nada.
                                currentIndex: {
                                    var alto = Screen.height
                                    var sesenta = fps.currentText === "60"
                                    if (alto > 1080) return 3
                                    if (alto > 720) return sesenta ? 2 : 1
                                    return 0
                                }
                            }
                            Kirigami.ContextualHelpButton {
                                Layout.alignment: Qt.AlignVCenter
                                toolTipText: qsTr(
                                    "Son los valores que recomienda YouTube. Necesitas subida " +
                                    "por encima de la cifra: si tu conexión no da, la emisión se " +
                                    "corta a trozos.")
                            }
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
                    RowLayout {
                        visible: raiz.emitiendo
                        Layout.fillWidth: true
                        QQC2.CheckBox {
                            id: guardarEmision
                            onToggled: raiz.recordar("guardarEmision", checked ? "1" : "0")
                            Component.onCompleted: raiz.restaurarCasilla(guardarEmision, "guardarEmision")
                            text: qsTr("Guardar también la emisión en un fichero")
                            // MARCADA. Lo que sale por el cable no se puede
                            // recuperar, y un fichero que sobra se borra; al reves
                            // no hay arreglo. Medido: no cuesta CPU ni RAM, porque
                            // el mismo grabador escribe las dos salidas. Lo que
                            // cuesta es disco, y por eso se dice aqui debajo cuanto.
                            checked: true
                        }
                        Kirigami.ContextualHelpButton {
                            Layout.alignment: Qt.AlignVCenter
                            toolTipText: qsTr(
                                "Lo escribe el mismo grabador mientras emite, así que no cuesta " +
                                "una segunda codificación ni el doble de GPU. Va en .flv, que es " +
                                "el contenedor de la emisión. Al parar se dice dónde quedó.")
                        }
                    }
                    QQC2.Label {
                        // Viniendo marcada, donde escribe y cuanto ocupa deja de
                        // ser un detalle: son gigas en el disco de alguien que no
                        // los ha pedido uno por uno.
                        visible: raiz.emitiendo && guardarEmision.checked
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Se guardará en %1, y ocupa el caudal que elijas: %2 aprox.")
                              .arg(Controlador.carpetaVideos)
                              .arg(qsTr("%1 GB por hora")
                                   .arg((parseInt(bitrateEmision.currentValue) * 3600 / 8 / 1000000)
                                        .toFixed(1)))
                        opacity: 0.7
                        font: Kirigami.Theme.smallFont
                    }

                    // --- Camara, a lo ancho: el mapa necesita sitio ---------
                    RowLayout {
                        visible: raiz.puedeCamara
                        QQC2.CheckBox {
                            id: usarCamara
                            onToggled: raiz.recordar("usarCamara", checked ? "1" : "0")
                            Component.onCompleted: raiz.restaurarCasilla(usarCamara, "usarCamara")
                            text: qsTr("Superponer la cámara sobre la pantalla")
                        }
                        Kirigami.ContextualHelpButton {
                            Layout.alignment: Qt.AlignVCenter
                            toolTipText: qsTr(
                                "Pantalla y cámara en la misma grabación y el mismo " +
                                "fichero, sin nada que montar después.\n\nLa vista " +
                                "previa se apaga al empezar a grabar: una cámara solo " +
                                "admite un programa a la vez, y a partir de ahí es " +
                                "del grabador. Por eso el encuadre se elige antes.")
                        }
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
                            onActivated: raiz.recordar("camara", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(camara, "camara")
                            })
                            Component.onCompleted: raiz.restaurarCombo(camara, "camara")
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
                            onToggled: raiz.recordar("espejoCamara", checked ? "1" : "0")
                            Component.onCompleted: raiz.restaurarCasilla(espejoCamara, "espejoCamara")
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
                                onActivated: raiz.recordar("formato", currentValue)
                                onModelChanged: Qt.callLater(function() {
                                    raiz.restaurarCombo(contenedor, "formato")
                                })
                                Component.onCompleted: raiz.restaurarCombo(contenedor, "formato")
                                // Emitiendo el contenedor es flv y el codec aac:
                                // no hay nada que elegir, asi que no se enseña.
                                visible: !fuente.esAudio && !raiz.emitiendo
                                Kirigami.FormData.label: qsTr("Formato:")
                                model: Controlador.contenedores
                            }
                            RowLayout {
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Códec de vídeo:")
                                QQC2.ComboBox {
                                    id: codecVideo
                                    onActivated: raiz.recordar("codecVideo", currentValue)
                                    Component.onCompleted: raiz.restaurarCombo(codecVideo, "codecVideo")
                                    textRole: "texto"
                                    valueRole: "valor"
                                    // Lo que la maquina soporta Y cabe en el
                                    // formato: sin el segundo filtro se podia
                                    // pedir h264 en un .webm, y eso no graba nada.
                                    readonly property var disponibles:
                                        Controlador.codecsVideoPara(contenedor.currentText,
                                                                    Controlador.codecsVideo)
                                    model: disponibles.map(function(c) {
                                        return { texto: raiz.etiquetaCodec(c), valor: c }
                                    })
                                    // h264 cuando esta, que es lo que elegiria
                                    // GSR solo y lo que abre cualquier
                                    // reproductor. Si no esta, el primero que
                                    // haya: la lista la manda la maquina.
                                    onModelChanged: {
                                        var i = disponibles.indexOf("h264")
                                        currentIndex = i >= 0 ? i : 0
                                        // Y si esta persona ya eligio otro, manda
                                        // el suyo: el default solo cubre la
                                        // primera vez.
                                        Qt.callLater(function() {
                                            raiz.restaurarCombo(codecVideo, "codecVideo")
                                        })
                                    }
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Solo salen los que esta máquina puede codificar por hardware y " +
                                        "caben en el formato elegido; la lista se detecta al arrancar." +
                                        "\n\nh264 es el seguro: lo abre cualquier móvil, ordenador, web " +
                                        "o televisor.\n\nDe hevc se dice que ocupa la mitad a igual " +
                                        "calidad, y es verdad solo si el driver de tu tarjeta lo maneja " +
                                        "bien. Medido en la máquina donde se probó esto, grabando lo " +
                                        "mismo y comparándolo con el original: h264 salió más pequeño Y " +
                                        "más fiel en los cuatro niveles de calidad, porque ese driver no " +
                                        "declara lo que su codificador HEVC sabe hacer y se conduce a " +
                                        "ojo.\n\nSi no sabes cómo va el tuyo, quédate con h264. vp8 y " +
                                        "vp9, solo si necesitas .webm.")
                                }
                            }
                            RowLayout {
                                visible: !fuente.esAudio && !raiz.emitiendo
                                Kirigami.FormData.label: qsTr("Calidad:")
                                QQC2.ComboBox {
                                    id: calidad
                                    onActivated: raiz.recordar("calidad", currentValue)
                                    onModelChanged: Qt.callLater(function() {
                                        raiz.restaurarCombo(calidad, "calidad")
                                    })
                                    Component.onCompleted: raiz.restaurarCombo(calidad, "calidad")
                                    // Emitiendo se fija un bitrate, no una calidad.
                                    textRole: "texto"
                                    valueRole: "valor"
                                    // SIN cifras de tamaño, y no por pereza: se pusieron medidas y hubo
                                    // que quitarlas. Una cifra aqui depende de tres cosas a la vez: lo
                                    // que se mueva en pantalla, el codec elegido y la GPU. La misma
                                    // etiqueta era verdad con h264 y mentira por casi el doble con hevc,
                                    // y una cifra que falla segun donde la mires no es una cifra.
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
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Se graba a calidad constante: el grabador gasta lo que haga " +
                                        "falta para mantener ese nivel.\n\nPor eso no se anuncia un " +
                                        "tamaño. Con la pantalla quieta casi no ocupa; con vídeo o " +
                                        "desplazamiento sube bastante, y además cambia según el códec." +
                                        "\n\n«Muy alta» es el punto de partida y para grabar la " +
                                        "pantalla sobra.")
                                }
                            }
                            QQC2.ComboBox {
                                id: fps
                                onActivated: raiz.recordar("fps", currentValue)
                                onModelChanged: Qt.callLater(function() {
                                    raiz.restaurarCombo(fps, "fps")
                                })
                                Component.onCompleted: raiz.restaurarCombo(fps, "fps")
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Imágenes por segundo:")
                                model: ["30", "60"]
                                currentIndex: 1
                            }
                            RowLayout {
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Resolución máxima:")
                                QQC2.ComboBox {
                                    id: limiteResolucion
                                    onActivated: raiz.recordar("resolucion", currentValue)
                                    onModelChanged: Qt.callLater(function() {
                                        raiz.restaurarCombo(limiteResolucion, "resolucion")
                                    })
                                    Component.onCompleted: raiz.restaurarCombo(limiteResolucion, "resolucion")
                                    textRole: "texto"
                                    valueRole: "valor"
                                    // Solo los techos que esta pantalla puede usar.
                                    // Ofrecer «como mucho 1080p» en una pantalla de
                                    // 768 px es ofrecer algo que no hace nada, y el
                                    // proyecto no enseña lo que la maquina no usa.
                                    model: {
                                        var m = [{ texto: qsTr("La de la pantalla (%1×%2)")
                                                            .arg(Screen.width).arg(Screen.height),
                                                   valor: "" }]
                                        if (Screen.height > 1080) {
                                            m.push({ texto: qsTr("Como mucho 1080p"),
                                                     valor: "1920x1080" })
                                        }
                                        if (Screen.height > 720) {
                                            m.push({ texto: qsTr("Como mucho 720p"),
                                                     valor: "1280x720" })
                                        }
                                        return m
                                    }
                                    currentIndex: 0
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Achica la imagen para que quepa dentro de ese tamaño, " +
                                        "manteniendo la proporción. No recorta: se sigue grabando " +
                                        "lo mismo, más pequeño y ocupando menos.")
                                }
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

                            RowLayout {
                                visible: !fuente.esAudio
                                Kirigami.FormData.label: qsTr("Audio:")
                                QQC2.ComboBox {
                                    id: audio
                                    onActivated: raiz.recordar("audio", currentValue)
                                    onModelChanged: Qt.callLater(function() {
                                        raiz.restaurarCombo(audio, "audio")
                                    })
                                    Component.onCompleted: raiz.restaurarCombo(audio, "audio")
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
                                    // Por defecto, los dos mezclados: quien graba la
                                    // pantalla casi siempre se esta explicando
                                    // encima, y descubrir al acabar que no habia voz
                                    // no tiene arreglo. Si no hay microfono de
                                    // verdad, solo el sistema: pedir una entrada que
                                    // no existe es un error de arranque regalado.
                                    currentIndex: Controlador.hayMicrofono ? 2 : 0
                                    // Al desplegarlo se vuelve a preguntar que esta
                                    // sonando. La lista calculada al arrancar casi
                                    // nunca sirve: entre abrir esto y darle a grabar,
                                    // el usuario abre justo lo que queria grabar.
                                    onPressedChanged: if (pressed) {
                                        Controlador.refrescarAplicacionesSonando()
                                    }
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "«En una sola pista» mezcla los dos y se oye " +
                                        "todo en cualquier reproductor.\n\n«En pistas " +
                                        "separadas» deja cada uno por su lado para " +
                                        "poder equilibrarlos al editar, pero casi " +
                                        "todos los reproductores suenan solo la " +
                                        "primera: el micrófono parecerá mudo.")
                                }
                            }
                            QQC2.ComboBox {
                                id: codecAudio
                                onActivated: raiz.recordar("codecAudio", currentValue)
                                Component.onCompleted: raiz.restaurarCombo(codecAudio, "codecAudio")
                                visible: !fuente.esAudio && !raiz.emitiendo
                                Kirigami.FormData.label: qsTr("Códec de audio:")
                                // Solo los que GSR respeta en ese formato y en
                                // ese reparto de pistas.
                                model: Controlador.codecsAudioPara(
                                           contenedor.currentText,
                                           String(audio.currentValue) === "mezclado")
                                onModelChanged: {
                                    currentIndex = 0
                                    Qt.callLater(function() {
                                        raiz.restaurarCombo(codecAudio, "codecAudio")
                                    })
                                }
                            }
                            RowLayout {
                                id: filaVumetro
                                visible: Controlador.hayMultimedia && !fuente.esAudio
                                         && ["micro", "mezclado", "ambos"].indexOf(
                                                String(audio.currentValue)) !== -1
                                Kirigami.FormData.label: qsTr("Nivel del micro:")
                                // El micro se abre cuando alguien lo PIDE, no al
                                // abrir la ventana.
                                //
                                // Antes esta fila vivia detras del plegado, asi
                                // que escuchar al hacerse visible significaba
                                // «cuando abres Avanzado». Con el formulario
                                // siempre a la vista, eso paso a significar «al
                                // arrancar»: la aplicacion encenderia el piloto
                                // del microfono nada mas abrirla, sin que nadie
                                // haya pedido grabar todavia. Y cuesta arranque.
                                readonly property bool escuchando: probarMicro.checked
                                                                   && visible
                                onEscuchandoChanged: Controlador.escucharMicro(escuchando)
                                QQC2.Button {
                                    id: probarMicro
                                    flat: true
                                    checkable: true
                                    icon.name: "audio-input-microphone"
                                    text: qsTr("Probar")
                                }
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
                                    text: !probarMicro.checked ? ""
                                          : Controlador.nivelMicro > 0.002 ? qsTr("te oigo")
                                                                           : qsTr("sin señal")
                                    opacity: 0.7
                                }
                            }
                            RowLayout {
                                visible: Controlador.hayAtajos
                                Kirigami.FormData.label: qsTr("Atajos:")
                                QQC2.Label {
                                    // Los atajos existian desde hace tandas y la
                                    // aplicacion no los mencionaba en ningun sitio,
                                    // asi que para el usuario no existian. Un atajo
                                    // que no se anuncia es codigo muerto.
                                    text: qsTr("%1 graba y para").arg(Controlador.atajoGrabar)
                                          + (Controlador.atajoPausa !== ""
                                             ? "\n" + qsTr("%1 pausa y reanuda")
                                                       .arg(Controlador.atajoPausa)
                                             : "")
                                    opacity: 0.8
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Funcionan con la ventana cerrada o minimizada, que es "
                                        + "para lo que sirven: pausar sin que la ventana salga "
                                        + "en el vídeo.\n\nSe pueden cambiar en Preferencias del "
                                        + "sistema, en Atajos de teclado.")
                                }
                            }
                            QQC2.CheckBox {
                                id: cuentaAtrasActiva
                                onToggled: raiz.recordar("cuentaAtras", checked ? "1" : "0")
                                Component.onCompleted: raiz.restaurarCasilla(cuentaAtrasActiva, "cuentaAtras")
                                Kirigami.FormData.label: qsTr("Al empezar:")
                                // Desactivada por defecto: lo que se espera al
                                // dar a Grabar es que grabe, y una espera de
                                // tres segundos que nadie pidio deja dudando
                                // cuando empieza de verdad. Quien la quiera
                                // —para quitar el raton o colocarse delante de
                                // la camara— la enciende, y ahi si cuenta.
                                checked: false
                                text: qsTr("Contar 3 segundos")
                            }
                            RowLayout {
                                RowLayout {
                                    // Solo donde hay algo que recomprimir: una nota de voz
                                    // ya viene comprimida.
                                    visible: !fuente.esAudio
                                    Kirigami.FormData.label: qsTr("Al terminar:")
                                    QQC2.ComboBox {
                                        id: reducirAlTerminar
                                        onActivated: raiz.recordar("reducir", currentValue)
                                        onModelChanged: Qt.callLater(function() {
                                            raiz.restaurarCombo(reducirAlTerminar, "reducir")
                                        })
                                        Component.onCompleted: raiz.restaurarCombo(reducirAlTerminar, "reducir")
                                        textRole: "texto"
                                        valueRole: "valor"
                                        model: [
                                            { texto: qsTr("Dejar la grabación como está"), valor: "" },
                                            { texto: qsTr("Reducir el tamaño"), valor: "normal" },
                                            { texto: qsTr("Reducir al máximo"), valor: "maximo" }
                                        ]
                                        currentIndex: 0
                                    }
                                    Kirigami.ContextualHelpButton {
                                        Layout.alignment: Qt.AlignVCenter
                                        // SIN prometer un porcentaje: cuanto encoge depende de
                                        // lo bueno que sea el codificador de cada tarjeta. Lo
                                        // que se promete es el dato real al terminar, y lo que
                                        // le paso A ESTA maquina lo dice la linea de abajo.
                                        toolTipText: qsTr(
                                            "Al acabar de grabar, la vuelve a comprimir con el mismo códec " +
                                            "pero por procesador, que se toma su tiempo y encuentra lo que la " +
                                            "tarjeta gráfica no tuvo tiempo de buscar. No cambia de formato: " +
                                            "lo que se abría antes se sigue abriendo.\n\nTarda aproximadamente " +
                                            "lo que dure el vídeo, y puedes seguir usando el programa " +
                                            "mientras.\n\n«Al máximo» aprieta más y pierde algo de calidad, " +
                                            "poca pero real.")
                                    }
                                }
                                QQC2.Label {
                                    // La cifra DEL NIVEL ELEGIDO: «al máximo» deja
                                    // el fichero bastante más pequeño, y enseñar
                                    // la del otro nivel sería una cifra que no es.
                                    readonly property int medido:
                                        String(reducirAlTerminar.currentValue) === "maximo"
                                        ? Controlador.reduccionMaximo
                                        : Controlador.reduccionNormal
                                    visible: medido > 0
                                             && String(reducirAlTerminar.currentValue) !== ""
                                    text: qsTr("La última vez quedó en el %1 % de su tamaño")
                                          .arg(medido)
                                    opacity: 0.7
                                    font: Kirigami.Theme.smallFont
                                }
                                QQC2.CheckBox {
                                    id: relojEnBandeja
                                    // Sin etiqueta «Bandeja:» al lado: el propio texto
                                    // ya dice donde sale, y decirlo dos veces sobra.
                                    // La MISMA opcion que la del menu de la bandeja,
                                    // no una copia: las dos escriben la preferencia
                                    // del controlador. Nacio solo en aquel menu y el
                                    // primero que la busco la busco aqui.
                                    checked: Controlador.relojEnBandeja
                                    onToggled: {
                                        Controlador.relojEnBandeja = checked
                                        // Y se devuelve la atadura: escribir
                                        // «checked» a mano la rompe, y sin ella
                                        // marcarla desde la bandeja dejaria esta
                                        // casilla diciendo lo contrario.
                                        checked = Qt.binding(function() {
                                            return Controlador.relojEnBandeja
                                        })
                                    }
                                    text: qsTr("Mostrar el tiempo de grabación en la bandeja")
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Mientras grabas, la ventana se aparta para no salir en el "
                                        + "vídeo. Con esto, el tiempo se ve en un segundo icono al "
                                        + "lado del de la bandeja.")
                                }
                            }
                            RowLayout {
                                // Solo donde de verdad hace algo: en Wayland
                                // sobre un monitor GSR lo acepta, avisa por
                                // stderr y lo ignora.
                                visible: !fuente.esAudio
                                         && Controlador.modoContentEfectivo(
                                                String(fuente.currentValue))
                                QQC2.CheckBox {
                                    id: soloAlCambiar
                                    onToggled: raiz.recordar("soloAlCambiar", checked ? "1" : "0")
                                    Component.onCompleted: raiz.restaurarCasilla(soloAlCambiar, "soloAlCambiar")
                                    text: qsTr("Codificar solo al cambiar la pantalla")
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Mientras la pantalla esté quieta no gasta " +
                                        "GPU ni ocupa sitio. Va bien en un tutorial " +
                                        "con pausas.\n\nSolo aparece donde funciona " +
                                        "de verdad; en Wayland grabando un monitor no " +
                                        "hace nada, así que ahí no se ofrece.")
                                }
                            }
                            RowLayout {
                                visible: !fuente.esAudio && !raiz.emitiendo
                                QQC2.CheckBox {
                                    id: replay
                                    onToggled: raiz.recordar("replay", checked ? "1" : "0")
                                    Component.onCompleted: raiz.restaurarCasilla(replay, "replay")
                                    // Repetir y emitir se excluyen: emitiendo no hay
                                    // buffer que guardar, lo que sale ya se ha ido.
                                    text: qsTr("Modo repetición")
                                }
                                Kirigami.ContextualHelpButton {
                                    Layout.alignment: Qt.AlignVCenter
                                    toolTipText: qsTr(
                                        "Graba sin escribir nada: va guardando en " +
                                        "memoria los últimos minutos y solo los " +
                                        "vuelca a un fichero cuando pulsas «Guardar " +
                                        "lo último».\n\nSirve para lo que YA ha " +
                                        "pasado: te das cuenta de que querías " +
                                        "grabarlo cuando ya ha ocurrido, y todavía " +
                                        "estás a tiempo.")
                                }
                            }
                            QQC2.ComboBox {
                                id: segundosReplay
                                onActivated: raiz.recordar("replaySegundos", currentValue)
                                onModelChanged: Qt.callLater(function() {
                                    raiz.restaurarCombo(segundosReplay, "replaySegundos")
                                })
                                Component.onCompleted: raiz.restaurarCombo(segundosReplay, "replaySegundos")
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
                    // No bloquea nada: se puede volver a grabar mientras, porque
                    // la recompresion va en otro proceso y con prioridad baja.
                    visible: Controlador.reduciendo
                    type: Kirigami.MessageType.Information
                    text: qsTr("Reduciendo la grabación… puedes seguir usando el programa")
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: !Controlador.reduciendo && Controlador.ultimaReduccion !== ""
                    type: Kirigami.MessageType.Positive
                    text: qsTr("Reducida: %1").arg(Controlador.ultimaReduccion)
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
