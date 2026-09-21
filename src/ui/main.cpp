// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

// La UI de Easy Screen Recorder. Ligera e intuitiva por contrato (CLAUDE.md): dos clics
// para grabar, todo lo demas plegado, y estetica nativa de Plasma.

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QMenu>
#include <QQuickWindow>
#include <QSystemTrayIcon>

#include <QElapsedTimer>

#include <cstdio>
#include <filesystem>

#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "atajos.hpp"
#include "controlador.hpp"
#include "esr/configuracion.hpp"
#include "reloj_bandeja.hpp"

namespace {

// Nuestros iconos viven en el tema solo despues de instalar. Desde el arbol de
// compilacion QIcon::fromTheme no los encuentra, asi que cada uno lleva detras
// el generico de Breeze que se usaba antes: instalado se ve la marca, sin
// instalar se ve algo. Un icono nulo en la bandeja es un hueco invisible y el
// usuario no sabe que la aplicacion sigue viva.
// Castellano si el sistema esta en castellano; ingles en cualquier otro caso.
//
// No hay un catalogo por idioma: las cadenas FUENTE estan en castellano y solo
// existe un catalogo, el ingles. Asi que la logica es al reves de lo habitual:
// no se carga nada para el castellano —las cadenas del codigo ya lo son— y se
// carga el catalogo para todo lo demas.
//
// «Todo lo demas» incluye idiomas que no son ingles, y es a proposito: para un
// hablante de aleman, el ingles es mucho mas util que un castellano que no
// entiende. La alternativa seria dejarlo en castellano por no tener su idioma,
// que es peor.
//
// Se mira uiLanguages() y no QLocale::system().name() porque la primera respeta
// el orden de preferencia del usuario cuando tiene varios idiomas puestos, que
// es lo que de verdad quiere decir «el idioma del sistema».
bool sistemaEnCastellano() {
    for (const QString& idioma : QLocale::system().uiLanguages()) {
        const QString base = idioma.left(2).toLower();
        if (base == QStringLiteral("es")) return true;
        // El primer idioma reconocible decide: si el usuario tiene ingles
        // delante y castellano detras, quiere ingles.
        if (base.size() == 2) return false;
    }
    return false;
}

QIcon iconoBandeja(bool grabando) {
    const QString nuestro = grabando
                                ? QStringLiteral("es.solucionesconscientes.EasyScreenRecorder-recording-symbolic")
                                : QStringLiteral("es.solucionesconscientes.EasyScreenRecorder-symbolic");
    const QString red = grabando ? QStringLiteral("media-record")
                                 : QStringLiteral("camera-video-symbolic");
    QIcon icono = QIcon::fromTheme(nuestro);
    return icono.isNull() ? QIcon::fromTheme(red) : icono;
}

// Los estados en los que hay algo en marcha. Es la misma lista que la ventana
// (Main.qml:22), y ahora esta escrita una sola vez aqui: la version de la
// bandeja se dejaba fuera «emitiendo», asi que emitiendo en directo ponia el
// icono de reposo y escondia «Terminar», que era justo lo que hacia falta para
// cortar el directo sin sacar la ventana.
bool enCurso(const QString& estado) {
    return estado == QStringLiteral("grabando") || estado == QStringLiteral("grabandoAudio") ||
           estado == QStringLiteral("pausado") || estado == QStringLiteral("replay") ||
           estado == QStringLiteral("emitiendo");
}

}  // namespace

