#include "controlador.hpp"

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

#include <cstdio>
#include <ctime>

#include "capturia/ajustes.hpp"
#include "capturia/audio.hpp"
#include "capturia/grabacion.hpp"

namespace {

// Las dos entradas de solo-audio van en la misma lista que las fuentes de
// pantalla: asi "dos clics" vale tambien para una nota de voz. Llevan prefijo
// para que el controlador sepa por que via van.
const QString kAudioSistema = QStringLiteral("Solo audio: lo que suena");
const QString kAudioMicro = QStringLiteral("Solo audio: micrófono");

}  // namespace

Controlador::Controlador(QObject* padre) : QObject(padre) {
    reloj_.setInterval(1000);
    connect(&reloj_, &QTimer::timeout, this, [this] {
        ++segundos_;
        emit segundosCambiados();
    });

    // La ventana pinta ya; la deteccion (0,7-0,9 s con el flatpak, medido)
    // llega por detras. Es la via honesta de cumplir "arranque < 1 s" sin
    // cachear capacidades que dependen del estado de la sesion.
    detectarEnSegundoPlano();

    // Si la UI arranca con una grabacion ya en marcha (lanzada por el CLI o
    // por una sesion anterior), lo dice en vez de fingir que no existe.
    const auto sesion = capturia::sesion_por_defecto();
    const auto sesion_audio = capturia::sesion_audio_por_defecto();
    if (capturia::grabacion_en_marcha(sesion)) {
        // El reloj parte del inicio real, no de cero: la grabacion pudo
        // arrancarla el CLI hace rato.
        const long inicio = capturia::inicio_grabacion(sesion.ruta_inicio);
        if (inicio > 0) segundos_ = static_cast<int>(std::time(nullptr) - inicio);
        ponerEstado(QStringLiteral("grabando"));
        reloj_.start();
    } else if (capturia::audio_en_marcha(sesion_audio)) {
        const long inicio = capturia::inicio_grabacion(sesion_audio.ruta_inicio);
        if (inicio > 0) segundos_ = static_cast<int>(std::time(nullptr) - inicio);
        audio_en_curso_ = true;
        ponerEstado(QStringLiteral("grabandoAudio"));
        reloj_.start();
    }
}

// Autoprueba del camino real del controlador, para el arnes: con
// CAPTURIA_AUTOPRUEBA=<fuente> graba unos segundos por el mismo codigo que
// dispara el boton, para, imprime la ruta y sale. No es una feature: existe
// porque el clic no se puede automatizar sin inyeccion de entrada y este
// camino (hilos + señales) no se verifica compilando.
void Controlador::autoprueba(const QString& fuente) {
    connect(this, &Controlador::grabacionGuardada, this, [](const QString& ruta) {
        std::printf("autoprueba guardo: %s\n", ruta.toUtf8().constData());
        QCoreApplication::exit(0);
    });
    connect(this, &Controlador::errorCambiado, this, [this] {
        if (error_.isEmpty()) return;
        std::fprintf(stderr, "autoprueba fallo: %s\n", error_.toUtf8().constData());
        QCoreApplication::exit(1);
    });
    connect(this, &Controlador::estadoCambiado, this, [this] {
        if (estado_ == QStringLiteral("grabando") || estado_ == QStringLiteral("grabandoAudio")) {
            QTimer::singleShot(3000, this, [this] { parar(); });
        }
    });
    grabar(fuente, QStringLiteral("very_high"), 60, QStringLiteral("sistema"));
}

void Controlador::detectarEnSegundoPlano() {
    auto* vigilante = new QFutureWatcher<capturia::Entorno>(this);
    connect(vigilante, &QFutureWatcher<capturia::Entorno>::finished, this, [this, vigilante] {
        aplicarEntorno(vigilante->result());
        vigilante->deleteLater();
        const QByteArray fuente = qgetenv("CAPTURIA_AUTOPRUEBA");
        if (!fuente.isEmpty() && estado_ == QStringLiteral("listo")) {
            autoprueba(QString::fromUtf8(fuente));
        }
    });
    vigilante->setFuture(QtConcurrent::run([] { return capturia::detectar(); }));
}

void Controlador::aplicarEntorno(const capturia::Entorno& e) {
    fuentes_.clear();
    for (const auto& f : e.capacidades.fuentes_captura) {
        QString etiqueta = QString::fromStdString(f.id);
        // Una camara con N modos es UNA fuente para el usuario; el modo lo
        // elige GSR. Sin esto la lista enseñaba /dev/video0 ocho veces.
        if (fuentes_.contains(etiqueta)) continue;
        fuentes_ << etiqueta;
    }
    if (e.graba_audio_solo()) {
        fuentes_ << kAudioSistema << kAudioMicro;
    }

    if (!e.gsr.presente) {
        diagnostico_ = QStringLiteral(
            "gpu-screen-recorder no está instalado. Sin él no hay grabación de pantalla.\n"
            "Instálalo nativo o como flatpak y vuelve a abrir Capturia.");
        ponerEstado(e.graba_audio_solo() ? QStringLiteral("listo") : QStringLiteral("sinGsr"));
        emit fuentesCambiadas();
        return;
    }

    QStringList carencias;
    for (const auto& c : e.carencias) {
        carencias << QString::fromStdString(c.que);
    }
    diagnostico_ = carencias.join(QStringLiteral("\n"));

    emit fuentesCambiadas();
    if (estado_ == QStringLiteral("detectando")) ponerEstado(QStringLiteral("listo"));
}

