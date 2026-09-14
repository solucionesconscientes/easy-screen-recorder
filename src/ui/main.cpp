// La UI de Capturia. Ligera e intuitiva por contrato (CLAUDE.md): dos clics
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

#include "controlador.hpp"

int main(int argc, char** argv) {
    // El contrato de CLAUDE.md es arranque < 1 s, y eso se mide, no se
    // siente. Con CAPTURIA_MEDIR_ARRANQUE=1 se imprime el tiempo hasta el
    // primer frame pintado y se sale: scripts/medir-arranque.sh lo usa.
    QElapsedTimer cronometro;
    cronometro.start();
    // QApplication y no QGuiApplication: la bandeja del sistema
    // (QSystemTrayIcon) vive en QtWidgets. En Plasma sale como
    // StatusNotifierItem, que es lo nativo.
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("capturia"));
    app.setApplicationName(QStringLiteral("capturia"));
    app.setDesktopFileName(QStringLiteral("org.capturia.Capturia"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("media-record")));

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
    motor.loadFromModule("org.capturia", "Main");
    if (motor.rootObjects().isEmpty()) return 1;

    auto* ventana = qobject_cast<QQuickWindow*>(motor.rootObjects().first());
    auto* controlador = motor.singletonInstance<Controlador*>("org.capturia", "Controlador");

    if (ventana != nullptr && qEnvironmentVariableIsSet("CAPTURIA_MEDIR_ARRANQUE")) {
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
    QSystemTrayIcon bandeja(QIcon::fromTheme(QStringLiteral("camera-video-symbolic")));
    bandeja.setToolTip(QStringLiteral("Capturia"));
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
                             bandeja.setIcon(QIcon::fromTheme(
                                 grabando ? QStringLiteral("media-record")
                                          : QStringLiteral("camera-video-symbolic")));
                             bandeja.setToolTip(grabando
                                                    ? QStringLiteral("Capturia: grabando")
                                                    : QStringLiteral("Capturia"));
                         });
    }

    // Cerrar la ventana sin grabacion sale del todo; el QML gestiona el
    // resto. Sin esto, Qt saldria al ocultar la ventana al empezar a grabar.
    app.setQuitOnLastWindowClosed(false);

    return app.exec();
}
