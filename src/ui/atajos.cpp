#include "atajos.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QKeySequence>

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

// SetPresent: el valor 2 de la enum SetShortcutFlag de KGlobalAccel. Activa
// el atajo ademas de apuntarlo.
constexpr uint kActivarAtajo = 2;

}  // namespace

AtajosGlobales::AtajosGlobales(Controlador* controlador, QObject* padre)
    : QObject(padre), controlador_(controlador) {
    QDBusInterface servicio(QStringLiteral("org.kde.kglobalaccel"),
                            QStringLiteral("/kglobalaccel"),
                            QStringLiteral("org.kde.KGlobalAccel"));
    if (!servicio.isValid()) return;  // fuera de KDE: sin atajos y sin drama

    servicio.call(QStringLiteral("doRegister"), kAccionGrabarParar);

    // Meta+Shift+R. Si el usuario ya lo cambio en Preferencias, ese cambio
    // esta guardado en kglobalaccel y este default no lo pisa (SetPresent no
    // trae NoAutoloading).
    const QKeySequence secuencia(QStringLiteral("Meta+Shift+R"));
    const QList<int> teclas = {secuencia[0].toCombined()};
    const QDBusReply<QList<int>> concedidas = servicio.call(
        QStringLiteral("setShortcut"), kAccionGrabarParar, QVariant::fromValue(teclas),
        kActivarAtajo);

    registrado_ = concedidas.isValid() && !concedidas.value().isEmpty() &&
                  concedidas.value().first() != 0;

    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.kde.kglobalaccel"), QStringLiteral("/component/easy-screen-recorder"),
        QStringLiteral("org.kde.kglobalaccel.Component"),
        QStringLiteral("globalShortcutPressed"), this,
        SLOT(alPulsar(QString, QString, qlonglong)));
}

void AtajosGlobales::alPulsar(const QString& componente, const QString& accion,
                              qlonglong instante) {
    Q_UNUSED(instante);
    if (componente != QStringLiteral("easy-screen-recorder")) return;
    if (accion == QStringLiteral("grabar_parar")) controlador_->alternarGrabacion();
}