int main(int argc, char** argv) {
    // El contrato de CLAUDE.md es arranque < 1 s, y eso se mide, no se
    // siente. Con ESR_MEDIR_ARRANQUE=1 se imprime el tiempo hasta el
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
    // La traduccion, antes de construir nada de interfaz: si se instala despues,
    // los textos ya compuestos se quedan en el idioma viejo.
    QTranslator traductor;
    if (!sistemaEnCastellano() &&
        traductor.load(QStringLiteral(":/i18n/easy-screen-recorder_en.qm"))) {
        app.installTranslator(&traductor);
    }
    // Y los textos que pone Qt —los botones de los dialogos de fichero, por
    // ejemplo— en el idioma del sistema, que los trae Qt ya traducidos.
    QTranslator traductorQt;
    if (traductorQt.load(QLocale::system(), QStringLiteral("qtbase"), QStringLiteral("_"),
                         QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&traductorQt);
    }

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

    if (ventana != nullptr && qEnvironmentVariableIsSet("ESR_MEDIR_ARRANQUE")) {
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

    // El reloj: un SEGUNDO item, al lado, que solo existe mientras hay algo en
    // marcha y solo si esta pedido. Va aparte y no pintado sobre el icono de la
    // aplicacion para que la marca siga reconocible justo cuando esta grabando,
    // que es cuando se la busca en el panel. Mientras esta apagado no se enseña,
    // asi que no hay un item registrado ni cuesta nada.
    QSystemTrayIcon reloj;

    // Y con menu, no solo con clic. Mientras se graba, la ventana esta
    // minimizada y apartada: si la unica forma de pausar fuera restaurarla, la
    // ventana entraria en el video justo en el momento en que uno intenta que
    // no salga. Desde aqui se pausa y se para sin que aparezca nada.
    QMenu menu;
    QAction* accion_mostrar = menu.addAction(QObject::tr("Mostrar la ventana"));
    menu.addSeparator();
    // Empezar desde aqui, sin sacar la ventana.
    //
    // Es la via buena y no un atajo de conveniencia: si la ventana ya esta
    // apartada cuando se pulsa, no hay nada que minimizar, asi que no hay
    // animacion que pueda colarse en el video ni cuenta atras que adivinar.
    // Empieza cuando se pulsa, y el icono de la bandeja lo confirma en el
    // instante exacto en que el grabador arranca.
    //
    // Graba el primer monitor con los defaults, lo mismo que el atajo global:
    // es el gesto de «graba ya», no el de «graba con lo que tengo configurado».
    // De ahi que el texto diga la pantalla y no «Grabar» a secas.
    QAction* accion_grabar = menu.addAction(QObject::tr("Grabar la pantalla"));
    menu.addSeparator();
    // En modo repeticion el que hace falta a mano es este: guardar lo que hay en
    // el buffer sin sacar la ventana, que es justo el momento en que uno no
    // quiere que aparezca nada en pantalla.
    QAction* accion_guardar = menu.addAction(QObject::tr("Guardar lo último"));
    QAction* accion_pausa = menu.addAction(QObject::tr("Pausa"));
    QAction* accion_parar = menu.addAction(QObject::tr("Parar y guardar"));
    menu.addSeparator();
    // La opcion vive AQUI y no en «Avanzado» de la ventana por dos razones: es
    // una preferencia de la bandeja, y «Avanzado» se deshabilita mientras se
    // graba, que es justo el momento en que a alguien se le ocurre que querria
    // ver el tiempo. Se recuerda entre sesiones.
    // Aqui no hace falta decir «en la bandeja»: se esta leyendo en la bandeja. En
    // la ventana si lo dice, porque alli hay que decir donde sale.
    // Cancelar la cuenta atras. Con la ventana ya apartada, su boton «Cancelar»
    // no esta a mano, y tres segundos son suficientes para arrepentirse.
    QAction* accion_cancelar = menu.addAction(QObject::tr("Cancelar la cuenta atrás"));
    accion_cancelar->setVisible(false);
    menu.addSeparator();
    QAction* accion_reloj = menu.addAction(QObject::tr("Mostrar el tiempo de grabación"));
    accion_reloj->setCheckable(true);
    accion_reloj->setChecked(controlador != nullptr ? controlador->relojEnBandeja()
                                                    : esr::reloj_bandeja_activo());
    menu.addSeparator();
    QAction* accion_salir = menu.addAction(QObject::tr("Salir"));
    bandeja.setContextMenu(&menu);
    // El reloj NO lleva menu, y no por quedarse corto: darle el mismo QMenu que
    // al otro item TUMBA la aplicacion. Medido, no supuesto: con las dos lineas
    // puestas, una grabacion de tres segundos por ESR_AUTOPRUEBA acaba en
    // violacion de segmento al salir, y por el camino el exportador escupe «No
    // id for action». El mismo QMenu exportado por dos items de bandeja se
    // reparte los identificadores de sus acciones y uno de los dos se queda con
    // punteros que ya no valen. Sin la linea, el mismo arnes sale con 0.
    //
    // Asi que el reloj es solo un numero: el menu esta a un icono de distancia,
    // en el de la aplicacion, que es donde ha estado siempre.

    // El icono no se enseña hasta aqui: con el menu ya puesto, el escritorio lo
    // recoge de una vez y no hay un instante con un icono sin menu detras.
    bandeja.show();

    const auto traer = [ventana] {
        if (ventana == nullptr) return;
        ventana->show();
        ventana->raise();
        ventana->requestActivate();
    };
    // Minimizada cuenta como apartada aunque `isVisible()` siga diciendo que
    // si: si no, el clic en la bandeja durante una grabacion la «ocultaba»
    // otra vez en vez de traerla.
    const auto a_la_vista = [ventana] {
        return ventana != nullptr && ventana->isVisible() &&
               ventana->visibility() != QWindow::Minimized;
    };

    if (ventana != nullptr) {
        QObject::connect(&bandeja, &QSystemTrayIcon::activated, ventana,
                         [ventana, traer, a_la_vista](QSystemTrayIcon::ActivationReason motivo) {
                             if (motivo != QSystemTrayIcon::Trigger) return;
                             if (a_la_vista()) {
                                 ventana->showMinimized();
                             } else {
                                 traer();
                             }
                         });
        QObject::connect(accion_mostrar, &QAction::triggered, ventana, traer);
    }
    QObject::connect(accion_salir, &QAction::triggered, &app, &QCoreApplication::quit);
    if (controlador != nullptr) {
        // alternarGrabacion es el mismo punto de entrada que el atajo global:
        // arranca si no hay nada y para si hay algo. Aqui solo se ofrece para
        // arrancar, porque parar ya tiene su propia entrada en el menu.
        QObject::connect(accion_grabar, &QAction::triggered, controlador,
                         [controlador] { controlador->alternarGrabacion(); });
    }

    if (controlador != nullptr) {
        QObject::connect(accion_pausa, &QAction::triggered, controlador, [controlador] {
            if (controlador->estado() == QStringLiteral("pausado")) {
                controlador->reanudar();
            } else {
                controlador->pausar();
            }
        });
        QObject::connect(accion_parar, &QAction::triggered, controlador, &Controlador::parar);
        QObject::connect(accion_guardar, &QAction::triggered, controlador,
                         &Controlador::guardarReplay);

        QObject::connect(controlador, &Controlador::grabacionGuardada, &bandeja,
                         [&bandeja](const QString& ruta) {
                             bandeja.showMessage(QObject::tr("Grabación guardada"), ruta,
                                                 QSystemTrayIcon::Information, 6000);
                         });
        // Y cuando la recompresion termina, el antes y el despues. Va a la
        // bandeja y no solo a la ventana porque esto acaba minutos despues, con
        // la ventana probablemente ya cerrada.
        QObject::connect(controlador, &Controlador::grabacionReducida, &bandeja,
                         [&bandeja](const QString& texto) {
                             bandeja.showMessage(QObject::tr("Grabación reducida"), texto,
                                                 QSystemTrayIcon::Information, 6000);
                         });
        // El reloj se repinta cada segundo, y solo si hay que enseñarlo. Es un
        // pixmap pequeño y una señal de DBus por segundo mientras se graba, el
        // mismo orden de trabajo que el reloj del propio panel.
        const auto pintar_reloj = [&reloj, controlador] {
            const QString estado = controlador->estado();
            const bool pausado = estado == QStringLiteral("pausado");
            // La cuenta atras manda sobre todo lo demas, y se enseña AUNQUE el
            // reloj este apagado: con la cuenta atras la ventana se aparta al
            // instante, asi que esto es lo unico que dice cuanto falta. Dura
            // tres segundos y se va sola.
            const int falta = controlador->cuentaAtras();
            if (falta > 0) {
                reloj.setIcon(esr::ui::iconoCuentaAtras(falta));
                reloj.setToolTip(QObject::tr("Empieza en %1…").arg(falta));
                if (!reloj.isVisible()) reloj.show();
                return;
            }
            if (!controlador->relojEnBandeja() || !enCurso(estado)) {
                if (reloj.isVisible()) reloj.hide();
                return;
            }
            const int segundos = controlador->segundos();
            reloj.setIcon(esr::ui::iconoReloj(segundos, pausado));
            // El tooltip lleva el tiempo entero, con horas si las hay: es el
            // sitio donde cabe lo que el icono tiene que recortar.
            reloj.setToolTip(pausado
                                 ? QObject::tr("En pausa · %1").arg(esr::ui::tiempoEscrito(segundos))
                                 : QObject::tr("Grabando · %1").arg(esr::ui::tiempoEscrito(segundos)));
            if (!reloj.isVisible()) reloj.show();
        };
        QObject::connect(controlador, &Controlador::segundosCambiados, &reloj, pintar_reloj);
        QObject::connect(controlador, &Controlador::cuentaAtrasCambiada, &reloj,
                         [controlador, accion_cancelar, pintar_reloj] {
                             accion_cancelar->setVisible(controlador->cuentaAtras() > 0);
                             pintar_reloj();
                         });
        QObject::connect(accion_cancelar, &QAction::triggered, controlador,
                         [controlador] { controlador->pedirCancelarCuentaAtras(); });
        // La casilla del menu y la de «Avanzado» son la MISMA preferencia, no dos
        // copias: las dos escriben en el controlador y las dos se enteran de lo
        // que haga la otra. Marcarla en un sitio y verla sin marcar en el otro
        // seria peor que tenerla en uno solo.
        QObject::connect(accion_reloj, &QAction::toggled, controlador,
                         [controlador](bool si) { controlador->ponerRelojEnBandeja(si); });
        QObject::connect(controlador, &Controlador::relojEnBandejaCambiado, &reloj,
                         [controlador, accion_reloj, pintar_reloj] {
                             accion_reloj->setChecked(controlador->relojEnBandeja());
                             pintar_reloj();
                         });

        const auto refrescar = [&bandeja, controlador, accion_pausa, accion_parar,
                                accion_guardar, accion_grabar, pintar_reloj] {
            const QString estado = controlador->estado();
            const bool pausado = estado == QStringLiteral("pausado");
            const bool audio = estado == QStringLiteral("grabandoAudio");
            const bool es_replay = estado == QStringLiteral("replay");
            const bool emitiendo = estado == QStringLiteral("emitiendo");
            const bool grabando = enCurso(estado);
            // Grabar solo cuando no hay nada en marcha: con una grabacion
            // encima, lo que hace falta es pararla, y esas acciones ya estan.
            accion_grabar->setVisible(!grabando && estado == QStringLiteral("listo"));
            // El atajo, escrito en el menu. Es donde alguien lo va a descubrir:
            // la ventana tambien lo dice, pero la ventana es justo lo que no
            // esta delante cuando hace falta.
            const QString atajo = controlador->atajoGrabar();
            accion_grabar->setText(atajo.isEmpty()
                                       ? QObject::tr("Grabar la pantalla")
                                       : QObject::tr("Grabar la pantalla (%1)").arg(atajo));
            accion_guardar->setVisible(es_replay);
            bandeja.setIcon(iconoBandeja(grabando));
            bandeja.setToolTip(grabando ? QObject::tr("Easy Screen Recorder: grabando")
                                        : QStringLiteral("Easy Screen Recorder"));
            // GSR no pausa el modo audio-only: la pausa es del IPC de la
            // pantalla. Emitiendo la acepta pero corta la imagen del directo
            // (docs/emision.md). Igual que en la ventana, en ninguno de los tres
            // se ofrece.
            accion_pausa->setVisible(grabando && !audio && !es_replay && !emitiendo);
            accion_pausa->setText(pausado ? QObject::tr("Reanudar") : QObject::tr("Pausa"));
            // En repeticion y emitiendo, parar no guarda nada: el texto tiene que
            // decirlo, y lo dice igual que el boton de la ventana.
            accion_parar->setVisible(grabando);
            accion_parar->setText(es_replay || emitiendo ? QObject::tr("Terminar")
                                                        : QObject::tr("Parar y guardar"));
            pintar_reloj();
        };
        QObject::connect(controlador, &Controlador::estadoCambiado, &bandeja, refrescar);
        // Y una vez ya, porque la aplicacion puede arrancar con una grabacion
        // en marcha que lanzo el CLI.
        refrescar();
    }

    // Cerrar la ventana sin grabacion sale del todo; el QML gestiona el
    // resto. Sin esto, Qt saldria al ocultar la ventana al empezar a grabar.
    app.setQuitOnLastWindowClosed(false);

    // El atajo global (Meta+Shift+R por defecto, cambiable en Preferencias
    // del sistema). Fuera de KDE no se registra y no pasa nada.
    AtajosGlobales atajos(controlador);
    // La interfaz solo anuncia los atajos si se han registrado de verdad.
    if (controlador != nullptr) {
        controlador->ponerAtajos(atajos.registrado(), atajos.atajoGrabar(), atajos.atajoPausa());
    }

    return app.exec();
}
