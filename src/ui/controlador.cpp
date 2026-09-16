// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "controlador.hpp"

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

#include <cstdio>
#include <ctime>
#include <filesystem>

#include "esr/ajustes.hpp"
#include "esr/audio.hpp"
#include "esr/configuracion.hpp"
#include "esr/grabacion.hpp"

namespace {

// Las dos entradas de solo-audio van en la misma lista que las fuentes de
// pantalla: asi "dos clics" vale tambien para una nota de voz. Llevan prefijo
// para que el controlador sepa por que via van.
// Identificadores internos de las dos fuentes de solo-audio. NO son texto
// visible y NO se traducen: viajan del QML a grabar() y se comparan aqui. El
// texto que ve el usuario se compone aparte, con tr(), y puede cambiar de
// idioma sin que nada de esto se entere.
const QString kAudioSistema = QStringLiteral("audio:sistema");
const QString kAudioMicro = QStringLiteral("audio:micro");

// Una entrada del desplegable de fuentes.
QVariantMap fuente(const QString& valor, const QString& texto) {
    return {{QStringLiteral("valor"), valor}, {QStringLiteral("texto"), texto}};
}

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
    const auto sesion = esr::sesion_por_defecto();
    const auto sesion_audio = esr::sesion_audio_por_defecto();
    if (esr::grabacion_en_marcha(sesion)) {
        // El reloj parte del inicio real, no de cero: la grabacion pudo
        // arrancarla el CLI hace rato.
        const long inicio = esr::inicio_grabacion(sesion.ruta_inicio);
        if (inicio > 0) segundos_ = static_cast<int>(std::time(nullptr) - inicio);
        ponerEstado(QStringLiteral("grabando"));
        reloj_.start();
    } else if (esr::audio_en_marcha(sesion_audio)) {
        const long inicio = esr::inicio_grabacion(sesion_audio.ruta_inicio);
        if (inicio > 0) segundos_ = static_cast<int>(std::time(nullptr) - inicio);
        audio_en_curso_ = true;
        ponerEstado(QStringLiteral("grabandoAudio"));
        reloj_.start();
    }
}

// Autoprueba del camino real del controlador, para el arnes: con
// ESR_AUTOPRUEBA=<fuente> graba unos segundos por el mismo codigo que
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
    QVariantMap opciones;
    const QByteArray region = qgetenv("ESR_AUTOPRUEBA_REGION");
    if (!region.isEmpty()) opciones[QStringLiteral("region")] = QString::fromUtf8(region);
    // ESR_AUTOPRUEBA_OPCIONES="clave=valor,clave=valor": el mismo mapa
    // que arma el QML, para verificar que las opciones llegan hasta GSR.
    const QString extra = QString::fromUtf8(qgetenv("ESR_AUTOPRUEBA_OPCIONES"));
    for (const QString& par : extra.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const auto corte = par.indexOf(QLatin1Char('='));
        if (corte > 0) opciones[par.left(corte)] = par.mid(corte + 1);
    }
    grabar(fuente, opciones);
}

void Controlador::detectarEnSegundoPlano() {
    auto* vigilante = new QFutureWatcher<esr::Entorno>(this);
    connect(vigilante, &QFutureWatcher<esr::Entorno>::finished, this, [this, vigilante] {
        aplicarEntorno(vigilante->result());
        vigilante->deleteLater();
        const QByteArray fuente = qgetenv("ESR_AUTOPRUEBA");
        if (!fuente.isEmpty() && estado_ == QStringLiteral("listo")) {
            autoprueba(QString::fromUtf8(fuente));
        }
    });
    vigilante->setFuture(QtConcurrent::run([] { return esr::detectar(); }));
}

QStringList Controlador::formatosAudio() const {
    QStringList lista;
    for (const auto& f : esr::formatos_audio()) lista << QString::fromStdString(f);
    return lista;
}

bool Controlador::formatoAudioSinPerdida(const QString& formato) const {
    return esr::audio_sin_perdida(formato.toStdString());
}

QStringList Controlador::contenedores() const {
    QStringList lista;
    for (const auto& c : esr::contenedores_soportados()) lista << QString::fromStdString(c);
    return lista;
}

QStringList Controlador::codecsAudioPara(const QString& contenedor) const {
    QStringList lista;
    for (const auto& c : esr::codecs_audio_para(contenedor.toStdString())) {
        lista << QString::fromStdString(c);
    }
    return lista;
}

