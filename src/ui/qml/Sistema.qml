// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// El sistema de diseño de la ventana. Cada numero sale de una serie y cada
// color de una medicion, para no volver a elegirlos a ojo cada vez.
//
// El ritmo es Fibonacci en pixeles. No es adorno: con una serie, todos los
// huecos de la aplicacion guardan la misma relacion entre si, y eso se nota
// aunque nadie sepa decir por que. Ademas dos de sus numeros ya son los de
// Kirigami —cornerRadius vale 5 y largeSpacing vale 8— asi que el ritmo
// nuestro y el nativo coinciden en vez de pelearse.
//
// Los colores de contraste estan MEDIDOS contra el fondo donde se usan, con la
// formula de WCAG. Los que traia Breeze por defecto no llegaban: blanco sobre
// el azul de acento da 2,49 y el minimo de AA son 4,5; el gris de las
// descripciones da 4,21. Los dos se corrigen aqui.
pragma Singleton
import QtQuick

QtObject {
    // --- Ritmo: Fibonacci ---------------------------------------------
    readonly property int r3: 3    // entre un texto y su explicacion
    readonly property int r5: 5    // radio de esquina
    readonly property int r8: 8    // entre controles de una misma fila
    readonly property int r13: 13  // entre tarjetas
    readonly property int r21: 21  // margen de la cabecera
    readonly property int r34: 34  // alto de un boton secundario
    readonly property int r55: 55  // alto del boton de grabar

    // --- Tipos: base 10 pt, paso raiz de phi (1,272) -------------------
    // 10 → 13 → 16 → 21 → 26 → 33. Los saltos grandes caen en 13, 21 y 34,
    // que son Fibonacci, y no por casualidad: las razones de Fibonacci
    // convergen en phi.
    readonly property int t1: 13   // el boton principal
    readonly property int t5: 33   // el reloj mientras graba

    // --- Proporciones --------------------------------------------------
    // La ventana de ajustes es 1:1,2722, que es la raiz de phi, y la de
    // grabar es 1:0,618, que es phi.
    //
    // El reparto aureo de la cabecera se probo y se quito: en 687 px de alto
    // le regalaba 41 px a una cabecera ya holgada y se los quitaba al hueco
    // desplazable. Cuando la proporcion y la medida no coinciden, manda la
    // medida.
    readonly property real raizPhi: 1.2720
    readonly property real phi: 1.6180

    // --- Color: solo significa, nunca decora ---------------------------
    // Rojo = hay una grabacion en curso, y en ningun otro sitio. Es el
    // ForegroundNegative de Breeze un punto mas oscuro, para que el blanco
    // de la pastilla de la bandeja pase AA (4,86 en vez de 4,26).
    readonly property color grabando: "#d62e3f"
    // Naranja = en pausa. Antes era gris, y el gris se lee como
    // «desactivado», que es otra cosa.
    readonly property color pausado: "#f67400"
    // El gris de las explicaciones, uno por tema. En claro 5,44 sobre la
    // tarjeta; en oscuro 5,25.
    readonly property color explicacionEnClaro: "#606b76"
    readonly property color explicacionEnOscuro: "#8e99a4"
}
