// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>
#include <QTimer>
#include <qqmlregistration.h>

#include "esr/entorno.hpp"
#include "medidor.hpp"

// El puente entre QML y libesr. Todo lo que la UI sabe pasa por aqui, y
// aqui no hay logica de grabacion: solo llamadas a la capa 1 y estado para
// pintar. La regla de CLAUDE.md se mantiene: lo que no funcione por CLI no
// se toca en la UI, porque ambas llaman a lo mismo.
class Controlador : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // "detectando" | "listo" | "grabando" | "pausado" | "grabandoAudio" |
    // "replay" | "emitiendo" | "sinGsr"
    //
    // «replay» es grabar sin escribir: GSR guarda en memoria los ultimos N
    // segundos y no vuelca nada hasta que se le pide. Es un estado aparte y no
    // un matiz de «grabando» porque los botones son otros: ahi no se para y
    // guarda, se GUARDA y se sigue.
    Q_PROPERTY(QString estado READ estado NOTIFY estadoCambiado)
    // Lo que la maquina soporta de verdad Y GSR acepta en -k. Nunca una lista
    // escrita en el QML. Se expone para que el QML sepa cuando cambia; lo que
    // pinta el selector sale de codecsVideoPara(), que ademas filtra por el
    // contenedor elegido.
    Q_PROPERTY(QStringList codecsVideo READ codecsVideo NOTIFY fuentesCambiadas)
    Q_PROPERTY(QStringList contenedores READ contenedores CONSTANT)
    // Donde se guarda: lo elegido por el usuario, con memoria entre
    // sesiones, o el default XDG si nunca eligio.
    Q_PROPERTY(QString carpetaVideos READ carpetaVideos NOTIFY carpetasCambiadas)
    Q_PROPERTY(QString carpetaAudio READ carpetaAudio NOTIFY carpetasCambiadas)
    // Lista de {texto, valor}: el texto se TRADUCE y el valor NO.
    //
    // Antes era un QStringList y el valor era el propio texto visible, asi que
    // el QML decidia el modo con `currentText.indexOf("Solo audio") === 0` y
    // grabar() comparaba contra la cadena en castellano. Con la interfaz
    // traducida eso se rompe en silencio: en ingles el prefijo no casa, y el
    // formulario ensena los controles de video en modo solo-audio. El texto
    // visible no puede ser tambien el identificador.
    Q_PROPERTY(QVariantList fuentes READ fuentes NOTIFY fuentesCambiadas)
    // Las camaras, aparte de las fuentes: una camara superpuesta NO es una
    // fuente alternativa, va ademas de la pantalla. Lista de {texto, valor}.
    Q_PROPERTY(QVariantList camaras READ camaras NOTIFY fuentesCambiadas)
    // Las aplicaciones que estan sonando ahora mismo, para grabar solo su audio
    // («app:nombre» de GSR). Cambia entre grabacion y grabacion, asi que se
    // relee al desplegar, no se cachea.
    Q_PROPERTY(QVariantList audiosAplicacion READ audiosAplicacion
                   NOTIFY audiosAplicacionCambiados)
    // «wayland» o «x11», segun lo que diga GSR. La UI lo necesita para no
    // ofrecer el modo «content», que en Wayland sobre un monitor no hace nada.
    Q_PROPERTY(QString servidorGrafico READ servidorGrafico NOTIFY fuentesCambiadas)
    // El servidor de ingesta recordado, para no escribirlo cada vez. La clave
    // NO se recuerda nunca y no tiene propiedad: vive en el campo de texto y se
    // va con la ventana.
    Q_PROPERTY(QString urlEmision READ urlEmision CONSTANT)
    Q_PROPERTY(QString diagnostico READ diagnostico NOTIFY estadoCambiado)
    Q_PROPERTY(QString rutaGuardada READ rutaGuardada NOTIFY rutaGuardadaCambiada)
    Q_PROPERTY(QString error READ error NOTIFY errorCambiado)
    // La grabacion que quedo a medias por un apagon, si la hay. Vacio si no.
    // La interfaz lo ofrece sola: el fichero tiene el video dentro pero en mkv
    // no se abre sin rehacerlo, y nadie tiene por que saber eso.
    Q_PROPERTY(QString grabacionAMedias READ grabacionAMedias NOTIFY grabacionAMediasCambiada)
    Q_PROPERTY(int segundos READ segundos NOTIFY segundosCambiados)
    // Si esta compilacion trae vista previa de camara y vumetro. Sale de si
    // habia Qt6Multimedia al compilar; el QML esconde los controles cuando no.
    Q_PROPERTY(bool hayMultimedia READ hayMultimedia CONSTANT)
    // Si los atajos globales estan registrados de verdad. Fuera de KDE no hay
    // servicio que los atienda, y anunciar unos atajos que no funcionan es peor
    // que no anunciar ninguno.
    Q_PROPERTY(bool hayAtajos READ hayAtajos NOTIFY hayAtajosCambiado)
    // El texto de cada atajo tal como lo tiene KDE ahora mismo, no el que
    // pedimos: el usuario puede haberlos cambiado.
    Q_PROPERTY(QString atajoGrabar READ atajoGrabar NOTIFY hayAtajosCambiado)
    Q_PROPERTY(QString atajoPausa READ atajoPausa NOTIFY hayAtajosCambiado)
    // 0 a 1. Solo se mueve mientras se esta escuchando, o sea antes de grabar.
    Q_PROPERTY(qreal nivelMicro READ nivelMicro NOTIFY nivelMicroCambiado)

