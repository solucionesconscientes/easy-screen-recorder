#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <qqmlregistration.h>

#include "capturia/entorno.hpp"

// El puente entre QML y libcapturia. Todo lo que la UI sabe pasa por aqui, y
// aqui no hay logica de grabacion: solo llamadas a la capa 1 y estado para
// pintar. La regla de CLAUDE.md se mantiene: lo que no funcione por CLI no
// se toca en la UI, porque ambas llaman a lo mismo.
class Controlador : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // "detectando" | "listo" | "grabando" | "pausado" | "grabandoAudio" | "sinGsr"
    Q_PROPERTY(QString estado READ estado NOTIFY estadoCambiado)
    Q_PROPERTY(QStringList fuentes READ fuentes NOTIFY fuentesCambiadas)
    Q_PROPERTY(QString diagnostico READ diagnostico NOTIFY estadoCambiado)
    Q_PROPERTY(QString rutaGuardada READ rutaGuardada NOTIFY rutaGuardadaCambiada)
    Q_PROPERTY(QString error READ error NOTIFY errorCambiado)
    Q_PROPERTY(int segundos READ segundos NOTIFY segundosCambiados)

public:
    explicit Controlador(QObject* padre = nullptr);

    QString estado() const { return estado_; }
    QStringList fuentes() const { return fuentes_; }
    QString diagnostico() const { return diagnostico_; }
    QString rutaGuardada() const { return ruta_guardada_; }
    QString error() const { return error_; }
    int segundos() const { return segundos_; }

    // calidad: medium|high|very_high|ultra. audio: sistema|micro|ambos|nada.
    Q_INVOKABLE void grabar(const QString& fuente, const QString& calidad, int fps,
                            const QString& audio);
    Q_INVOKABLE void parar();
    Q_INVOKABLE void pausar();
    Q_INVOKABLE void reanudar();

signals:
    void estadoCambiado();
    void fuentesCambiadas();
    void rutaGuardadaCambiada();
    void errorCambiado();
    void segundosCambiados();
    void grabacionGuardada(const QString& ruta);

private:
    void autoprueba(const QString& fuente);
    void detectarEnSegundoPlano();
    void aplicarEntorno(const capturia::Entorno& e);
    void ponerEstado(const QString& estado);
    void ponerError(const QString& error);

    QString estado_ = QStringLiteral("detectando");
    QStringList fuentes_;
    QString diagnostico_;
    QString ruta_guardada_;
    QString error_;
    int segundos_ = 0;
    QTimer reloj_;
    bool audio_en_curso_ = false;
};
