// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// El titulo de una seccion, que ademas la pliega.
//
// Y que SIEMPRE dice lo que hay puesto dentro. Esa linea de resumen es la
// razon de que esto no sea el boton «Avanzado» que se quito en la 0.9.0:
// aquel escondia sin decir que escondia, y lo que se busco de verdad no se
// encontro. Aqui, plegado, se lee «Vídeo · Muy alta · 60 fps · h264», o sea
// mas de lo que se sabia antes sin bajar hasta alli.
//
// Existe porque la pagina medida daba 2567 px de contenido en un hueco de 445:
// cinco pantallas y pico de bajar para llegar a la ultima opcion. Plegadas,
// las seis secciones caben de una vez y la configuracion entera se lee de un
// vistazo.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import es.solucionesconscientes.esr

QQC2.ItemDelegate {
    id: cabecera

    property string titulo: ""
    // Lo que hay puesto ahi dentro, en una linea.
    property string resumen: ""
    // Con que nombre se recuerda si quedo abierta. Vacio, no se recuerda.
    property string clave: ""
    property bool abierta: false

    Layout.fillWidth: true
    Layout.topMargin: Sistema.r8
    padding: Sistema.r8
    leftPadding: Sistema.r5

    onClicked: {
        cabecera.abierta = !cabecera.abierta
        if (cabecera.clave !== "") {
            Controlador.recordarAjuste("abierta_" + cabecera.clave,
                                       cabecera.abierta ? "1" : "0")
        }
    }
    Component.onCompleted: {
        if (cabecera.clave !== "") {
            // Cerradas de serie: asi la primera pantalla es el mapa entero.
            cabecera.abierta = Controlador.ajusteRecordado(
                        "abierta_" + cabecera.clave, "0") === "1"
        }
    }

    contentItem: RowLayout {
        spacing: Sistema.r5

        Kirigami.Icon {
            // La flecha, a la izquierda del titulo: es lo que dice que esto se
            // abre. A la derecha se lee como adorno.
            source: cabecera.abierta ? "arrow-down-symbolic" : "arrow-right-symbolic"
            implicitWidth: Kirigami.Units.iconSizes.small
            implicitHeight: Kirigami.Units.iconSizes.small
            Layout.alignment: Qt.AlignVCenter
        }
        QQC2.Label {
            text: cabecera.titulo
            font.bold: true
        }
        QQC2.Label {
            // El resumen, gris y al lado, no debajo: asi una seccion plegada
            // ocupa una linea y las seis caben en la primera pantalla.
            Layout.fillWidth: true
            visible: text !== "" && !cabecera.abierta
            text: "· " + cabecera.resumen
            color: Kirigami.Theme.disabledTextColor
            elide: Text.ElideRight
        }
        Item { Layout.fillWidth: cabecera.abierta }
    }
}
