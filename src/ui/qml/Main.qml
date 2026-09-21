// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// La ventana de Easy Screen Recorder.
//
// Esta pensada por JERARQUIA y por MOMENTO, no por tipo de dato. De los
// cuarenta y dos controles que hay, uno se toca siempre —la fuente—, cuatro a
// menudo y diecisiete una vez en la vida. Antes los cuarenta y dos tenian el
// mismo tamaño, el mismo color y el mismo peso, repartidos en dos columnas que
// no significaban nada: la izquierda y la derecha se cortaron por espacio, no
// por sentido. Y la ventana medida salia de 1114x866 en una pantalla de
// 1366x768, o sea que no cabia en la pantalla donde se diseño.
//
// Ahora:
//
//   - Cabecera FIJA con lo unico que se toca siempre: fuente, grabar y el
//     perfil. No se desplaza nunca, pase lo que pase debajo.
//   - Debajo, una sola columna de tarjetas con titulo, en el orden en que se
//     piensa una grabacion: que grabo, video, audio, mientras grabo, al
//     terminar, donde se guarda.
//   - Cada opcion lleva su explicacion escrita debajo. Antes vivian en
//     diecinueve botones «i» que habia que descubrir y perseguir con el raton;
//     eran la textura mas visible de la ventana, o sea ruido.
//   - Nada plegado: las secciones se ven todas, solo hay que bajar. Lo que se
//     quito en la 0.9.0 fue un boton «Avanzado» que escondia sin decir que,
//     y eso no vuelve.
//
// Las medidas y los colores salen de Sistema.qml, que explica de donde viene
// cada numero.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import es.solucionesconscientes.esr

