// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atajos.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QKeySequence>
#include <QStringList>

#include "controlador.hpp"

namespace {

// El identificador de accion de KGlobalAccel: componente, accion y sus dos
// nombres visibles. Con esto el atajo aparece en Preferencias del sistema >
// Atajos, donde el usuario puede cambiarlo. Ahi esta la personalizacion:
// Easy Screen Recorder no necesita su propia pantalla de atajos.
const QStringList kAccionGrabarParar = {
    QStringLiteral("easy-screen-recorder"),
    QStringLiteral("grabar_parar"),
    QStringLiteral("Easy Screen Recorder"),
    QStringLiteral("Empezar o parar la grabación"),
};

// Y el de pausa, que es el que de verdad hace falta a mitad de grabacion.
//
// Sin el, pausar obligaba a sacar la ventana a la pantalla que se esta
// grabando, que es justo lo que no se quiere. Con la bandeja se podia, pero
// hay que ir a buscarla con el raton y eso tambien sale en el video.
const QStringList kAccionPausa = {
    QStringLiteral("easy-screen-recorder"),
    QStringLiteral("pausar_reanudar"),
    QStringLiteral("Easy Screen Recorder"),
    QStringLiteral("Pausar o reanudar la grabación"),
};

// SetPresent: activa el atajo ademas de apuntarlo. Respeta lo que el usuario
// tuviera guardado, porque no lleva NoAutoloading.
constexpr uint kActivarAtajo = 2;

// SetPresent | NoAutoloading: ademas, IGNORA lo guardado y pone lo que se pide.
//
// Hace falta para un caso concreto y bastante comun: una accion registrada
// alguna vez y guardada SIN tecla. Con solo SetPresent, kglobalaccel carga ese
// «sin tecla» y rechaza cualquier default, asi que la accion se queda para
// siempre sin nada que la dispare. Medido: con la bandera 2 no se concedia ni
// una combinacion libre; con la 4, si.
//
// Por eso se usa en una SEGUNDA pasada y no en la primera: forzar de entrada
// pisaria el atajo que el usuario hubiera elegido a mano.
constexpr uint kForzarAtajo = 2 | 4;

// El camino DBus del componente, que NO es «/component/<nombre>» sin mas.
//
// Un camino de DBus solo admite letras, digitos y guion bajo, asi que
// kglobalaccel sustituye todo lo demas. Nuestro componente se llama
// «easy-screen-recorder» y su camino real es «/component/easy_screen_recorder»,
// comprobado con `busctl --user tree org.kde.kglobalaccel`.
//
// Esto estuvo ROTO y sin que nadie se enterara. El componente se llamaba
// «capturia», que no tiene guiones, asi que el camino coincidia y el atajo
// funcionaba; al renombrar el proyecto aparecieron los guiones, el camino dejo
// de ser valido, la conexion a la señal fallo en silencio y el atajo no volvio a
// disparar nada. Se registraba, salia en Preferencias del sistema, y no hacia
// nada al pulsarlo.
//
// Se calcula en vez de escribirse a mano para que el proximo renombrado no
// vuelva a romperlo.
QString caminoComponente(const QString& nombre) {
    QString camino;
    camino.reserve(nombre.size());
    for (const QChar c : nombre) {
        camino += (c.isLetterOrNumber() && c.unicode() < 128) || c == QLatin1Char('_')
                      ? c
                      : QLatin1Char('_');
    }
    return QStringLiteral("/component/") + camino;
}

}  // namespace