QString Controlador::carpetaVideos() const {
    return QString::fromStdString(esr::carpeta_videos_elegida());
}

QString Controlador::carpetaAudio() const {
    return QString::fromStdString(esr::carpeta_audio_elegida());
}

void Controlador::elegirCarpeta(bool paraAudio, const QUrl& carpeta) {
    const QString ruta = carpeta.toLocalFile();
    if (ruta.isEmpty()) return;
    if (!esr::guardar_ajuste(paraAudio ? "carpeta_audio" : "carpeta_videos",
                                  ruta.toStdString())) {
        ponerError(tr("no se pudo guardar la elección de carpeta"));
        return;
    }
    emit carpetasCambiadas();
}

void Controlador::aplicarEntorno(const esr::Entorno& e) {
    fuentes_.clear();
    codecs_video_.clear();
    // «auto» delante: delega en GSR, que es el criterio probado. El resto,
    // tal como la maquina los nombra; el sufijo _software se enseña como CPU
    // para que nadie lo confunda con hardware.
    codecs_video_ << QStringLiteral("auto");
    for (const auto& c : e.capacidades.info.codecs_video) {
        codecs_video_ << QString::fromStdString(c);
    }
    QStringList vistas;
    for (const auto& f : e.capacidades.fuentes_captura) {
        const QString id = QString::fromStdString(f.id);
        // Una camara con N modos es UNA fuente para el usuario; el modo lo
        // elige GSR. Sin esto la lista enseñaba /dev/video0 ocho veces.
        if (vistas.contains(id)) continue;
        vistas << id;
        // El identificador es el de GSR y no se toca. Solo se traduce el texto
        // de las fuentes especiales, que son las que tienen un nombre que no
        // significa nada para el usuario; un monitor o una camara se enseñan
        // tal como los nombra la maquina.
        QString texto = id;
        if (id == QStringLiteral("region")) {
            texto = tr("Elegir una región arrastrando");
        } else if (id == QStringLiteral("portal")) {
            texto = tr("Preguntar al empezar (lo elige el sistema)");
        } else if (id == QStringLiteral("focused")) {
            texto = tr("La ventana que tenga el foco");
        }
        fuentes_ << fuente(id, texto);
    }
    if (e.graba_audio_solo()) {
        fuentes_ << fuente(kAudioSistema, tr("Solo audio: audio del sistema"));
        fuentes_ << fuente(kAudioMicro, tr("Solo audio: micrófono"));
    }

    if (!e.gsr.presente) {
        diagnostico_ = tr("gpu-screen-recorder no está instalado. Sin él no hay "
                          "grabación de pantalla.\n"
                          "Instálalo nativo o como flatpak y vuelve a abrir "
                          "Easy Screen Recorder.");
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

void Controlador::grabar(const QString& fuente, const QVariantMap& opciones) {
    ponerError({});
    ruta_guardada_.clear();
    emit rutaGuardadaCambiada();

    // Las dos entradas de solo-audio van por ffmpeg, no por GSR. Tambien en
    // hilo aparte: resolver el dispositivo pregunta a pactl y ffmpeg tarda
    // en confirmar que arranco.
    if (fuente == kAudioSistema || fuente == kAudioMicro) {
        esr::AjustesAudio a;
        a.dispositivo = fuente == kAudioMicro ? "default_input" : "default_output";
        a.formato = opciones.value(QStringLiteral("formatoAudio"), QStringLiteral("opus"))
                        .toString()
                        .toStdString();
        const std::string carpeta = esr::carpeta_audio_elegida();
        a.bitrate_kbps = opciones.value(QStringLiteral("bitrateAudio"), 0).toInt();
        a.salida = esr::nombre_por_defecto(carpeta, esr::extension_por_defecto(a.formato));

        ponerEstado(QStringLiteral("arrancando"));
        auto* vigilante = new QFutureWatcher<esr::ResultadoAudio>(this);
        connect(vigilante, &QFutureWatcher<esr::ResultadoAudio>::finished, this,
                [this, vigilante] {
                    const auto r = vigilante->result();
                    vigilante->deleteLater();
                    if (!r.bien) {
                        ponerError(QString::fromStdString(r.motivo));
                        ponerEstado(QStringLiteral("listo"));
                        return;
                    }
                    audio_en_curso_ = true;
                    segundos_ = 0;
                    emit segundosCambiados();
                    reloj_.start();
                    ponerEstado(QStringLiteral("grabandoAudio"));
                });
        vigilante->setFuture(QtConcurrent::run([a, carpeta] {
            // La carpeta por defecto del usuario se crea si falta: es la que
            // el ya tiene configurada en XDG, no una inventada.
            std::error_code ec;
            std::filesystem::create_directories(carpeta, ec);
            return esr::empezar_audio(a, esr::sesion_audio_por_defecto());
        }));
        return;
    }

    esr::AjustesGrabacion a;
    a.fuente = fuente.toStdString();
    const std::string carpeta_v = esr::carpeta_videos_elegida();
    const QString contenedor =
        opciones.value(QStringLiteral("contenedor"), QStringLiteral("mkv")).toString();
    a.salida = esr::nombre_por_defecto(carpeta_v, contenedor.toStdString());
    a.calidad = opciones.value(QStringLiteral("calidad"), QStringLiteral("very_high"))
                    .toString()
                    .toStdString();
    a.fps = opciones.value(QStringLiteral("fps"), 60).toInt();
    a.codec_video =
        opciones.value(QStringLiteral("codecVideo"), QStringLiteral("auto")).toString().toStdString();
    a.codec_audio =
        opciones.value(QStringLiteral("codecAudio"), QStringLiteral("opus")).toString().toStdString();
    a.region = opciones.value(QStringLiteral("region")).toString().toStdString();

    const QString audio = opciones.value(QStringLiteral("audio"), QStringLiteral("sistema")).toString();
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
    {
        std::error_code ec;
        std::filesystem::create_directories(carpeta_v, ec);
    }

    // empezar_grabacion espera al socket del grabador (hasta 15 s si el
    // flatpak arranca frio), asi que fuera del hilo de la ventana.
    ponerEstado(QStringLiteral("arrancando"));
    auto* vigilante = new QFutureWatcher<esr::ResultadoLanzamiento>(this);
    connect(vigilante, &QFutureWatcher<esr::ResultadoLanzamiento>::finished, this,
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
        [a] { return esr::empezar_grabacion(a, esr::sesion_por_defecto()); }));
}

void Controlador::alternarGrabacion() {
    if (estado_ == QStringLiteral("grabando") || estado_ == QStringLiteral("pausado") ||
        estado_ == QStringLiteral("grabandoAudio")) {
        parar();
        return;
    }
    if (estado_ != QStringLiteral("listo")) return;
    // El atajo global graba lo mas obvio: el primer monitor. Se descartan las
    // fuentes que necesitan una decision del usuario —region, portal, la
    // ventana en foco— y las camaras y el solo-audio, que no son «grabar la
    // pantalla». Se compara contra el IDENTIFICADOR y no contra el texto
    // visible: antes buscaba el prefijo «Solo audio», y en cualquier idioma que
    // no fuera castellano eso habria dejado que el atajo arrancara una
    // grabacion de audio creyendo que era un monitor.
    for (const QVariant& entrada : fuentes_) {
        const QString f = entrada.toMap().value(QStringLiteral("valor")).toString();
        if (f.startsWith(QStringLiteral("audio:"))) continue;
        if (f == QStringLiteral("region") || f == QStringLiteral("portal") ||
            f == QStringLiteral("focused") || f.startsWith(QStringLiteral("/dev/"))) {
            continue;
        }
        grabar(f, {});
        return;
    }
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
            const auto r = esr::parar_audio(esr::sesion_audio_por_defecto());
            return {r.bien, QString::fromStdString(r.bien ? r.ruta_fichero : r.motivo)};
        }
        const auto r = esr::parar_grabacion(esr::sesion_por_defecto());
        return {r.parado, QString::fromStdString(r.parado ? r.ruta_fichero : r.motivo)};
    }));
    audio_en_curso_ = false;
}

void Controlador::pausar() {
    std::string motivo;
    if (!esr::poner_pausa(esr::sesion_por_defecto(), true, motivo)) {
        ponerError(QString::fromStdString(motivo));
        return;
    }
    reloj_.stop();
    ponerEstado(QStringLiteral("pausado"));
}

void Controlador::reanudar() {
    std::string motivo;
    if (!esr::poner_pausa(esr::sesion_por_defecto(), false, motivo)) {
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
