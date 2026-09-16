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

// El nombre de un monitor, ya clasificado por libesr. Aqui solo estan las
// palabras: la logica —agrupar, ordenar, desempatar— vive en el nucleo, que es
// donde se puede probar con ctest y sin Qt.
QString textoMonitor(const esr::FuenteAmable& f) {
    switch (f.familia) {
        case esr::FamiliaMonitor::Interna: return Controlador::tr("Pantalla del portátil");
        case esr::FamiliaMonitor::Hdmi: return Controlador::tr("Pantalla HDMI");
        case esr::FamiliaMonitor::DisplayPort: return Controlador::tr("Pantalla DisplayPort");
        case esr::FamiliaMonitor::Vga: return Controlador::tr("Pantalla VGA");
        case esr::FamiliaMonitor::Dvi: return Controlador::tr("Pantalla DVI");
        case esr::FamiliaMonitor::Otra: break;
    }
    return Controlador::tr("Pantalla");
}

// «1366x768» se enseña como «1366×768»: el signo de multiplicar, no una equis.
QString resolucionBonita(const std::string& resolucion) {
    if (resolucion.empty()) return {};
    return QString::fromStdString(resolucion).replace(QLatin1Char('x'), QChar(0x00D7));
}

}  // namespace

Controlador::Controlador(QObject* padre) : QObject(padre), medidor_(this) {
    connect(&medidor_, &Medidor::nivelCambiado, this, &Controlador::nivelMicroCambiado);
    reloj_.setInterval(1000);
    connect(&reloj_, &QTimer::timeout, this, [this] {
        ++segundos_;
        emit segundosCambiados();
    });

    // ¿Quedo algo a medias la ultima vez? Se mira al abrir, que es cuando el
    // usuario puede hacer algo al respecto. Es barato: mirar un fichero.
    grabacion_a_medias_ =
        QString::fromStdString(esr::grabacion_a_medias(esr::sesion_por_defecto()));

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
        // De replay o normal: el socket no lo dice, lo dice la marca que dejo
        // quien la arranco. Confundirlos hace que «Parar y guardar» tire el
        // buffer sin guardarlo.
        ponerEstado(esr::replay_en_marcha(sesion.ruta_replay) != 0
                        ? QStringLiteral("replay")
                        : QStringLiteral("grabando"));
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

bool Controlador::modoContentEfectivo(const QString& fuente) const {
    return esr::modo_content_efectivo(fuente.toStdString(), servidor_grafico_.toStdString());
}

bool Controlador::hayMultimedia() const { return Medidor::disponible(); }

qreal Controlador::nivelMicro() const { return medidor_.nivel(); }

void Controlador::escucharMicro(bool si) {
    // Nunca mientras se graba: medir ahi no sirve para decidir nada y ademas
    // abre un flujo de audio de mas.
    medidor_.escuchar(si && estado_ == QStringLiteral("listo"));
}

qreal Controlador::proporcionCamara(const QString& id) const {
    for (const auto& f : camaras_) {
        if (f.toMap().value(QStringLiteral("valor")).toString() != id) continue;
        const QString res = f.toMap().value(QStringLiteral("proporcion")).toString();
        const auto partes = res.split(QLatin1Char('x'));
        if (partes.size() == 2) {
            bool bien_a = false;
            bool bien_b = false;
            const int ancho = partes[0].toInt(&bien_a);
            const int alto = partes[1].toInt(&bien_b);
            if (bien_a && bien_b && alto > 0) return static_cast<qreal>(ancho) / alto;
        }
    }
    // Sin dato, 16:9: es lo que trae casi cualquier camara de portatil, y una
    // proporcion inventada solo afecta a como se DIBUJA el recuadro, nunca a lo
    // que se graba, que lo decide GSR con la imagen real delante.
    return 16.0 / 9.0;
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

QStringList Controlador::codecsVideoPara(const QString& contenedor,
                                         const QStringList& disponibles) const {
    std::vector<std::string> entrada;
    entrada.reserve(static_cast<std::size_t>(disponibles.size()));
    for (const QString& c : disponibles) entrada.push_back(c.toStdString());

    QStringList lista;
    for (const auto& c : esr::codecs_video_para(contenedor.toStdString(), entrada)) {
        lista << QString::fromStdString(c);
    }
    return lista;
}

QStringList Controlador::codecsAudioPara(const QString& contenedor, bool mezcla) const {
    QStringList lista;
    for (const auto& c : esr::codecs_audio_para(contenedor.toStdString(), mezcla)) {
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
    // «auto» delante: delega en GSR, que es el criterio probado, y ademas es el
    // unico que acierta siempre con el contenedor. El resto, tal como la maquina
    // los nombra, pero solo los que GSR acepta de verdad en -k: «h264_software»
    // salia de --info y mataba al grabador al arrancar.
    codecs_video_ << QStringLiteral("auto");
    for (const auto& c : esr::codecs_video_ofrecibles(e.capacidades.info)) {
        codecs_video_ << QString::fromStdString(c);
    }
    // El identificador que viaja a GSR es el suyo y no se toca NUNCA. Lo que se
    // compone aqui es solo el texto visible.
    //
    // Antes ese texto era el identificador tal cual, y el desplegable decia
    // «eDP-1» y «/dev/video0». Eso es el nombre de un conector DRM y la ruta de
    // un nodo de dispositivo: correcto, comprobable y completamente opaco para
    // quien solo quiere grabar su pantalla.
    for (const auto& f : esr::fuentes_amables(e.capacidades.fuentes_captura)) {
        const QString id = QString::fromStdString(f.id);
        QString texto;
        switch (f.tipo) {
            case esr::TipoFuente::Monitor:
                texto = textoMonitor(f);
                break;
            case esr::TipoFuente::Region:
                texto = tr("Elegir una región arrastrando");
                break;
            case esr::TipoFuente::Portal:
                // El texto anterior era «Preguntar al empezar (lo elige el
                // sistema)», que dice quien decide y no QUE se decide. Lo que
                // pasa de verdad es que el escritorio abre su dialogo de
                // compartir pantalla y ahi se elige una ventana o una pantalla.
                texto = tr("Elegir ventana o pantalla al empezar");
                break;
            case esr::TipoFuente::VentanaActiva:
                texto = tr("La ventana activa");
                break;
            case esr::TipoFuente::Camara:
                texto = tr("Cámara");
                if (!f.nombre.empty()) {
                    texto += QStringLiteral(" · ") + QString::fromStdString(f.nombre);
                }
                break;
        }
        // El identificador solo aparece cuando hace falta para distinguir: con
        // dos pantallas DisplayPort, «Pantalla DisplayPort» dos veces no se
        // puede elegir.
        if (f.desempatar) texto += QStringLiteral(" (%1)").arg(id);
        const QString resolucion = resolucionBonita(f.resolucion);
        if (!resolucion.isEmpty()) texto += QStringLiteral(" · ") + resolucion;
        fuentes_ << fuente(id, texto);
    }

    // La camara va DOS veces en la interfaz, y no es una duplicidad: como fuente
    // suelta (grabar solo la camara) y como superposicion (la camara ADEMAS de
    // la pantalla). Son dos cosas distintas y las dos se usan.
    camaras_.clear();
    for (const auto& f : esr::fuentes_amables(e.capacidades.fuentes_captura)) {
        if (f.tipo != esr::TipoFuente::Camara) continue;
        QString texto = tr("Cámara");
        if (!f.nombre.empty()) texto += QStringLiteral(" · ") + QString::fromStdString(f.nombre);
        if (f.desempatar) texto += QStringLiteral(" (%1)").arg(QString::fromStdString(f.id));
        QVariantMap entrada = fuente(QString::fromStdString(f.id), texto);
        entrada[QStringLiteral("proporcion")] = QString::fromStdString(f.resolucion);
        camaras_ << entrada;
    }

    ponerAplicacionesSonando(e.capacidades.audio_por_aplicacion);
    servidor_grafico_ = QString::fromStdString(e.capacidades.info.servidor_grafico);

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
    a.camara = opciones.value(QStringLiteral("camara")).toString().toStdString();
    a.camara_ancho_pct = opciones.value(QStringLiteral("camaraTamano"), 25).toInt();
    a.camara_x_pct = opciones.value(QStringLiteral("camaraX"), 73).toInt();
    a.camara_y_pct = opciones.value(QStringLiteral("camaraY"), 73).toInt();
    a.camara_espejo = opciones.value(QStringLiteral("camaraEspejo"), true).toBool();
    a.modo_fotogramas =
        opciones.value(QStringLiteral("modoFotogramas")).toString().toStdString();
    a.limite_resolucion =
        opciones.value(QStringLiteral("limiteResolucion")).toString().toStdString();
    a.replay_segundos = opciones.value(QStringLiteral("replaySegundos"), 0).toInt();
    a.contenedor = contenedor.toStdString();
    // En modo replay la salida es la CARPETA: el nombre de cada volcado lo pone
    // GSR, y darle un nombre de fichero le haria escribir dentro de el como si
    // fuera un directorio.
    if (a.replay_segundos != 0) a.salida = carpeta_v;

    const QString audio = opciones.value(QStringLiteral("audio"), QStringLiteral("sistema")).toString();
    if (audio == QStringLiteral("micro")) {
        a.audios = {"default_input"};
    } else if (audio == QStringLiteral("mezclado")) {
        // Una sola pista con las dos fuentes dentro, que es la sintaxis «a|b»
        // de GSR. Es lo que hay que pedir para que se oiga todo en cualquier
        // reproductor: con dos pistas separadas, casi todos suenan solo la
        // primera y el microfono parece que no se grabo.
        a.audios = {"default_output|default_input"};
    } else if (audio == QStringLiteral("ambos")) {
        // Dos pistas separadas: se pueden equilibrar despues al editar, a
        // cambio de que un reproductor normal solo suene una.
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
    const bool es_replay = a.replay_segundos != 0;
    ponerEstado(QStringLiteral("arrancando"));
    auto* vigilante = new QFutureWatcher<esr::ResultadoLanzamiento>(this);
    connect(vigilante, &QFutureWatcher<esr::ResultadoLanzamiento>::finished, this,
            [this, vigilante, es_replay] {
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
                ponerEstado(es_replay ? QStringLiteral("replay") : QStringLiteral("grabando"));
            });
    vigilante->setFuture(QtConcurrent::run(
        [a] { return esr::empezar_grabacion(a, esr::sesion_por_defecto()); }));
}

void Controlador::alternarGrabacion() {
    if (estado_ == QStringLiteral("replay")) {
        // En replay el atajo GUARDA, no para: es lo que uno quiere del «se me ha
        // escapado eso, salvalo». Terminar se hace desde la ventana.
        guardarReplay();
        return;
    }
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

void Controlador::ponerAtajos(bool hay, const QString& grabar, const QString& pausa) {
    hay_atajos_ = hay;
    atajo_grabar_ = grabar;
    atajo_pausa_ = pausa;
    emit hayAtajosCambiado();
}

void Controlador::ponerAplicacionesSonando(const std::vector<esr::Opcion>& lista) {
    audios_aplicacion_.clear();
    for (const auto& a : lista) {
        const QString nombre = QString::fromStdString(a.id);
        // El identificador que viaja a GSR lleva el prefijo «app:»; el texto, no.
        audios_aplicacion_ << fuente(QStringLiteral("app:") + nombre, nombre);
    }
    emit audiosAplicacionCambiados();
}

void Controlador::refrescarAplicacionesSonando() {
    // En hilo aparte: es un proceso externo y con el flatpak de GSR tarda unas
    // decimas. Congelar el menu justo al desplegarlo se nota mucho.
    auto* vigilante = new QFutureWatcher<std::vector<esr::Opcion>>(this);
    connect(vigilante, &QFutureWatcher<std::vector<esr::Opcion>>::finished, this,
            [this, vigilante] {
                ponerAplicacionesSonando(vigilante->result());
                vigilante->deleteLater();
            });
    vigilante->setFuture(QtConcurrent::run([] { return esr::aplicaciones_sonando(); }));
}

void Controlador::alternarPausa() {
    if (estado_ == QStringLiteral("grabando")) {
        pausar();
    } else if (estado_ == QStringLiteral("pausado")) {
        reanudar();
    }
    // En solo-audio y en replay no se hace nada: GSR no pausa el primero, y
    // pausar un buffer de repeticion no significa gran cosa. Callar es mejor
    // que un error por pulsar una tecla que ahi no toca.
}

void Controlador::guardarReplay() {
    ponerError({});
    // No se cambia de estado ni se para el reloj: el replay SIGUE. Esto no es
    // terminar, es llevarse una copia de lo que hay en el buffer.
    auto* vigilante = new QFutureWatcher<QPair<bool, QString>>(this);
    connect(vigilante, &QFutureWatcher<QPair<bool, QString>>::finished, this,
            [this, vigilante] {
                const auto [bien, texto] = vigilante->result();
                vigilante->deleteLater();
                if (!bien) {
                    ponerError(texto);
                    return;
                }
                ruta_guardada_ = texto;
                emit rutaGuardadaCambiada();
                emit grabacionGuardada(texto);
            });
    vigilante->setFuture(QtConcurrent::run([]() -> QPair<bool, QString> {
        const auto r = esr::guardar_replay(esr::sesion_por_defecto());
        return {r.parado, QString::fromStdString(r.parado ? r.ruta_fichero : r.motivo)};
    }));
}

void Controlador::repararGrabacion() {
    if (grabacion_a_medias_.isEmpty()) return;
    const std::string ruta = grabacion_a_medias_.toStdString();
    ponerError({});

    auto* vigilante = new QFutureWatcher<QPair<bool, QString>>(this);
    connect(vigilante, &QFutureWatcher<QPair<bool, QString>>::finished, this,
            [this, vigilante, ruta] {
                const auto [bien, motivo] = vigilante->result();
                vigilante->deleteLater();
                if (!bien) {
                    ponerError(motivo);
                    return;
                }
                // Se deja de ofrecer y se enseña como recien guardada, que es lo
                // que es: un fichero que ya se puede abrir.
                olvidarGrabacionAMedias();
                ruta_guardada_ = QString::fromStdString(ruta);
                emit rutaGuardadaCambiada();
            });
    vigilante->setFuture(QtConcurrent::run([ruta]() -> QPair<bool, QString> {
        std::string motivo;
        const bool bien = esr::reparar_grabacion(ruta, motivo);
        return {bien, QString::fromStdString(motivo)};
    }));
}

void Controlador::olvidarGrabacionAMedias() {
    if (grabacion_a_medias_.isEmpty()) return;
    grabacion_a_medias_.clear();
    // Se borra la marca para no volver a preguntar por lo mismo en cada
    // arranque: quien dice que no, lo dice una vez.
    std::error_code ec;
    std::filesystem::remove(esr::sesion_por_defecto().ruta_salida, ec);
    emit grabacionAMediasCambiada();
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
    // Al salir de «listo» se suelta el microfono sin que nadie tenga que
    // acordarse: el vumetro solo tiene sentido antes de empezar.
    if (estado_ != QStringLiteral("listo")) medidor_.escuchar(false);
    emit estadoCambiado();
}

void Controlador::ponerError(const QString& error) {
    error_ = error;
    emit errorCambiado();
}