AtajosGlobales::AtajosGlobales(Controlador* controlador, QObject* padre)
    : QObject(padre), controlador_(controlador) {
    QDBusInterface servicio(QStringLiteral("org.kde.kglobalaccel"),
                            QStringLiteral("/kglobalaccel"),
                            QStringLiteral("org.kde.KGlobalAccel"));
    if (!servicio.isValid()) return;  // fuera de KDE: sin atajos y sin drama

    // Se prueban VARIAS combinaciones y se coge la primera libre.
    //
    // Con una sola, esto estaba roto en esta maquina y en cualquiera con KDE
    // completo: Meta+Shift+R lo tiene Spectacle para «Iniciar/detener grabacion
    // de region», y tambien Meta+Alt+R y Meta+Ctrl+R. kglobalaccel no da una
    // tecla ocupada: se quedaba sin asignar, en silencio, y la accion existia
    // sin ninguna tecla que la disparara.
    //
    // Si el usuario ya eligio la suya, kglobalaccel la conserva y esto no la
    // pisa; por eso al final se PREGUNTA cual ha quedado en vez de suponerlo.
    const auto pedir = [&servicio](const QStringList& accion, const QString& candidata,
                                   uint bandera) {
        const QKeySequence secuencia(candidata);
        const QList<int> teclas = {secuencia[0].toCombined()};
        const QDBusReply<QList<int>> concedidas = servicio.call(
            QStringLiteral("setShortcut"), accion, QVariant::fromValue(teclas), bandera);
        // Lo que decide es lo que kglobalaccel CONCEDE, no que la llamada no
        // diera error: pedir una tecla ocupada devuelve una lista con un cero y
        // ni se queja. Dar eso por bueno era quedarse con la primera candidata y
        // no llegar nunca a las demas.
        return concedidas.isValid() && !concedidas.value().isEmpty() &&
               concedidas.value().first() != 0;
    };

    const auto registrar = [&servicio, &pedir](const QStringList& accion,
                                               const QStringList& candidatas) {
        servicio.call(QStringLiteral("doRegister"), accion);
        // Primera pasada, respetando lo guardado: si el usuario ya eligio una
        // tecla, esto la conserva y no se toca nada mas.
        for (const QString& candidata : candidatas) {
            if (pedir(accion, candidata, kActivarAtajo)) return;
        }
        // Segunda, forzando: se llega aqui cuando la accion esta guardada sin
        // tecla, que es un estado del que no se sale pidiendo por las buenas.
        for (const QString& candidata : candidatas) {
            if (pedir(accion, candidata, kForzarAtajo)) return;
        }
    };

    // La R es la intuitiva y va primero; las demas son el plan B para cuando
    // otra aplicacion ya se la quedo.
    registrar(kAccionGrabarParar, {QStringLiteral("Meta+Shift+R"),
                                   QStringLiteral("Meta+Alt+R"),
                                   QStringLiteral("Meta+Shift+G"),
                                   QStringLiteral("Meta+Ctrl+G")});
    registrar(kAccionPausa, {QStringLiteral("Meta+Shift+P"),
                             QStringLiteral("Meta+Alt+P"),
                             QStringLiteral("Meta+Ctrl+P")});

    // Y se pregunta cual ha quedado. No se da por hecho el que acabamos de
    // pedir: si el usuario ya lo habia cambiado, KDE conserva el suyo y es ese
    // el que hay que enseñar.
    const auto leerAtajo = [&servicio](const QStringList& accion) {
        const QDBusReply<QList<int>> teclas =
            servicio.call(QStringLiteral("shortcut"), QVariant::fromValue(accion));
        if (!teclas.isValid() || teclas.value().isEmpty() || teclas.value().first() == 0) {
            return QString();
        }
        return QKeySequence(teclas.value().first()).toString(QKeySequence::NativeText);
    };
    atajo_grabar_ = leerAtajo(kAccionGrabarParar);
    atajo_pausa_ = leerAtajo(kAccionPausa);

    const bool conectado = QDBusConnection::sessionBus().connect(
        QStringLiteral("org.kde.kglobalaccel"), caminoComponente(kAccionGrabarParar.at(0)),
        QStringLiteral("org.kde.kglobalaccel.Component"),
        QStringLiteral("globalShortcutPressed"), this,
        SLOT(alPulsar(QString, QString, qlonglong)));
    // «Registrado» significa que las pulsaciones LLEGAN, no que el atajo figure
    // en una lista. El estado roto de antes era justo ese: salia en Preferencias
    // del sistema y no hacia nada, porque la señal no estaba conectada.
    registrado_ = conectado && !atajo_grabar_.isEmpty();
}

void AtajosGlobales::alPulsar(const QString& componente, const QString& accion,
                              qlonglong instante) {
    Q_UNUSED(instante);
    if (componente != QStringLiteral("easy-screen-recorder")) return;
    if (accion == QStringLiteral("grabar_parar")) controlador_->alternarGrabacion();
    if (accion == QStringLiteral("pausar_reanudar")) controlador_->alternarPausa();
}