Kirigami.ApplicationWindow {
    id: raiz
    title: "Easy Screen Recorder"

    // Una sola columna, asi que el ancho ya no depende de cuanto mida la
    // etiqueta mas larga. 30 unidades de rejilla son 540 px con el tipo de
    // letra de serie, y crecen con el tipo de letra de quien lo use.
    readonly property int anchoBase: Kirigami.Units.gridUnit * 30
    width: Math.min(anchoBase, Screen.desktopAvailableWidth)
    height: Math.min(Math.round(width * Sistema.raizPhi), Screen.desktopAvailableHeight)
    minimumWidth: Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Units.gridUnit * 16

    // El alto cambia con el estado: configurando es un rectangulo de raiz de
    // phi y grabando uno aureo, que es mucho mas bajo porque ahi solo hay un
    // reloj y tres botones. Se hace a mano y no con una atadura para que, si
    // alguien estira la ventana, su tamaño mande a partir de entonces.
    function ajustarVentana() {
        if (raiz.visibility === Window.Maximized
                || raiz.visibility === Window.FullScreen) return
        var alto = raiz.grabando ? Math.round(raiz.width / Sistema.phi)
                                 : Math.round(raiz.width * Sistema.raizPhi)
        raiz.height = Math.min(alto, Screen.desktopAvailableHeight)
    }

    // El azul del tema del usuario, oscurecido hasta que el blanco encima pasa
    // AA: 4,73 en vez de 2,49. Sigue siendo SU acento, asi que quien tenga
    // Plasma en verde vera el boton en verde.
    readonly property color colorAccion: Qt.darker(Kirigami.Theme.highlightColor, 1.5)
    readonly property color colorAccionPulsada: Qt.darker(Kirigami.Theme.highlightColor, 1.8)
    // El gris de las explicaciones, el que toque segun el tema.
    readonly property color colorExplicacion:
        Kirigami.Theme.backgroundColor.hslLightness > 0.5 ? Sistema.explicacionEnClaro
                                                          : Sistema.explicacionEnOscuro

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
            modoFotogramas: raiz.puedeModoContent && soloAlCambiar.checked ? "content" : "",
            limiteResolucion: limiteResolucion.currentValue,
            replaySegundos: replay.checked ? segundosReplay.value : 0,
            reducir: String(reducirAlTerminar.currentValue),
            cursor: grabarCursor.checked,
            quitarInicio: quitarInicio.value,
            quitarFinal: quitarFinal.value,
            bufferEnDisco: replay.checked && bufferEnDisco.checked,
            carpetasPorFecha: replay.checked && carpetasPorFecha.checked,
            guion: guionAlTerminar.text.trim()
        }
        if (region !== "") opciones.region = region
        // El temporizador, si se pidio. Se arma aqui y no al cambiar de estado
        // porque aqui es donde se sabe que ESTA grabacion lo pidio.
        if (pararSola.value > 0) {
            relojParada.interval = pararSola.value * 60 * 1000
            relojParada.start()
        }
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
    // Ojo: NO se mira `emitirEnDirecto.visible`. Desde que las secciones se
    // pliegan, «visible» dice si la tarjeta esta abierta, no si la opcion
    // aplica, y con la seccion cerrada se perdian las opciones puestas.
    readonly property bool emitiendo: !fuente.esAudio && emitirEnDirecto.checked
    readonly property bool puedeModoContent:
        Controlador.modoContentEfectivo(String(fuente.currentValue))

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
        // Lo piden la bandeja y el atajo global cuando la grabacion ya es larga:
        // hay que sacar la ventana, porque la pregunta no se puede hacer desde
        // un icono.
        function onConfirmarDescartePedido() {
            raiz.show()
            raiz.raise()
            raiz.requestActivate()
            confirmarDescartar.open()
        }
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
    // Wayland `visibility` dice que el compositor acepto el cambio de estado,
    // no que haya terminado de dibujar su animacion. El margen esta
    // dimensionado por arriba a proposito, porque lo unico que cuesta es
    // empezar un tercio de segundo mas tarde; la duracion real de la animacion
    // de KWin quedo SIN MEDIR.
    //
    // El tope existe porque la señal puede no llegar nunca en un escritorio que
    // no minimice. Sin el, el boton se quedaria muerto para siempre.
    property var arranquePendiente: null
    // Parar sola al cumplirse el tiempo pedido. Se para por el camino normal,
    // asi que guarda y hace el post-proceso como cualquier otra.
    Timer {
        id: relojParada
        repeat: false
        onTriggered: if (raiz.grabando) Controlador.parar()
    }
    onGrabandoChanged: {
        if (!raiz.grabando) relojParada.stop()
        raiz.ajustarVentana()
    }

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

    // Los perfiles: un clic que deja puestas varias opciones a la vez.
    //
    // No son ajustes nuevos, son atajos a los que ya hay. Despues de aplicarlos
    // se puede cambiar lo que sea, y lo que quede es lo que se recuerda: el
    // perfil no manda sobre nada, solo ahorra el paseo.
    //
    // En la cabecera y en fila, no en un desplegable perdido entre los ajustes:
    // es el segundo control mas usado despues de la fuente, y en fila se ven
    // los cuatro de golpe sin abrir nada.
    property string perfilActual: ""
    function elegirPerfil(valor) {
        raiz.perfilActual = valor
        raiz.aplicarPerfil(valor)
        raiz.recordar("perfil", valor)
    }
    function aplicarPerfil(perfil) {
        if (perfil === "") return
        function poner(combo, valor) {
            var i = combo.indexOfValue(valor)
            if (i >= 0) combo.currentIndex = i
        }
        if (perfil === "tutorial") {
            // Explicando algo: voz incluida, 30 imagenes bastan para una
            // pantalla, y el fichero se reduce porque suele acabar compartido.
            poner(calidad, "very_high")
            poner(fps, 30)
            poner(audio, Controlador.hayMicrofono ? "mezclado" : "sistema")
            poner(reducirAlTerminar, "normal")
            grabarCursor.checked = true
        } else if (perfil === "juego") {
            // Fluidez por encima de todo, y sin tocar el fichero despues: una
            // partida larga tardaria demasiado en recomprimirse.
            poner(calidad, "very_high")
            poner(fps, 60)
            poner(audio, "sistema")
            poner(reducirAlTerminar, "")
            grabarCursor.checked = true
        } else if (perfil === "reunion") {
            // Lo que importa es que se oiga y que ocupe poco: nadie revisa una
            // reunion fotograma a fotograma.
            poner(calidad, "high")
            poner(fps, 30)
            poner(audio, Controlador.hayMicrofono ? "mezclado" : "sistema")
            poner(reducirAlTerminar, "maximo")
            grabarCursor.checked = true
        }
    }

    // El texto de cada codec.
    //
    // El identificador es el de GSR y no se toca NUNCA: es lo que viaja en -k.
    // Lo que se le añade al lado es lo unico que decide la eleccion, que es
    // siempre lo mismo: tamaño contra compatibilidad. Un identificador que no
    // conozcamos sale tal cual, sin inventarle una descripcion, porque la lista
    // la da la maquina y GSR puede añadir nombres nuevos cuando quiera.
    function etiquetaCodec(id) {
        var notas = {
            "h264": qsTr("lo reproduce todo, hasta un televisor viejo"),
            "hevc": Controlador.hevcPocoFiable
                    // Ya no es un «puede»: esta maquina lo ha demostrado al
                    // grabar, y decirlo flojito seria dejar que tropiece otra vez.
                    ? qsTr("en tu tarjeta sale más grande y con menos detalle que h264")
                    : qsTr("comprime más que h264 en tarjetas recientes"),
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

    Kirigami.PromptDialog {
        id: confirmarDescartar
        title: qsTr("¿Descartar esta grabación?")
        subtitle: qsTr("El fichero se borra. No se puede recuperar.")
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action {
                text: qsTr("Descartarla")
                icon.name: "edit-delete"
                onTriggered: {
                    Controlador.descartar()
                    confirmarDescartar.close()
                }
            },
            Kirigami.Action {
                text: qsTr("Seguir grabando")
                icon.name: "dialog-cancel"
                onTriggered: confirmarDescartar.close()
            }
        ]
    }

    FileDialog {
        id: dialogoGuion
        title: qsTr("¿Qué programa se ejecuta al guardar?")
        onAccepted: {
            guionAlTerminar.text = selectedFile.toString().replace("file://", "")
            raiz.recordar("guion", guionAlTerminar.text)
        }
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
        raiz.perfilActual = Controlador.ajusteRecordado("perfil", "")
        fuente.forceActiveFocus()
        if (Qt.application.arguments.indexOf("--selector") !== -1) {
            selectorRegion.abrir()
        }
    }

    // Sin barra de titulo de Kirigami: la ventana ya lleva la del sistema y
    // dos titulos seguidos son un titulo de menos.
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    pageStack.initialPage: Kirigami.ScrollablePage {
        id: pagina
        padding: 0
        topPadding: Sistema.r13
        bottomPadding: Sistema.r21
        leftPadding: Sistema.r13
        rightPadding: Sistema.r13

        // --- La cabecera, que no se desplaza NUNCA -----------------------
        //
        // Es lo unico que se toca siempre: la fuente, el boton y el perfil.
        // Da igual por donde vaya la pagina, grabar esta siempre a la vista.
        // Se lleva la parte corta del reparto aureo del alto, 0,382, y de ahi
        // sale su aire: no es un margen elegido a ojo.
        header: Rectangle {
            id: cabeceraFija
            // Grabando se queda en una franja roja de 3 px, que es la unica
            // señal que hace falta cuando la ventana solo tiene un reloj: se
            // reconoce de un vistazo y no ocupa sitio.
            color: raiz.grabando ? Sistema.grabando : Kirigami.Theme.backgroundColor
            readonly property bool configurando:
                !raiz.grabando && (Controlador.estado === "listo" || raiz.ocupado)
            visible: configurando || raiz.grabando
            // Lo que pida su contenido, ni un pixel mas.
            //
            // Aqui estuvo el reparto aureo: la cabecera se quedaba 0,382 del
            // alto (262 px de 687) y el resto era para desplazar. Se midio lo
            // que costaba y se quito. En una ventana de 687 px, forzar ese
            // minimo regalaba 41 px de aire a una cabecera que ya estaba
            // holgada y se los quitaba al hueco desplazable, que es el que
            // anda justo. La proporcion es bonita; el sitio, mas util.
            implicitHeight: raiz.grabando ? Sistema.r3
                          : configurando ? cabecera.implicitHeight + Sistema.r21 * 2
                          : 0

            ColumnLayout {
                id: cabecera
                visible: cabeceraFija.configurando
                anchors.fill: parent
                anchors.margins: Sistema.r21
                anchors.bottomMargin: Sistema.r13
                spacing: Sistema.r8

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
                    enabled: !raiz.ocupado
                    // Sin etiqueta «Fuente» encima: el propio desplegable dice
                    // «Pantalla del portátil · 1366×768», que se explica solo.
                    // Una etiqueta que repite lo que hay debajo es una linea
                    // que se lee y no informa.
                    //
                    // Por el IDENTIFICADOR, no por el texto. Antes era
                    // `currentText.indexOf("Solo audio") === 0`, y con la
                    // interfaz traducida ese prefijo no casa: el formulario
                    // ensenaria los controles de video en modo solo-audio.
                    readonly property bool esAudio:
                        String(currentValue).indexOf("audio:") === 0
                }

                QQC2.Button {
                    id: botonGrabar
                    Layout.fillWidth: true
                    Layout.preferredHeight: Sistema.r55
                    Layout.topMargin: Sistema.r5
                    // Emitiendo hace falta la clave: sin ella el servidor
                    // rechaza la conexion y el usuario solo ve que no pasa nada.
                    enabled: !raiz.ocupado && String(fuente.currentValue) !== ""
                             && (!raiz.emitiendo || claveEmision.text.trim() !== "")
                    text: Controlador.estado === "arrancando" ? qsTr("Arrancando…")
                        : Controlador.estado === "guardando" ? qsTr("Guardando…")
                        : raiz.emitiendo ? qsTr("Emitir en directo")
                        : qsTr("Grabar")
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
                    // Pintado a mano y no con `highlighted`: el azul de Breeze
                    // con texto blanco encima da 2,49 de contraste y AA pide
                    // 4,5. Oscurecido sube a 4,73 y sigue siendo el acento de
                    // quien lo use.
                    contentItem: RowLayout {
                        spacing: Sistema.r8
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            implicitWidth: Sistema.r13
                            implicitHeight: Sistema.r13
                            radius: width / 2
                            // El punto rojo es la señal universal de grabar, y
                            // el rojo no se usa para nada mas en la ventana.
                            color: botonGrabar.enabled ? Sistema.grabando
                                                       : Kirigami.Theme.disabledTextColor
                            visible: !raiz.emitiendo
                        }
                        QQC2.Label {
                            text: botonGrabar.text
                            font.pointSize: Sistema.t1
                            font.bold: true
                            color: botonGrabar.enabled ? "white"
                                                       : Kirigami.Theme.disabledTextColor
                        }
                        Item { Layout.fillWidth: true }
                    }
                    background: Rectangle {
                        radius: Sistema.r5
                        color: !botonGrabar.enabled ? Qt.rgba(0, 0, 0, 0.07)
                             : botonGrabar.down ? raiz.colorAccionPulsada
                             : botonGrabar.hovered ? Qt.lighter(raiz.colorAccion, 1.15)
                             : raiz.colorAccion
                        border.width: botonGrabar.enabled ? 0 : 1
                        border.color: Kirigami.Theme.disabledTextColor
                    }
                }

                // Debajo del boton, las dos unicas cifras que hacen falta antes
                // de grabar: como se dispara sin la ventana, y donde cae lo
                // grabado con cuanto sitio queda. El resto son ajustes.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Sistema.r3
                    spacing: Sistema.r8
                    QQC2.Label {
                        visible: Controlador.hayAtajos
                        text: qsTr("%1 en cualquier ventana").arg(Controlador.atajoGrabar)
                        color: raiz.colorExplicacion
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Item { Layout.fillWidth: !Controlador.hayAtajos }
                    QQC2.Label {
                        visible: !raiz.emitiendo
                        text: {
                            var carpeta = fuente.esAudio ? Controlador.carpetaAudio
                                                         : Controlador.carpetaVideos
                            var corta = carpeta.substring(carpeta.lastIndexOf("/") + 1)
                            if (Controlador.espacioLibreMb < 0) return corta
                            var sitio = Controlador.espacioLibreMb < 1024
                                ? qsTr("%1 MB libres").arg(Controlador.espacioLibreMb)
                                : qsTr("%1 GB libres")
                                  .arg((Controlador.espacioLibreMb / 1024).toLocaleString(Qt.locale(), 'f', 1))
                            return corta + " · " + sitio
                        }
                        // En rojo cuando aprieta: quedarse sin sitio a mitad de
                        // grabacion es el peor fallo que puede tener un
                        // grabador, porque te enteras al final. Dos gigas son
                        // media hora larga a los caudales normales.
                        color: Controlador.espacioLibreMb >= 0
                               && Controlador.espacioLibreMb < 2048
                               ? Kirigami.Theme.negativeTextColor : raiz.colorExplicacion
                        font: Kirigami.Theme.smallFont
                    }
                }

                Item { Layout.fillHeight: true }

                // --- Perfiles, en fila -----------------------------------
                QQC2.Label {
                    visible: !fuente.esAudio
                    text: qsTr("Perfiles")
                    color: raiz.colorExplicacion
                    font: Kirigami.Theme.smallFont
                }
                RowLayout {
                    visible: !fuente.esAudio
                    Layout.fillWidth: true
                    spacing: 0
                    Repeater {
                        // Sin `checkable`: al pulsar una casilla marcable, Qt
                        // escribe `checked` a mano y rompe la atadura que dice
                        // cual esta puesto. Con `highlighted` se pinta lo
                        // mismo y el estado sigue viviendo en un solo sitio.
                        model: [
                            { texto: qsTr("A mi manera"), valor: "" },
                            { texto: qsTr("Tutorial"), valor: "tutorial" },
                            { texto: qsTr("Juego"), valor: "juego" },
                            { texto: qsTr("Reunión"), valor: "reunion" }
                        ]
                        QQC2.Button {
                            required property var modelData
                            text: modelData.texto
                            flat: raiz.perfilActual !== modelData.valor
                            highlighted: raiz.perfilActual === modelData.valor
                            enabled: !raiz.ocupado
                            onClicked: raiz.elegirPerfil(modelData.valor)
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            Kirigami.Separator {
                visible: cabeceraFija.configurando
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }

        ColumnLayout {
            id: columna
            // Dentro de una ScrollablePage el ancho lo pone uno mismo (asi lo
            // documenta Kirigami) y el alto manda sobre el desplazamiento. El
            // minimo es el hueco visible para que los mensajes centrados sigan
            // centrados cuando sobra sitio.
            width: pagina.width - pagina.leftPadding - pagina.rightPadding
            height: Math.max(implicitHeight, pagina.flickable ? pagina.flickable.height : 0)
            spacing: Sistema.r13

            // El ancho de un desplegable dentro de una fila. Fijo para que
            // todos caigan en la misma vertical: una columna de controles que
            // no se alinea se lee como una lista rota.
            readonly property int anchoControl: Kirigami.Units.gridUnit * 12

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

            // --- Los avisos, arriba del todo: lo que ha pasado se lee antes
            // que lo que se puede configurar.
            ColumnLayout {
                Layout.fillWidth: true
                // Un layout dentro de otro RELLENA por defecto, y sin esto se
                // repartia el hueco sobrante con el espaciador del final: sin
                // un solo aviso que enseñar, este bloque se quedaba 63 px y
                // dejaba un palmo de nada entre la cabecera y la primera
                // seccion. Medido con los hijos de la columna.
                Layout.fillHeight: false
                spacing: Sistema.r8
                visible: !raiz.grabando

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
                    visible: Controlador.veredictoCodecs !== ""
                    type: Kirigami.MessageType.Information
                    text: Controlador.veredictoCodecs
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    // No bloquea nada: se puede volver a grabar mientras, porque
                    // la recompresion va en otro proceso y con prioridad baja.
                    visible: Controlador.reduciendo
                    type: Kirigami.MessageType.Information
                    text: qsTr("Reduciendo la grabación. Puedes seguir trabajando")
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
                    visible: Controlador.diagnostico !== "" && Controlador.estado !== "sinGsr"
                    type: Kirigami.MessageType.Warning
                    text: Controlador.diagnostico
                }
            }

            // --- Los ajustes, en tarjetas con titulo -----------------------
            ColumnLayout {
                id: ajustes
                visible: !raiz.grabando
                         && (Controlador.estado === "listo" || raiz.ocupado)
                enabled: !raiz.ocupado
                Layout.fillWidth: true
                Layout.fillHeight: false
                spacing: Sistema.r13

                // ============ QUE GRABO ============
                CabeceraSeccion {
                    id: cabQueGrabo
                    visible: !fuente.esAudio
                    clave: "queGrabo"
                    titulo: qsTr("Qué grabo")
                    resumen: {
                        var p = []
                        if (emitirEnDirecto.checked) p.push(qsTr("en directo"))
                        if (raiz.camaraActiva) p.push(qsTr("con cámara"))
                        if (replay.checked && !raiz.emitiendo) {
                            p.push(qsTr("repetición, %1").arg(segundosReplay.displayText))
                        }
                        return p.length > 0 ? p.join(" · ") : qsTr("solo la pantalla")
                    }
                }
                FormCard.FormCard {
                    id: tarjetaQueGrabo
                    Layout.fillWidth: true
                    visible: cabQueGrabo.visible && cabQueGrabo.abierta
                    // El gris corregido, que pasa AA sobre la tarjeta. Lo
                    // heredan tanto los delegados de Kirigami como los nuestros.
                    Kirigami.Theme.disabledTextColor: raiz.colorExplicacion

                    // --- Emitir en directo: es un MODO, no una opcion mas.
                    // Mientras esta puesto no se graba a fichero, no hay pausa
                    // y la calidad pasa a ser un caudal.
                    FormCard.FormCheckDelegate {
                        id: emitirEnDirecto
                        text: qsTr("Emitir en directo")
                        description: qsTr("Manda la pantalla a YouTube, Twitch o tu propio " +
                                          "servidor. No se puede pausar: dejar de mandar imagen " +
                                          "hace que la plataforma dé la emisión por caída.")
                    }
                    FormCard.FormDelegateSeparator { visible: raiz.emitiendo }
                    FilaAjuste {
                        visible: raiz.emitiendo
                        enColumna: true
                        titulo: qsTr("Servidor")
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
                    }
                    FilaAjuste {
                        visible: raiz.emitiendo
                        enColumna: true
                        titulo: qsTr("Clave")
                        // Dicho donde se pega, no en la documentacion: es una
                        // credencial y el usuario tiene derecho a saber que no
                        // se queda en ningun sitio.
                        explicacion: qsTr("La clave se usa y se olvida. Pégala cada vez.")
                        QQC2.TextField {
                            id: claveEmision
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: qsTr("pégala aquí")
                        }
                    }
                    FilaAjuste {
                        visible: raiz.emitiendo
                        titulo: qsTr("Caudal de subida")
                        explicacion: qsTr("Son los valores que recomienda YouTube. Necesitas " +
                                          "subida por encima de la cifra: si tu conexión no da, " +
                                          "la emisión se corta a trozos.")
                        QQC2.ComboBox {
                            id: bitrateEmision
                            Layout.preferredWidth: columna.anchoControl
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
                    }
                    FormCard.FormCheckDelegate {
                        id: guardarEmision
                        visible: raiz.emitiendo
                        onToggled: raiz.recordar("guardarEmision", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(guardarEmision, "guardarEmision")
                        text: qsTr("Guardar una copia mientras emites")
                        // MARCADA. Lo que sale por el cable no se puede
                        // recuperar, y un fichero que sobra se borra; al reves
                        // no hay arreglo. Medido: no cuesta CPU ni RAM, porque
                        // el mismo grabador escribe las dos salidas. Lo que
                        // cuesta es disco, y por eso se dice aqui cuanto.
                        checked: true
                        description: qsTr("La escribe el mismo grabador, así que no cuesta una " +
                                          "segunda codificación. Va en .flv, dentro de %1, y " +
                                          "ocupa unos %2.")
                                     .arg(Controlador.carpetaVideos)
                                     .arg(qsTr("%1 GB por hora")
                                          .arg((parseInt(bitrateEmision.currentValue) * 3600
                                                / 8 / 1000000).toLocaleString(Qt.locale(), 'f', 1)))
                    }

                    // --- La camara superpuesta.
                    FormCard.FormDelegateSeparator { visible: raiz.puedeCamara }
                    FormCard.FormCheckDelegate {
                        id: usarCamara
                        visible: raiz.puedeCamara
                        onToggled: raiz.recordar("usarCamara", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(usarCamara, "usarCamara")
                        text: qsTr("Sacar tu cámara sobre la pantalla")
                        description: qsTr("Pantalla y cámara en el mismo fichero, sin nada que " +
                                          "montar después. La vista previa se apaga al grabar: " +
                                          "una cámara solo admite un programa a la vez.")
                    }
                    FilaAjuste {
                        visible: raiz.camaraActiva
                        enColumna: true
                        titulo: qsTr("Cámara")
                        QQC2.ComboBox {
                            id: camara
                            onActivated: raiz.recordar("camara", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(camara, "camara")
                            })
                            Component.onCompleted: raiz.restaurarCombo(camara, "camara")
                            Layout.fillWidth: true
                            model: Controlador.camaras
                            textRole: "texto"
                            valueRole: "valor"
                        }
                    }
                    FilaAjuste {
                        visible: raiz.camaraActiva
                        titulo: qsTr("Tamaño")
                        // En PORCENTAJE del ancho, no en pixeles: un tamaño
                        // fijo es un cuarto de pantalla en 1366 y un decimo
                        // en 4K.
                        explicacion: qsTr("Parte del ancho de la pantalla que ocupa tu cara.")
                        QQC2.Slider {
                            id: tamanoCamara
                            from: 5; to: 50; stepSize: 1; value: 25
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                        }
                        QQC2.Label {
                            text: tamanoCamara.value + " %"
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        }
                    }
                    FormCard.FormCheckDelegate {
                        id: espejoCamara
                        visible: raiz.camaraActiva
                        onToggled: raiz.recordar("espejoCamara", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(espejoCamara, "espejoCamara")
                        checked: true
                        text: qsTr("Verte como en un espejo")
                        description: qsTr("Solo cambia lo que ves tú mientras grabas.")
                    }
                    // Las dos ayudas a la vez: a la izquierda tu cara, a la
                    // derecha donde va a quedar. Responden a preguntas distintas.
                    FilaAjuste {
                        visible: raiz.camaraActiva
                        enColumna: true
                        titulo: qsTr("Dónde queda")
                        explicacion: qsTr("Arrastra el recuadro para colocarla.")
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Sistema.r13
                            // Va en un Loader porque el fichero solo existe si
                            // se compilo con Qt6Multimedia.
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
                    }

                    // --- Modo repeticion. Repetir y emitir se excluyen:
                    // emitiendo no hay nada que guardar, lo que sale ya se ha ido.
                    FormCard.FormDelegateSeparator { visible: !raiz.emitiendo }
                    FormCard.FormCheckDelegate {
                        id: replay
                        visible: !raiz.emitiendo
                        onToggled: raiz.recordar("replay", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(replay, "replay")
                        text: qsTr("Modo repetición")
                        description: qsTr("Graba sin escribir nada: va guardando los últimos " +
                                          "minutos y solo los vuelca a un fichero cuando pulsas " +
                                          "«Guardar lo último». Sirve para lo que YA ha pasado.")
                    }
                    FilaAjuste {
                        visible: !raiz.emitiendo && replay.checked
                        titulo: qsTr("Guardar los últimos")
                        // Lo que va a costar de RAM, con lo que graba ESTA
                        // persona: quince minutos de escritorio quieto son unos
                        // 36 MB y quince de juego medio giga. Por eso la cifra
                        // sale de lo ultimo que grabo esta maquina, no de una tabla.
                        explicacion: {
                            var t = qsTr("Escribe los segundos o los minutos que quieras, " +
                                         "de 2 segundos a 24 horas.")
                            if (Controlador.mbPorMinuto > 0 && !bufferEnDisco.checked) {
                                t += " " + qsTr("Ahora ocupa ≈ %1 MB de memoria, según lo que sueles grabar.")
                                     .arg(Math.round(Controlador.mbPorMinuto
                                                     * segundosReplay.value / 60))
                            }
                            return t
                        }
                        QQC2.SpinBox {
                            id: segundosReplay
                            // En SEGUNDOS y a mano, sin lista cerrada: los
                            // treinta segundos de una jugada y los veinte
                            // minutos de una clase son la misma opcion con otro
                            // numero. Los limites son los de GSR
                            // (gpu-screen-recorder.1, -r).
                            from: 2
                            to: 86400
                            stepSize: 15
                            value: 60
                            editable: true
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                            textFromValue: function(valor) {
                                if (valor < 60) return qsTr("%1 s").arg(valor)
                                var m = Math.floor(valor / 60)
                                var s = valor % 60
                                return s === 0 ? qsTr("%1 min").arg(m)
                                               : qsTr("%1 min %2 s").arg(m).arg(s)
                            }
                            valueFromText: function(texto) {
                                // «90» son noventa segundos; «3 min» son ciento
                                // ochenta. Se lee el numero y se mira si la
                                // persona escribio «min».
                                var n = parseInt(texto.replace(/[^0-9]/g, ""))
                                if (isNaN(n)) return 60
                                return texto.indexOf("min") >= 0 ? n * 60 : n
                            }
                            onValueModified: raiz.recordar("replaySegundos", value)
                            Component.onCompleted:
                                value = parseInt(Controlador.ajusteRecordado("replaySegundos", "60"))
                        }
                    }
                    FormCard.FormCheckDelegate {
                        id: bufferEnDisco
                        visible: !raiz.emitiendo && replay.checked
                        text: qsTr("Guardar lo último en el disco en vez de en la memoria")
                        description: qsTr("Libera la memoria, a cambio de escribir en el disco " +
                                          "sin parar. En un SSD eso desgasta.")
                        onToggled: raiz.recordar("bufferEnDisco", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(bufferEnDisco, "bufferEnDisco")
                    }
                    FormCard.FormCheckDelegate {
                        id: carpetasPorFecha
                        visible: !raiz.emitiendo && replay.checked
                        text: qsTr("Una carpeta por día")
                        description: qsTr("Crea una carpeta con la fecha y mete dentro lo que " +
                                          "guardes ese día.")
                        onToggled: raiz.recordar("carpetasPorFecha", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(carpetasPorFecha, "carpetasPorFecha")
                    }
                }

                // ============ VIDEO ============
                CabeceraSeccion {
                    id: cabVideo
                    visible: !fuente.esAudio
                    clave: "video"
                    titulo: qsTr("Vídeo")
                    resumen: raiz.emitiendo
                             ? bitrateEmision.currentText
                             : qsTr("%1 · %2 fps · %3").arg(calidad.currentText)
                               .arg(fps.currentText).arg(codecVideo.displayText)
                }
                FormCard.FormCard {
                    id: tarjetaVideo
                    Layout.fillWidth: true
                    visible: cabVideo.visible && cabVideo.abierta
                    Kirigami.Theme.disabledTextColor: raiz.colorExplicacion

                    FilaAjuste {
                        visible: !raiz.emitiendo
                        titulo: qsTr("Calidad")
                        // SIN cifras de tamaño, y no por pereza: se pusieron
                        // medidas y hubo que quitarlas. Una cifra aqui depende
                        // de tres cosas a la vez —lo que se mueva en pantalla,
                        // el codec y la GPU— y la misma etiqueta era verdad con
                        // h264 y mentira por casi el doble con hevc.
                        explicacion: qsTr("Mantiene la misma nitidez de principio a fin: ocupa más " +
                                          "donde hay movimiento y menos con la pantalla quieta.")
                        QQC2.ComboBox {
                            id: calidad
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("calidad", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(calidad, "calidad")
                            })
                            Component.onCompleted: raiz.restaurarCombo(calidad, "calidad")
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
                    }
                    FormCard.FormDelegateSeparator {}
                    FilaAjuste {
                        titulo: qsTr("Imágenes por segundo")
                        explicacion: qsTr("30 basta para un tutorial. 60 para juegos o " +
                                          "movimiento rápido.")
                        QQC2.ComboBox {
                            id: fps
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("fps", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(fps, "fps")
                            })
                            Component.onCompleted: raiz.restaurarCombo(fps, "fps")
                            model: ["30", "60"]
                            currentIndex: 1
                        }
                    }
                    FormCard.FormDelegateSeparator {}
                    FilaAjuste {
                        titulo: qsTr("Códec de vídeo")
                        explicacion: qsTr("Solo salen los que esta máquina codifica por hardware. " +
                                          "h264 es el seguro: lo abre cualquier móvil, tele o programa.")
                        QQC2.ComboBox {
                            id: codecVideo
                            // Cerrado enseña solo el identificador; la lista
                            // desplegada va ancha, que es donde se lee la
                            // descripcion para elegir.
                            displayText: currentIndex >= 0 ? String(currentValue) : ""
                            Layout.preferredWidth: columna.anchoControl
                            popup.width: Kirigami.Units.gridUnit * 26
                            onActivated: raiz.recordar("codecVideo", currentValue)
                            Component.onCompleted: raiz.restaurarCombo(codecVideo, "codecVideo")
                            textRole: "texto"
                            valueRole: "valor"
                            // Lo que la maquina soporta Y cabe en el formato:
                            // sin el segundo filtro se podia pedir h264 en un
                            // .webm, y eso no graba nada.
                            readonly property var disponibles:
                                Controlador.codecsVideoPara(contenedor.currentText,
                                                            Controlador.codecsVideo)
                            model: disponibles.map(function(c) {
                                return { texto: raiz.etiquetaCodec(c), valor: c }
                            })
                            // h264 cuando esta, que es lo que elegiria GSR solo.
                            // Si no esta, el primero que haya: la lista la manda
                            // la maquina.
                            onModelChanged: {
                                var i = disponibles.indexOf("h264")
                                currentIndex = i >= 0 ? i : 0
                                // Y si esta persona ya eligio otro, manda el
                                // suyo: el default solo cubre la primera vez.
                                Qt.callLater(function() {
                                    raiz.restaurarCombo(codecVideo, "codecVideo")
                                })
                            }
                        }
                    }
                    FormCard.FormButtonDelegate {
                        // La prueba de la tarjeta, pegada al desplegable que
                        // decide. Antes era una fila suelta —«Tu tarjeta:
                        // [Probar]»— en otra parte de la ventana, lejos de la
                        // eleccion que ayuda a tomar.
                        visible: !raiz.ocupado
                        enabled: !Controlador.comprobandoCodecs
                        icon.name: "speedometer"
                        text: Controlador.comprobandoCodecs ? qsTr("Probando…")
                                                            : qsTr("Probar mi tarjeta")
                        description: qsTr("Que una tarjeta ofrezca un códec no dice que lo haga " +
                                          "bien. Graba unos segundos con cada uno y te dice cuál " +
                                          "te conviene. Tarda diez segundos y no deja nada.")
                        onClicked: Controlador.comprobarCodecs()
                    }
                    FormCard.FormDelegateSeparator {}
                    FilaAjuste {
                        titulo: qsTr("Resolución máxima")
                        explicacion: qsTr("Achica la imagen sin recortar nada: se graba lo mismo, " +
                                          "más pequeño y ocupando menos.")
                        QQC2.ComboBox {
                            id: limiteResolucion
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("resolucion", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(limiteResolucion, "resolucion")
                            })
                            Component.onCompleted: raiz.restaurarCombo(limiteResolucion, "resolucion")
                            textRole: "texto"
                            valueRole: "valor"
                            // Solo los techos que esta pantalla puede usar.
                            // Ofrecer «como mucho 1080p» en una pantalla de 768
                            // px es ofrecer algo que no hace nada.
                            model: {
                                var m = [{ texto: qsTr("La de la pantalla (%1×%2)")
                                                    .arg(Screen.width).arg(Screen.height),
                                           valor: "" }]
                                if (Screen.height > 1080) {
                                    m.push({ texto: qsTr("Como mucho 1080p"), valor: "1920x1080" })
                                }
                                if (Screen.height > 720) {
                                    m.push({ texto: qsTr("Como mucho 720p"), valor: "1280x720" })
                                }
                                return m
                            }
                            currentIndex: 0
                        }
                    }
                    FormCard.FormDelegateSeparator { visible: contenedorFila.visible }
                    FilaAjuste {
                        id: contenedorFila
                        // Emitiendo el contenedor es flv y el codec aac: no hay
                        // nada que elegir, asi que no se enseña.
                        visible: !raiz.emitiendo
                        titulo: qsTr("Formato del fichero")
                        explicacion: qsTr("mkv aguanta un corte de luz sin perder lo grabado. " +
                                          "mp4 es el que entienden más editores y páginas.")
                        QQC2.ComboBox {
                            id: contenedor
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("formato", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(contenedor, "formato")
                            })
                            Component.onCompleted: raiz.restaurarCombo(contenedor, "formato")
                            model: Controlador.contenedores
                        }
                    }
                    FormCard.FormDelegateSeparator {}
                    FormCard.FormCheckDelegate {
                        id: grabarCursor
                        checked: true
                        text: qsTr("El puntero sale en el vídeo")
                        description: qsTr("Quítalo para una demo limpia de una interfaz o para " +
                                          "documentación: el puntero paseando mientras explicas " +
                                          "distrae más que ayuda.")
                        onToggled: raiz.recordar("cursor", checked ? "1" : "0")
                        Component.onCompleted: {
                            // Por defecto SI sale, que es el default de GSR y lo
                            // que espera casi todo el mundo.
                            checked = Controlador.ajusteRecordado("cursor", "1") === "1"
                        }
                    }
                    FormCard.FormCheckDelegate {
                        id: soloAlCambiar
                        // Solo donde de verdad hace algo: en Wayland sobre un
                        // monitor GSR lo acepta, avisa por stderr y lo ignora.
                        visible: Controlador.modoContentEfectivo(String(fuente.currentValue))
                        text: qsTr("Escribir solo cuando la pantalla cambia")
                        description: qsTr("Mientras la pantalla esté quieta no gasta GPU ni ocupa " +
                                          "sitio. Un tutorial con pausas acaba ocupando mucho menos.")
                        onToggled: raiz.recordar("soloAlCambiar", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(soloAlCambiar, "soloAlCambiar")
                    }
                }

                // ============ AUDIO ============
                CabeceraSeccion {
                    id: cabAudio
                    clave: "audio"
                    titulo: qsTr("Audio")
                    resumen: fuente.esAudio ? formatoAudio.currentText
                           : raiz.emitiendo ? audio.currentText
                           : audio.currentText + " · " + codecAudio.currentText
                }
                FormCard.FormCard {
                    Layout.fillWidth: true
                    visible: cabAudio.abierta
                    Kirigami.Theme.disabledTextColor: raiz.colorExplicacion

                    FilaAjuste {
                        visible: !fuente.esAudio
                        titulo: qsTr("Qué se oye")
                        explicacion: qsTr("«En una sola pista» se oye todo en cualquier reproductor. " +
                                          "«Separadas» deja equilibrarlas al editar, pero muchos " +
                                          "reproductores suenan solo la primera y el micrófono " +
                                          "parecerá mudo.")
                        QQC2.ComboBox {
                            id: audio
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("audio", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(audio, "audio")
                            })
                            Component.onCompleted: raiz.restaurarCombo(audio, "audio")
                            textRole: "texto"
                            valueRole: "valor"
                            // Mezclado va ANTES que separado: con dos pistas
                            // casi todos los reproductores suenan solo la
                            // primera y el microfono parece mudo. Detras, las
                            // aplicaciones que suenan AHORA.
                            model: [
                                { texto: qsTr("Audio del sistema"), valor: "sistema" },
                                { texto: qsTr("Micrófono"), valor: "micro" },
                                { texto: qsTr("Los dos, en una sola pista"), valor: "mezclado" },
                                { texto: qsTr("Los dos, en pistas separadas (para editar)"), valor: "ambos" },
                                { texto: qsTr("Sin audio"), valor: "nada" }
                            ].concat(Controlador.audiosAplicacion.map(function(a) {
                                return { texto: qsTr("Solo el sonido de %1").arg(a.texto),
                                         valor: a.valor }
                            }))
                            // Por defecto, los dos mezclados: quien graba la
                            // pantalla casi siempre se esta explicando encima, y
                            // descubrir al acabar que no habia voz no tiene
                            // arreglo. Sin microfono de verdad, solo el sistema.
                            currentIndex: Controlador.hayMicrofono ? 2 : 0
                            // Al desplegarlo se vuelve a preguntar que esta
                            // sonando. La lista calculada al arrancar casi nunca
                            // sirve: entre abrir esto y darle a grabar, el
                            // usuario abre justo lo que queria grabar.
                            onPressedChanged: if (pressed) {
                                Controlador.refrescarAplicacionesSonando()
                            }
                        }
                    }
                    FormCard.FormDelegateSeparator { visible: !fuente.esAudio && !raiz.emitiendo }
                    FilaAjuste {
                        visible: !fuente.esAudio && !raiz.emitiendo
                        titulo: qsTr("Códec de audio")
                        explicacion: qsTr("opus suena mejor ocupando lo mismo. aac solo si el " +
                                          "vídeo va a un editor antiguo.")
                        QQC2.ComboBox {
                            id: codecAudio
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("codecAudio", currentValue)
                            Component.onCompleted: raiz.restaurarCombo(codecAudio, "codecAudio")
                            // Solo los que GSR respeta en ese formato y en ese
                            // reparto de pistas.
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
                    }
                    FilaAjuste {
                        id: filaVumetro
                        visible: Controlador.hayMultimedia && !fuente.esAudio
                                 && ["micro", "mezclado", "ambos"].indexOf(
                                        String(audio.currentValue)) !== -1
                        titulo: qsTr("Volumen del micrófono")
                        explicacion: !probarMicro.checked
                                     ? qsTr("Compruébalo antes de grabar, no después.")
                                     : Controlador.nivelMicro > 0.002 ? qsTr("Te oigo.")
                                                                      : qsTr("Sin señal.")
                        // El micro se abre cuando alguien lo PIDE, no al abrir
                        // la ventana: encender el piloto del microfono nada mas
                        // arrancar, sin que nadie haya pedido grabar, ademas de
                        // costar arranque asusta.
                        readonly property bool escuchando: probarMicro.checked && visible
                        onEscuchandoChanged: Controlador.escucharMicro(escuchando)
                        QQC2.ProgressBar {
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 6
                            from: 0; to: 1
                            // Escala en DECIBELIOS, de -60 dB a 0. Con la lineal
                            // la barra no se movia: una voz normal en este
                            // microfono da 0,004 de RMS.
                            value: {
                                var n = Controlador.nivelMicro
                                if (n <= 0.0001) return 0
                                var db = 20 * Math.log(n) / Math.LN10
                                return Math.max(0, Math.min(1, (db + 60) / 60))
                            }
                        }
                        QQC2.Button {
                            id: probarMicro
                            checkable: true
                            icon.name: "audio-input-microphone"
                            text: qsTr("Probar")
                        }
                    }
                    FilaAjuste {
                        visible: fuente.esAudio
                        titulo: qsTr("Formato")
                        explicacion: qsTr("flac y wav no pierden nada, a cambio de ocupar más.")
                        QQC2.ComboBox {
                            id: formatoAudio
                            Layout.preferredWidth: columna.anchoControl
                            model: Controlador.formatosAudio()
                        }
                    }
                    FilaAjuste {
                        // Se ESCONDE en flac y wav, no se deshabilita: ahi ese
                        // ajuste no existe porque no pierden informacion.
                        visible: fuente.esAudio
                                 && !Controlador.formatoAudioSinPerdida(formatoAudio.currentText)
                        titulo: qsTr("Calidad del audio")
                        QQC2.ComboBox {
                            id: bitrateAudio
                            Layout.preferredWidth: columna.anchoControl
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
                }

                // ============ MIENTRAS GRABO ============
                CabeceraSeccion {
                    id: cabMientras
                    clave: "mientras"
                    titulo: qsTr("Mientras grabo")
                    resumen: {
                        var p = []
                        if (cuentaAtrasActiva.checked) p.push(qsTr("cuenta atrás"))
                        if (Controlador.relojEnBandeja) p.push(qsTr("reloj en la bandeja"))
                        if (pararSola.value > 0) {
                            p.push(qsTr("para a los %1 min").arg(pararSola.value))
                        }
                        return p.length > 0 ? p.join(" · ")
                                            : qsTr("sin cuenta atrás, no para sola")
                    }
                }
                FormCard.FormCard {
                    Layout.fillWidth: true
                    visible: cabMientras.abierta
                    Kirigami.Theme.disabledTextColor: raiz.colorExplicacion

                    FormCard.FormCheckDelegate {
                        id: cuentaAtrasActiva
                        onToggled: raiz.recordar("cuentaAtras", checked ? "1" : "0")
                        Component.onCompleted: raiz.restaurarCasilla(cuentaAtrasActiva, "cuentaAtras")
                        // Desactivada por defecto: lo que se espera al dar a
                        // Grabar es que grabe, y una espera de tres segundos que
                        // nadie pidio deja dudando cuando empieza de verdad.
                        checked: false
                        text: qsTr("Contar 3 segundos antes de empezar")
                        description: qsTr("Tiempo para colocarte. Se ve en la bandeja y se puede " +
                                          "cancelar desde ahí.")
                    }
                    FormCard.FormDelegateSeparator {}
                    FormCard.FormCheckDelegate {
                        id: relojEnBandeja
                        // La MISMA opcion que la del menu de la bandeja, no una
                        // copia: las dos escriben la preferencia del
                        // controlador. Nacio solo en aquel menu y el primero que
                        // la busco la busco aqui.
                        checked: Controlador.relojEnBandeja
                        onToggled: {
                            Controlador.relojEnBandeja = checked
                            // Y se devuelve la atadura: escribir «checked» a
                            // mano la rompe, y sin ella marcarla desde la
                            // bandeja dejaria esta casilla diciendo lo contrario.
                            checked = Qt.binding(function() {
                                return Controlador.relojEnBandeja
                            })
                        }
                        text: qsTr("Mostrar el tiempo de grabación en la bandeja")
                        description: qsTr("Mientras grabas, la ventana se aparta para no salir en " +
                                          "el vídeo. Con esto, el tiempo se ve en un segundo icono " +
                                          "al lado del de la aplicación.")
                    }
                    FormCard.FormDelegateSeparator {}
                    FilaAjuste {
                        titulo: qsTr("Parar sola")
                        explicacion: qsTr("Para una clase, una reunión o una captura que dejas " +
                                          "sola: para y guarda ella misma al cumplirse el tiempo. " +
                                          "Escribe los minutos que quieras.")
                        QQC2.SpinBox {
                            id: pararSola
                            // Sin lista cerrada: el tiempo lo pone quien graba.
                            // Cero significa «no pares sola», y se dice con la
                            // palabra, no con un cero suelto.
                            from: 0
                            to: 600
                            stepSize: 5
                            editable: true
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                            textFromValue: function(valor) {
                                return valor === 0 ? qsTr("no") : qsTr("%1 min").arg(valor)
                            }
                            valueFromText: function(texto) {
                                var n = parseInt(texto.replace(/[^0-9]/g, ""))
                                return isNaN(n) ? 0 : n
                            }
                            onValueModified: raiz.recordar("pararSola", value)
                            Component.onCompleted:
                                value = parseInt(Controlador.ajusteRecordado("pararSola", "0"))
                        }
                    }
                    FormCard.FormDelegateSeparator { visible: Controlador.hayAtajos }
                    FormCard.FormTextDelegate {
                        // Los atajos existian desde hace tandas y la aplicacion
                        // no los mencionaba en ningun sitio, asi que para el
                        // usuario no existian. Un atajo que no se anuncia es
                        // codigo muerto.
                        visible: Controlador.hayAtajos
                        text: qsTr("Atajos de teclado")
                        description: qsTr("%1 graba y para").arg(Controlador.atajoGrabar)
                                     + (Controlador.atajoPausa !== ""
                                        ? " · " + qsTr("%1 pausa y reanuda").arg(Controlador.atajoPausa)
                                        : "")
                                     + ". " + qsTr("Funcionan con la ventana cerrada o minimizada. " +
                                                   "Se cambian en Preferencias del sistema, en " +
                                                   "Atajos de teclado.")
                    }
                }

                // ============ AL TERMINAR ============
                CabeceraSeccion {
                    id: cabAlTerminar
                    visible: !fuente.esAudio && !raiz.emitiendo
                    clave: "alTerminar"
                    titulo: qsTr("Al terminar")
                    resumen: {
                        var p = [reducirAlTerminar.currentText]
                        if (quitarInicio.value > 0 || quitarFinal.value > 0) {
                            p.push(qsTr("quita %1 s y %2 s")
                                   .arg(quitarInicio.value).arg(quitarFinal.value))
                        }
                        if (guionAlTerminar.text.trim() !== "") {
                            p.push(qsTr("ejecuta un programa"))
                        }
                        return p.join(" · ")
                    }
                }
                FormCard.FormCard {
                    id: tarjetaAlTerminar
                    Layout.fillWidth: true
                    visible: cabAlTerminar.visible && cabAlTerminar.abierta
                    Kirigami.Theme.disabledTextColor: raiz.colorExplicacion

                    FilaAjuste {
                        titulo: qsTr("Reducir el tamaño")
                        // SIN prometer un porcentaje de fabrica: cuanto encoge
                        // depende de lo bueno que sea el codificador de cada
                        // tarjeta. Lo que se enseña es lo que le paso A ESTA
                        // maquina la ultima vez, y del nivel elegido: enseñar
                        // la cifra del otro nivel seria una cifra que no es.
                        explicacion: {
                            var t = qsTr("Al acabar la comprime otra vez por procesador, que se toma " +
                                         "su tiempo y encuentra lo que la tarjeta no buscó. No " +
                                         "cambia de formato y tarda lo que dure el vídeo.")
                            var medido = String(reducirAlTerminar.currentValue) === "maximo"
                                         ? Controlador.reduccionMaximo
                                         : Controlador.reduccionNormal
                            if (medido > 0 && String(reducirAlTerminar.currentValue) !== "") {
                                t += " " + qsTr("La última vez quedó en el %1 % de su tamaño.")
                                     .arg(medido)
                            }
                            return t
                        }
                        QQC2.ComboBox {
                            id: reducirAlTerminar
                            Layout.preferredWidth: columna.anchoControl
                            onActivated: raiz.recordar("reducir", currentValue)
                            onModelChanged: Qt.callLater(function() {
                                raiz.restaurarCombo(reducirAlTerminar, "reducir")
                            })
                            Component.onCompleted: raiz.restaurarCombo(reducirAlTerminar, "reducir")
                            textRole: "texto"
                            valueRole: "valor"
                            model: [
                                { texto: qsTr("Dejarla como está"), valor: "" },
                                { texto: qsTr("Reducir el tamaño"), valor: "normal" },
                                { texto: qsTr("Reducir al máximo"), valor: "maximo" }
                            ]
                            currentIndex: 0
                        }
                    }
                    FormCard.FormDelegateSeparator {}
                    FilaAjuste {
                        titulo: qsTr("Quitar del principio y del final")
                        // Lo del fotograma clave se dice porque si no parece un
                        // fallo: pides tres segundos y quita dos o cuatro.
                        explicacion: qsTr("Para quitar el «¿dónde estaba el botón?» del principio y " +
                                          "el buscar el ratón del final. Es instantáneo y no pierde " +
                                          "calidad; a cambio corta en el fotograma clave más " +
                                          "cercano y puede desviarse un par de segundos.")
                        QQC2.SpinBox {
                            id: quitarInicio
                            from: 0; to: 120; stepSize: 1
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 5
                            textFromValue: function(valor) { return qsTr("%1 s").arg(valor) }
                            valueFromText: function(texto) {
                                var n = parseInt(texto.replace(/[^0-9]/g, ""))
                                return isNaN(n) ? 0 : n
                            }
                            onValueModified: raiz.recordar("quitarInicio", value)
                            Component.onCompleted:
                                value = parseInt(Controlador.ajusteRecordado("quitarInicio", "0"))
                        }
                        QQC2.Label { text: qsTr("y") }
                        QQC2.SpinBox {
                            id: quitarFinal
                            from: 0; to: 120; stepSize: 1
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 5
                            textFromValue: function(valor) { return qsTr("%1 s").arg(valor) }
                            valueFromText: function(texto) {
                                var n = parseInt(texto.replace(/[^0-9]/g, ""))
                                return isNaN(n) ? 0 : n
                            }
                            onValueModified: raiz.recordar("quitarFinal", value)
                            Component.onCompleted:
                                value = parseInt(Controlador.ajusteRecordado("quitarFinal", "0"))
                        }
                    }
                    FormCard.FormDelegateSeparator {}
                    FilaAjuste {
                        enColumna: true
                        titulo: qsTr("Ejecutar un programa al guardar")
                        explicacion: qsTr("Se lanza en cuanto el fichero está cerrado y recibe su " +
                                          "ruta. Es el sitio para lo que esta aplicación no va a " +
                                          "hacer: subirlo, moverlo o avisarte.")
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Sistema.r8
                            QQC2.TextField {
                                id: guionAlTerminar
                                Layout.fillWidth: true
                                placeholderText: qsTr("ninguno")
                                onEditingFinished: raiz.recordar("guion", text)
                                Component.onCompleted: text = Controlador.ajusteRecordado("guion", "")
                            }
                            QQC2.Button {
                                icon.name: "document-open"
                                onClicked: dialogoGuion.open()
                            }
                        }
                    }
                }

                // ============ DONDE SE GUARDA ============
                CabeceraSeccion {
                    id: cabCarpeta
                    visible: !raiz.emitiendo
                    clave: "carpeta"
                    titulo: qsTr("Dónde se guarda")
                    // La ruta ENTERA. La cabecera de arriba ya dice la carpeta
                    // corta y el sitio libre, asi que repetirlo aqui seria
                    // gastar la linea en algo que ya se sabe.
                    resumen: fuente.esAudio ? Controlador.carpetaAudio
                                            : Controlador.carpetaVideos
                }
                FormCard.FormCard {
                    id: tarjetaCarpeta
                    Layout.fillWidth: true
                    // Emitiendo no se guarda nada salvo la copia, que ya dice
                    // donde cae en su propia linea.
                    visible: cabCarpeta.visible && cabCarpeta.abierta
                    Kirigami.Theme.disabledTextColor: raiz.colorExplicacion

                    FilaAjuste {
                        titulo: fuente.esAudio ? Controlador.carpetaAudio
                                               : Controlador.carpetaVideos
                        explicacion: Controlador.espacioLibreMb < 0 ? ""
                                     : Controlador.espacioLibreMb < 1024
                                       ? qsTr("Quedan %1 MB libres").arg(Controlador.espacioLibreMb)
                                       : qsTr("Quedan %1 GB libres")
                                         .arg((Controlador.espacioLibreMb / 1024).toLocaleString(Qt.locale(), 'f', 1))
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
                }
            }

            // --- Grabando: un reloj y los botones. Nada mas que mirar.
            //
            // El reloj va a 33 pt, que es el ultimo escalon de la escala
            // tipografica (base 10, paso raiz de phi). No es capricho: esta
            // ventana se mira de reojo y desde lejos, con la mano en otra cosa.
            ColumnLayout {
                visible: raiz.grabando
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Sistema.r5

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Sistema.r13
                    Rectangle {
                        implicitWidth: Sistema.r13
                        implicitHeight: Sistema.r13
                        radius: width / 2
                        Layout.alignment: Qt.AlignVCenter
                        color: Controlador.estado === "pausado" ? Sistema.pausado
                                                                : Sistema.grabando
                        // Late mientras graba y se queda quieto en pausa: un
                        // punto fijo y uno que late se distinguen sin leer nada.
                        SequentialAnimation on opacity {
                            running: raiz.grabando && Controlador.estado !== "pausado"
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.25; duration: 900; easing.type: Easing.InOutQuad }
                            NumberAnimation { to: 1.0; duration: 900; easing.type: Easing.InOutQuad }
                        }
                        onVisibleChanged: if (!visible) opacity = 1
                    }
                    QQC2.Label {
                        text: raiz.tiempoBonito(Controlador.segundos)
                        font.pointSize: Sistema.t5
                        font.bold: true
                        // Cifras de ancho fijo: sin esto el reloj se ensancha y
                        // se encoge a cada segundo y baila.
                        font.features: { "tnum": 1 }
                    }
                }
                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: Controlador.estado === "pausado" ? qsTr("En pausa")
                        : Controlador.estado === "grabandoAudio" ? qsTr("Grabando audio")
                        : Controlador.estado === "replay" ? qsTr("En memoria, sin guardar")
                        : Controlador.estado === "emitiendo" ? qsTr("Emitiendo en directo")
                        : qsTr("Grabando")
                    color: raiz.colorExplicacion
                }
                QQC2.Label {
                    // Cuanto queda de disco a ESTE ritmo. Las dos cifras son
                    // medidas: el sitio libre y los MB por minuto de lo ultimo
                    // que grabo esta maquina. Sin una de las dos no se dice nada.
                    Layout.alignment: Qt.AlignHCenter
                    visible: !raiz.emitiendo && Controlador.mbPorMinuto > 0
                             && Controlador.espacioLibreMb > 0
                    text: {
                        var minutos = Controlador.espacioLibreMb / Controlador.mbPorMinuto
                        if (minutos < 90) return qsTr("Caben %1 min más").arg(Math.round(minutos))
                        var h = Math.floor(minutos / 60)
                        var m = Math.round(minutos % 60)
                        return qsTr("Caben %1 h %2 min más").arg(h).arg(m)
                    }
                    color: raiz.colorExplicacion
                    font: Kirigami.Theme.smallFont
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Sistema.r13
                    spacing: Sistema.r8

                    QQC2.Button {
                        // GSR no pausa el modo audio-only: la pausa es del IPC
                        // de la pantalla. En audio y en repeticion no se ofrece.
                        visible: Controlador.estado !== "grabandoAudio"
                                 && Controlador.estado !== "replay"
                                 && Controlador.estado !== "emitiendo"
                        Layout.preferredHeight: Sistema.r34 + Sistema.r5
                        icon.name: Controlador.estado === "pausado"
                                   ? "media-playback-start" : "media-playback-pause"
                        text: Controlador.estado === "pausado" ? qsTr("Reanudar") : qsTr("Pausa")
                        onClicked: Controlador.estado === "pausado"
                                   ? Controlador.reanudar() : Controlador.pausar()
                    }
                    QQC2.Button {
                        // En modo repeticion el boton principal no es parar: es
                        // GUARDAR lo que hay en memoria, y seguir. Parar ahi no
                        // guarda nada, asi que si fuera lo unico a mano
                        // acabarias tirando lo que querias salvar.
                        id: botonPrincipalGrabando
                        Layout.fillWidth: true
                        Layout.preferredHeight: Sistema.r34 + Sistema.r5
                        readonly property bool esGuardarReplay: Controlador.estado === "replay"
                        text: esGuardarReplay ? qsTr("Guardar lo último")
                            : Controlador.estado === "emitiendo" ? qsTr("Terminar")
                            : qsTr("Parar y guardar")
                        onClicked: esGuardarReplay ? Controlador.guardarReplay()
                                                   : Controlador.parar()
                        contentItem: RowLayout {
                            spacing: Sistema.r5
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                implicitWidth: 11; implicitHeight: 11
                                radius: botonPrincipalGrabando.esGuardarReplay ? 2 : 0
                                color: "white"
                            }
                            QQC2.Label {
                                text: botonPrincipalGrabando.text
                                color: "white"
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                        }
                        background: Rectangle {
                            radius: Sistema.r5
                            color: botonPrincipalGrabando.down ? raiz.colorAccionPulsada
                                 : botonPrincipalGrabando.hovered
                                   ? Qt.lighter(raiz.colorAccion, 1.15)
                                   : raiz.colorAccion
                        }
                    }
                    QQC2.Button {
                        // En repeticion no hay fichero que tirar, y emitiendo
                        // tampoco: lo que salio por el cable ya no vuelve.
                        visible: Controlador.estado !== "replay"
                                 && Controlador.estado !== "emitiendo"
                        Layout.preferredHeight: Sistema.r34 + Sistema.r5
                        flat: true
                        text: qsTr("Descartar")
                        // En rojo y sin marco: destruye algo, asi que se lee
                        // distinto del resto sin gritar como un boton lleno.
                        contentItem: QQC2.Label {
                            text: qsTr("Descartar")
                            color: Sistema.grabando
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            // Bajo diez segundos no se pregunta: eso es «me he
                            // equivocado al empezar», y una confirmacion ahi
                            // estorba mas que protege. Por encima si, porque
                            // borrar una grabacion no tiene vuelta atras.
                            if (Controlador.segundos < 10) {
                                Controlador.descartar()
                            } else {
                                confirmarDescartar.open()
                            }
                        }
                    }
                }
                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    visible: Controlador.hayAtajos
                    text: Controlador.atajoPausa !== ""
                          ? qsTr("%1 para y guarda · %2 pausa")
                            .arg(Controlador.atajoGrabar).arg(Controlador.atajoPausa)
                          : qsTr("%1 para y guarda").arg(Controlador.atajoGrabar)
                    color: raiz.colorExplicacion
                    font: Kirigami.Theme.smallFont
                }
            }

            Item {
                visible: !raiz.grabando && Controlador.estado !== "detectando"
                Layout.fillHeight: true
            }
        }
    }
}
