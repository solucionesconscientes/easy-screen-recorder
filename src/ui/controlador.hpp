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

// El puente entre QML y libesr. Todo lo que la UI sabe pasa por aqui, y
// aqui no hay logica de grabacion: solo llamadas a la capa 1 y estado para
// pintar. La regla de CLAUDE.md se mantiene: lo que no funcione por CLI no
// se toca en la UI, porque ambas llaman a lo mismo.
class Controlador : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // "detectando" | "listo" | "grabando" | "pausado" | "grabandoAudio" | "sinGsr"
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
    Q_PROPERTY(QString diagnostico READ diagnostico NOTIFY estadoCambiado)
    Q_PROPERTY(QString rutaGuardada READ rutaGuardada NOTIFY rutaGuardadaCambiada)
    Q_PROPERTY(QString error READ error NOTIFY errorCambiado)
    Q_PROPERTY(int segundos READ segundos NOTIFY segundosCambiados)

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
    Q_INVOKABLE QStringList formatosAudio() const;
    Q_INVOKABLE bool formatoAudioSinPerdida(const QString& formato) const;
    QString carpetaVideos() const;
    QString carpetaAudio() const;
    Q_INVOKABLE void elegirCarpeta(bool paraAudio, const QUrl& carpeta);
    QString diagnostico() const { return diagnostico_; }
    QString rutaGuardada() const { return ruta_guardada_; }
    QString error() const { return error_; }
    int segundos() const { return segundos_; }

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
    void grabacionGuardada(const QString& ruta);

private:
    void autoprueba(const QString& fuente);
    void detectarEnSegundoPlano();
    void aplicarEntorno(const esr::Entorno& e);
    void ponerEstado(const QString& estado);
    void ponerError(const QString& error);

    QString estado_ = QStringLiteral("detectando");
    QVariantList fuentes_;
    QStringList codecs_video_;
    QString diagnostico_;
    QString ruta_guardada_;
    QString error_;
    int segundos_ = 0;
    QTimer reloj_;
    bool audio_en_curso_ = false;
};
