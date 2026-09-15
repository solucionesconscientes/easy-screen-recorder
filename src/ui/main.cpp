// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// La UI de Easy Screen Recorder. Ligera e intuitiva por contrato (CLAUDE.md): dos clics
// para grabar, todo lo demas plegado, y estetica nativa de Plasma.

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>

#include <QElapsedTimer>

#include <cstdio>
#include <filesystem>

#include "atajos.hpp"
#include "controlador.hpp"

namespace {

// Nuestros iconos viven en el tema solo despues de instalar. Desde el arbol de
// compilacion QIcon::fromTheme no los encuentra, asi que cada uno lleva detras
// el generico de Breeze que se usaba antes: instalado se ve la marca, sin
// instalar se ve algo. Un icono nulo en la bandeja es un hueco invisible y el
// usuario no sabe que la aplicacion sigue viva.
QIcon iconoBandeja(bool grabando) {
    const QString nuestro = grabando
                                ? QStringLiteral("es.solucionesconscientes.EasyScreenRecorder-recording-symbolic")
                                : QStringLiteral("es.solucionesconscientes.EasyScreenRecorder-symbolic");
    const QString red = grabando ? QStringLiteral("media-record")
                                 : QStringLiteral("camera-video-symbolic");
    QIcon icono = QIcon::fromTheme(nuestro);
    return icono.isNull() ? QIcon::fromTheme(red) : icono;
}

}  // namespace

int main(int argc, char** argv) {
    // El contrato de CLAUDE.md es arranque < 1 s, y eso se mide, no se
    // siente. Con EASY SCREEN RECORDER_MEDIR_ARRANQUE=1 se imprime el tiempo hasta el
    // primer frame pintado y se sale: scripts/medir-arranque.sh lo usa.
    QElapsedTimer cronometro;
    cronometro.start();
    // QApplication y no QGuiApplication: la bandeja del sistema
    // (QSystemTrayIcon) vive en QtWidgets. En Plasma sale como
    // StatusNotifierItem, que es lo nativo.
    QApplication app(argc, argv);
    // Forma slug a proposito: esto son componentes de ruta de QStandardPaths y
    // claves de registro, no texto visible. El nombre bonito va en el .desktop
    // y en el metainfo.
    app.setOrganizationName(QStringLiteral("solucionesconscientes"));
    app.setOrganizationDomain(QStringLiteral("solucionesconscientes.es"));
    app.setApplicationName(QStringLiteral("easy-screen-recorder"));
    app.setDesktopFileName(QStringLiteral("es.solucionesconscientes.EasyScreenRecorder"));
    const QIcon iconoApp = QIcon::fromTheme(QStringLiteral("es.solucionesconscientes.EasyScreenRecorder"));
    app.setWindowIcon(iconoApp.isNull() ? QIcon::fromTheme(QStringLiteral("media-record"))
                                        : iconoApp);

    // El estilo del escritorio, si esta. Forzarlo a ciegas rompe fuera de
    // Plasma; mirar el modulo en disco es barato y honesto.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        std::error_code ec;
        if (std::filesystem::is_directory("/usr/lib/x86_64-linux-gnu/qt6/qml/org/kde/desktop",
                                          ec)) {
            QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
        }
    }

    QQmlApplicationEngine motor;
    QObject::connect(
        &motor, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    motor.loadFromModule("es.solucionesconscientes.esr", "Main");
    if (motor.rootObjects().isEmpty()) return 1;

    auto* ventana = qobject_cast<QQuickWindow*>(motor.rootObjects().first());
    auto* controlador = motor.singletonInstance<Controlador*>("es.solucionesconscientes.esr", "Controlador");

    if (ventana != nullptr && qEnvironmentVariableIsSet("EASY SCREEN RECORDER_MEDIR_ARRANQUE")) {
        QObject::connect(ventana, &QQuickWindow::frameSwapped, &app,
                         [&cronometro] {
                             std::printf("primer frame: %lld ms\n",
                                         static_cast<long long>(cronometro.elapsed()));
                             QCoreApplication::exit(0);
                         },
                         static_cast<Qt::ConnectionType>(Qt::QueuedConnection |
                                                         Qt::SingleShotConnection));
    }

    // La bandeja: el acceso permanente y el indicador de grabacion. El punto
    // rojo SOLO cuando se graba: un indicador siempre encendido miente.
    QSystemTrayIcon bandeja(iconoBandeja(false));
    bandeja.setToolTip(QStringLiteral("Easy Screen Recorder"));
    bandeja.show();

    if (ventana != nullptr) {
        QObject::connect(&bandeja, &QSystemTrayIcon::activated, ventana,
                         [ventana](QSystemTrayIcon::ActivationReason motivo) {
                             if (motivo != QSystemTrayIcon::Trigger) return;
                             if (ventana->isVisible()) {
                                 ventana->hide();
                             } else {
                                 ventana->show();
                                 ventana->raise();
                                 ventana->requestActivate();
                             }
                         });
    }
    if (controlador != nullptr) {
        QObject::connect(controlador, &Controlador::grabacionGuardada, &bandeja,
                         [&bandeja](const QString& ruta) {
                             bandeja.showMessage(QStringLiteral("Grabación guardada"), ruta,
                                                 QSystemTrayIcon::Information, 6000);
                         });
        QObject::connect(controlador, &Controlador::estadoCambiado, &bandeja,
                         [&bandeja, controlador] {
                             const QString estado = controlador->estado();
                             const bool grabando = estado == QStringLiteral("grabando") ||
                                                   estado == QStringLiteral("grabandoAudio") ||
                                                   estado == QStringLiteral("pausado");
                             bandeja.setIcon(iconoBandeja(grabando));
                             bandeja.setToolTip(grabando
                                                    ? QStringLiteral("Easy Screen Recorder: grabando")
                                                    : QStringLiteral("Easy Screen Recorder"));
                         });
    }

    // Cerrar la ventana sin grabacion sale del todo; el QML gestiona el
    // resto. Sin esto, Qt saldria al ocultar la ventana al empezar a grabar.
    app.setQuitOnLastWindowClosed(false);

    // El atajo global (Meta+Shift+R por defecto, cambiable en Preferencias
    // del sistema). Fuera de KDE no se registra y no pasa nada.
    AtajosGlobales atajos(controlador);

    return app.exec();
}