public:
    explicit Controlador(QObject* padre = nullptr);

    QString estado() const { return estado_; }
    QVariantList fuentes() const { return fuentes_; }
    QStringList codecsVideo() const { return codecs_video_; }
    QStringList contenedores() const;
    // Los codecs de video que caben en ese contenedor. Un «.webm» no admite h264
    // ni hevc y un «.mp4» no admite vp8: sin este filtro se podia pedir una
    // pareja que no graba NADA.
    //
    // `disponibles` se PASA y no se lee de dentro, aunque de dentro estaria a
    // mano. El motivo es de QML: un binding a una funcion invocable solo se
    // reevalua cuando cambia alguna propiedad que el binding lee, y la
    // deteccion del entorno termina despues de pintar la ventana. Leyendo la
    // lista por dentro, el selector se quedaba vacio para siempre; recibiendola
    // como argumento, el binding depende de `codecsVideo`, que notifica.
    Q_INVOKABLE QStringList codecsVideoPara(const QString& contenedor,
                                            const QStringList& disponibles) const;
    // `mezcla` es «las dos fuentes van en una sola pista»: ahi flac no entra,
    // porque GSR lo cambiaria a opus por detras (codec_select.c:186-191).
    Q_INVOKABLE QStringList codecsAudioPara(const QString& contenedor, bool mezcla = false) const;
    // Los formatos del modo solo-audio y si el elegido pierde informacion. El
    // QML usa lo segundo para ESCONDER el bitrate, no para deshabilitarlo: un
    // control en gris invita a preguntarse por que, y en flac la respuesta es
    // que ese ajuste no existe.
    QVariantList camaras() const { return camaras_; }
    QVariantList audiosAplicacion() const { return audios_aplicacion_; }
    QString servidorGrafico() const { return servidor_grafico_; }
    QString urlEmision() const;
    // Si «-fm content» va a servir de algo con esa fuente. GSR lo acepta
    // siempre y avisa por stderr cuando lo ignora, asi que se filtra aqui.
    Q_INVOKABLE bool modoContentEfectivo(const QString& fuente) const;
    Q_INVOKABLE QStringList formatosAudio() const;
    Q_INVOKABLE bool formatoAudioSinPerdida(const QString& formato) const;
    QString carpetaVideos() const;
    QString carpetaAudio() const;
    Q_INVOKABLE void elegirCarpeta(bool paraAudio, const QUrl& carpeta);
    QString diagnostico() const { return diagnostico_; }
    QString rutaGuardada() const { return ruta_guardada_; }
    QString error() const { return error_; }
    int segundos() const { return segundos_; }
    QString grabacionAMedias() const { return grabacion_a_medias_; }
    // Rehace esa grabacion. Va en hilo aparte: copiar los flujos de un video
    // largo tarda, y bloquear la ventana para esto seria absurdo.
    Q_INVOKABLE void repararGrabacion();
    // Y dejarla como esta, que tambien es una respuesta valida.
    Q_INVOKABLE void olvidarGrabacionAMedias();
    bool hayMultimedia() const;
    bool hayAtajos() const { return hay_atajos_; }
    QString atajoGrabar() const { return atajo_grabar_; }
    QString atajoPausa() const { return atajo_pausa_; }
    void ponerAtajos(bool hay, const QString& grabar, const QString& pausa);
    qreal nivelMicro() const;
    // Enciende o apaga el vumetro. Lo llama el QML: escucha solo cuando el
    // control esta a la vista y no hay grabacion, que es cuando sirve.
    Q_INVOKABLE void escucharMicro(bool si);

    // La proporcion (ancho/alto) de esa camara segun su mejor modo, o 16/9 si
    // no se sabe. La interfaz la usa para dibujar su recuadro a escala mientras
    // se coloca: un 4:3 no se situa igual que un 16:9.
    Q_INVOKABLE qreal proporcionCamara(const QString& id) const;

    // Emite la pantalla en directo. La clave llega como argumento y NO se
    // guarda: ni en la configuracion ni en una propiedad ni en el log.
    Q_INVOKABLE void emitir(const QString& fuente, const QString& servidor,
                            const QString& clave, int bitrateKbps,
                            const QVariantMap& opciones);

    // opciones: calidad (medium|high|very_high|ultra), fps, audio
    // (sistema|micro|mezclado|ambos|nada), contenedor (mkv|mp4|webm), codecVideo
    // ("auto" o uno detectado), codecAudio, region ("WxH+X+Y", solo con
    // fuente «region») y formatoAudio (opus|flac, solo para solo-audio).
    // Lo ausente cae al default del proyecto.
    Q_INVOKABLE void grabar(const QString& fuente, const QVariantMap& opciones);
    // Lo que dispara el atajo global: sin grabacion empieza una del primer
    // monitor con los defaults; con una en marcha, la para. Sin estados
    // intermedios: pulsado en «arrancando» o «guardando» no hace nada.
    Q_INVOKABLE void alternarGrabacion();
    // Vuelca el buffer de replay a un fichero y sigue grabando. La respuesta
    // trae la ruta y tarda lo que tarde en escribirse, asi que va en hilo
    // aparte igual que parar().
    // Pausa si esta grabando, reanuda si esta en pausa, y no hace nada en el
    // resto de estados. Lo dispara el atajo global, que es la unica via de
    // pausar sin que la ventana aparezca en el video que se esta grabando.
    // Vuelve a preguntar que aplicaciones estan sonando. Lo llama el QML al
    // desplegar el selector de audio: la lista caduca en cuanto alguien abre o
    // cierra algo, y la calculada al arrancar casi nunca es la buena.
    Q_INVOKABLE void refrescarAplicacionesSonando();
    Q_INVOKABLE void alternarPausa();
    Q_INVOKABLE void guardarReplay();
    Q_INVOKABLE void parar();
    Q_INVOKABLE void pausar();
    Q_INVOKABLE void reanudar();

signals:
    void estadoCambiado();
    void carpetasCambiadas();
    void fuentesCambiadas();
    void rutaGuardadaCambiada();
    void errorCambiado();
    void segundosCambiados();
    void nivelMicroCambiado();
    void grabacionAMediasCambiada();
    void hayAtajosCambiado();
    void audiosAplicacionCambiados();
    void grabacionGuardada(const QString& ruta);

private:
    void autoprueba(const QString& fuente);
    void detectarEnSegundoPlano();
    void aplicarEntorno(const esr::Entorno& e);
    void ponerAplicacionesSonando(const std::vector<esr::Opcion>& lista);
    void ponerEstado(const QString& estado);
    void ponerError(const QString& error);

    QString estado_ = QStringLiteral("detectando");
    QVariantList fuentes_;
    QVariantList camaras_;
    QVariantList audios_aplicacion_;
    QString servidor_grafico_;
    QStringList codecs_video_;
    QString diagnostico_;
    QString ruta_guardada_;
    QString grabacion_a_medias_;
    QString error_;
    int segundos_ = 0;
    QTimer reloj_;
    bool audio_en_curso_ = false;
    bool hay_atajos_ = false;
    QString atajo_grabar_;
    QString atajo_pausa_;
    Medidor medidor_;
};