void Controlador::grabar(const QString& fuente, const QString& calidad, int fps,
                         const QString& audio) {
    ponerError({});
    ruta_guardada_.clear();
    emit rutaGuardadaCambiada();

    // Las dos entradas de solo-audio van por ffmpeg, no por GSR.
    if (fuente == kAudioSistema || fuente == kAudioMicro) {
        capturia::AjustesAudio a;
        a.dispositivo = fuente == kAudioMicro ? "default_input" : "default_output";
        a.salida = capturia::nombre_por_defecto(capturia::carpeta_musica(), "opus");
        const auto r = capturia::empezar_audio(a, capturia::sesion_audio_por_defecto());
        if (!r.bien) {
            ponerError(QString::fromStdString(r.motivo));
            return;
        }
        audio_en_curso_ = true;
        segundos_ = 0;
        emit segundosCambiados();
        reloj_.start();
        ponerEstado(QStringLiteral("grabandoAudio"));
        return;
    }

    capturia::AjustesGrabacion a;
    a.fuente = fuente.toStdString();
    a.salida = capturia::nombre_por_defecto(capturia::carpeta_videos());
    a.calidad = calidad.toStdString();
    a.fps = fps;
    if (audio == QStringLiteral("micro")) {
        a.audios = {"default_input"};
    } else if (audio == QStringLiteral("ambos")) {
        // Dos pistas separadas, no mezcladas: mezclar con «a|b» cambiaria
        // FLAC a Opus por detras (codec_select.c:186-191) y separadas se
        // pueden equilibrar despues.
        a.audios = {"default_output", "default_input"};
    } else if (audio == QStringLiteral("nada")) {
        a.audios.clear();
        a.sin_audio = true;
    }

    // empezar_grabacion espera al socket del grabador (hasta 15 s si el
    // flatpak arranca frio), asi que fuera del hilo de la ventana.
    ponerEstado(QStringLiteral("arrancando"));
    auto* vigilante = new QFutureWatcher<capturia::ResultadoLanzamiento>(this);
    connect(vigilante, &QFutureWatcher<capturia::ResultadoLanzamiento>::finished, this,
            [this, vigilante] {
                const auto r = vigilante->result();
                vigilante->deleteLater();
                if (!r.en_marcha) {
                    ponerError(QString::fromStdString(r.motivo));
                    ponerEstado(QStringLiteral("listo"));
                    return;
                }
                audio_en_curso_ = false;
                segundos_ = 0;
                emit segundosCambiados();
                reloj_.start();
                ponerEstado(QStringLiteral("grabando"));
            });
    vigilante->setFuture(QtConcurrent::run(
        [a] { return capturia::empezar_grabacion(a, capturia::sesion_por_defecto()); }));
}

void Controlador::parar() {
    reloj_.stop();
    ponerEstado(QStringLiteral("guardando"));

    const bool era_audio = audio_en_curso_;
    // La respuesta del stop llega cuando el fichero esta escrito, sin limite
    // de tiempo (docs/gsr-ipc.md). Jamas en el hilo de la ventana.
    auto* vigilante = new QFutureWatcher<QPair<bool, QString>>(this);
    connect(vigilante, &QFutureWatcher<QPair<bool, QString>>::finished, this,
            [this, vigilante] {
                const auto [bien, texto] = vigilante->result();
                vigilante->deleteLater();
                if (!bien) {
                    ponerError(texto);
                    ponerEstado(QStringLiteral("listo"));
                    return;
                }
                ruta_guardada_ = texto;
                emit rutaGuardadaCambiada();
                emit grabacionGuardada(texto);
                ponerEstado(QStringLiteral("listo"));
            });
    vigilante->setFuture(QtConcurrent::run([era_audio]() -> QPair<bool, QString> {
        if (era_audio) {
            const auto r = capturia::parar_audio(capturia::sesion_audio_por_defecto());
            return {r.bien, QString::fromStdString(r.bien ? r.ruta_fichero : r.motivo)};
        }
        const auto r = capturia::parar_grabacion(capturia::sesion_por_defecto());
        return {r.parado, QString::fromStdString(r.parado ? r.ruta_fichero : r.motivo)};
    }));
    audio_en_curso_ = false;
}

void Controlador::pausar() {
    std::string motivo;
    if (!capturia::poner_pausa(capturia::sesion_por_defecto(), true, motivo)) {
        ponerError(QString::fromStdString(motivo));
        return;
    }
    reloj_.stop();
    ponerEstado(QStringLiteral("pausado"));
}

void Controlador::reanudar() {
    std::string motivo;
    if (!capturia::poner_pausa(capturia::sesion_por_defecto(), false, motivo)) {
        ponerError(QString::fromStdString(motivo));
        return;
    }
    reloj_.start();
    ponerEstado(QStringLiteral("grabando"));
}

void Controlador::ponerEstado(const QString& estado) {
    if (estado_ == estado) return;
    estado_ = estado;
    emit estadoCambiado();
}

void Controlador::ponerError(const QString& error) {
    error_ = error;
    emit errorCambiado();
}
