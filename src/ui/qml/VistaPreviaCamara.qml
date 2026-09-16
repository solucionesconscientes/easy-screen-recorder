// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// La vista previa de la camara, para encuadrarte ANTES de grabar.
//
// Por que solo antes: la camara admite un unico cliente. Lo dice el manual de
// GSR —«Other applications can't use the camera when GPU Screen Recorder is
// using the camera»— y esta medido en esta maquina: un segundo proceso recibe
// «Device or resource busy». Asi que mientras se graba, la camara es de GSR y
// aqui no hay imagen. Lo que importa es que el encuadre estuviera bien antes de
// empezar, y para eso sirve esto.
//
// Este fichero solo entra en el modulo si Qt6Multimedia esta disponible; el
// CMakeLists lo añade condicionalmente. Sin el, el Loader que lo carga nunca se
// activa y no hay ni un aviso por consola.
import QtQuick
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami

Item {
    id: raiz
    // La ruta del dispositivo, «/dev/video0». Vacia = apagar la camara.
    property string dispositivo: ""
    // Espejo, para verte como te ves en un espejo. Es cosmetica de la vista
    // previa y ademas lo que se va a grabar si el espejo esta puesto.
    property bool espejo: true
    // Si hay que soltar la camara ya: al empezar a grabar, GSR la necesita.
    property bool activa: true

    implicitHeight: Kirigami.Units.gridUnit * 7

    readonly property bool encendida: activa && dispositivo !== ""

    CaptureSession {
        camera: Camera {
            id: dispositivoCamara
            active: raiz.encendida
            cameraDevice: {
                var lista = mediaDevices.videoInputs
                for (var i = 0; i < lista.length; ++i) {
                    // El id de Qt para una camara V4L2 es su ruta de
                    // dispositivo, que es el mismo identificador que usa GSR.
                    if (String(lista[i].id) === raiz.dispositivo) return lista[i]
                }
                return mediaDevices.defaultVideoInput
            }
        }
        videoOutput: salida
    }

    MediaDevices { id: mediaDevices }

    VideoOutput {
        id: salida
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
        // El volteo se hace aqui y no en el flujo: es solo como se mira.
        transform: Scale {
            origin.x: salida.width / 2
            xScale: raiz.espejo ? -1 : 1
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: !raiz.encendida
        color: Kirigami.Theme.alternateBackgroundColor
        radius: Kirigami.Units.smallSpacing
        Kirigami.Heading {
            anchors.centerIn: parent
            level: 5
            opacity: 0.6
            horizontalAlignment: Text.AlignHCenter
            text: raiz.dispositivo === "" ? qsTr("Sin cámara")
                                          : qsTr("La cámara la tiene la grabación")
        }
    }
}
